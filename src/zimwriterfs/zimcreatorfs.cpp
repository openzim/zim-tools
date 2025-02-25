/*
 * Copyright 2013-2016 Emmanuel Engelhart <kelson@kiwix.org>
 * Copyright 2016 Matthieu Gautier <mgautier@kymeria.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU  General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include "zimcreatorfs.h"
#include "../mimetypes.h"
#include "../tools.h"
#include "tools.h"

#include <fstream>
#include <dirent.h>
#include <sys/stat.h>
#include <regex>
#include <unistd.h>
#include <limits.h>
#include <cassert>

void parse_redirectArticles(std::istream& in_stream, redirect_handler handler) {
  std::string line;
  int line_number = 1;
  while (std::getline(in_stream, line)) {
    std::regex line_regex("^([^\\t]+)\\t([^\\t]+)\\t([^\\t]+)$");
    std::smatch matches;
    if (!std::regex_search(line, matches, line_regex) || matches.size() != 4) {
      throw std::runtime_error(
        Formatter() << "Invalid line #" << line_number << " : '" << line << "'"
      );
    }

    Redirect redirect = {
      .path= matches[1].str(),
      .title = matches[2].str(),
      .target = matches[3].str()
    };
    handler(redirect);
    ++line_number;
  }
}

bool isVerbose();

ZimCreatorFS::ZimCreatorFS(std::string _directoryPath)
  : directoryPath(_directoryPath)
{
  char buf[PATH_MAX];

  if (realpath(directoryPath.c_str(), buf) != buf) {
    throw std::invalid_argument(
          Formatter() << "Unable to canonicalize HTML directory path "
                      << directoryPath << ": " << strerror(errno));
  }

  canonical_basedir = buf;
}

void ZimCreatorFS::add_redirectArticles_from_file(const std::string& path)
{
  std::ifstream in_stream;

  in_stream.open(path.c_str());
  try {
    parse_redirectArticles(in_stream,
      [this](Redirect redirect) {
        this->addRedirection(
          redirect.path,
          redirect.title,
          redirect.target,
          {{zim::writer::HintKeys::FRONT_ARTICLE, 1}}
        );
      }
    );
  } catch(const std::runtime_error& e) {
    std::cerr << e.what() << "\nin redirect file " << path << std::endl;
    in_stream.close();
    exit(1);
  }
  in_stream.close();
}

void ZimCreatorFS::visitDirectory(const std::string& path)
{
  if (isVerbose())
    std::cout << "Visiting directory " << path << std::endl;

  DIR* directory;

  /* Open directory */
  directory = opendir(path.c_str());
  if (directory == NULL) {
    std::cerr << "zimwriterfs: unable to open directory " << path << std::endl;
    exit(1);
  }

  /* Read directory content */
  struct dirent* entry;
  while ((entry = readdir(directory)) != NULL) {
    std::string entryName = entry->d_name;

    /* Ignore this system navigation virtual directories */
    if (entryName == "." || entryName == "..")
      continue;

    std::string fullEntryName = path + '/' + entryName;

    switch (entry->d_type) {
      case DT_REG:
        addFile(fullEntryName);
        break;
      case DT_LNK:
        processSymlink(path, fullEntryName);
        break;
      case DT_DIR:
        visitDirectory(fullEntryName);
        break;
      case DT_BLK:
        std::cerr << "Unable to deal with " << fullEntryName
                  << " (this is a block device)" << std::endl;
        break;
      case DT_CHR:
        std::cerr << "Unable to deal with " << fullEntryName
                  << " (this is a character device)" << std::endl;
        break;
      case DT_FIFO:
        std::cerr << "Unable to deal with " << fullEntryName
                  << " (this is a named pipe)" << std::endl;
        break;
      case DT_SOCK:
        std::cerr << "Unable to deal with " << fullEntryName
                  << " (this is a UNIX domain socket)" << std::endl;
        break;
      case DT_UNKNOWN:
        struct stat s;
        if (stat(fullEntryName.c_str(), &s) == 0) {
          if (S_ISREG(s.st_mode)) {
            addFile(fullEntryName);
          } else if (S_ISDIR(s.st_mode)) {
            visitDirectory(fullEntryName);
          } else {
            std::cerr << "Unable to deal with " << fullEntryName
                      << " (no clue what kind of file it is - from stat())"
                      << std::endl;
          }
        } else {
          std::cerr << "Unable to stat " << fullEntryName << std::endl;
        }
        break;
      default:
        std::cerr << "Unable to deal with " << fullEntryName
                  << " (no clue what kind of file it is)" << std::endl;
        break;
    }
  }

  closedir(directory);
}

void ZimCreatorFS::addFile(const std::string& path)
{
  auto url = path.substr(directoryPath.size()+1);
  auto mimetype = getMimeTypeForFile(directoryPath, url);
  auto title = std::string{};
  zim::writer::Hints hints;

  std::shared_ptr<zim::writer::Item> item;
  if ( mimetype.find(mimeTextHtml) != std::string::npos
    || mimetype.find(mimeTextCss) != std::string::npos) {
    auto content = getFileContent(path);

    if (mimetype.find(mimeTextHtml) != std::string::npos) {
      hints[zim::writer::FRONT_ARTICLE] = 1;
      auto redirectUrl = parseAndAdaptHtml(content, title, url);
      if (!redirectUrl.empty()) {
        // This is a redirect.
        addRedirection(url, title, redirectUrl, hints);
        return;
      }
    } else {
      adaptCss(content, url);
    }

    item = zim::writer::StringItem::create(url, mimetype, title, hints, content);
  } else {
    item = std::make_shared<zim::writer::FileItem>(url, mimetype, title, hints, path);
  }
  addItem(item);
}

void ZimCreatorFS::processSymlink(const std::string& curdir, const std::string& symlink_path)
{
  /* #102 Links can be 3 different types:
   *  - dandling (not pointing to a valid file)
   *  - pointing to file but outside of 'directoryPath'
   *  - looped symlinks
   */
  char resolved[PATH_MAX];
  if (realpath(symlink_path.c_str(), resolved) != resolved) {
    // looping symlinks also fall here: Too many levels of symbolic links
    // It also handles dangling symlink: No such file or directory
    std::cerr << "Unable to resolve symlink " << symlink_path
              << ": " << strerror(errno) << std::endl;
    return;
  }

  if (isDirectory(resolved)) {
    std::cerr << "Skip symlink " << symlink_path
              << ": points to a directory" << std::endl;
    return;
  }

  if (strncmp(canonical_basedir.c_str(), resolved, canonical_basedir.size()) != 0
      || resolved[canonical_basedir.size()] != '/') {
    std::cerr << "Skip symlink " << symlink_path
              << ": points outside of HTML directory" << std::endl;
    return;
  }

  std::string source_url = symlink_path.substr(directoryPath.size() + 1);
  std::string target_url = std::string(resolved).substr(canonical_basedir.size() + 1);
  addRedirection(source_url, "", target_url);
}

inline std::string removeLocalTagAndParameters(const std::string& url)
{
  std::string retVal = url;
  std::size_t found;

  /* Remove URL arguments */
  found = retVal.find("?");
  if (found != std::string::npos) {
    retVal = retVal.substr(0, found);
  }

  /* Remove local tag */
  found = retVal.find("#");
  if (found != std::string::npos) {
    retVal = retVal.substr(0, found);
  }

  return retVal;
}

struct GumboOutputDestructor {
  GumboOutputDestructor(GumboOutput* output) : output(output) {}
  ~GumboOutputDestructor() { gumbo_destroy_output(&kGumboDefaultOptions, output); }
  GumboOutput* output;
};

std::string ZimCreatorFS::parseAndAdaptHtml(std::string& data, std::string& title, const std::string& url)
{
  GumboOutput* output = gumbo_parse(data.c_str());
  GumboOutputDestructor outputDestructor(output);
  GumboNode* root = output->root;

  /* Search the content of the <title> tag in the HTML */
  if (root->type == GUMBO_NODE_ELEMENT
      && root->v.element.children.length >= 2) {
    const GumboVector* root_children = &root->v.element.children;
    GumboNode* head = NULL;
    for (unsigned int i = 0; i < root_children->length; ++i) {
      GumboNode* child = (GumboNode*)(root_children->data[i]);
      if (child->type == GUMBO_NODE_ELEMENT
          && child->v.element.tag == GUMBO_TAG_HEAD) {
        head = child;
        break;
      }
    }

    if (head != NULL) {
      GumboVector* head_children = &head->v.element.children;
      for (unsigned int i = 0; i < head_children->length; ++i) {
        GumboNode* child = (GumboNode*)(head_children->data[i]);
        if (child->type == GUMBO_NODE_ELEMENT
            && child->v.element.tag == GUMBO_TAG_TITLE) {
          if (child->v.element.children.length == 1) {
            GumboNode* title_text
                = (GumboNode*)(child->v.element.children.data[0]);
            if (title_text->type == GUMBO_NODE_TEXT) {
              title = title_text->v.text.text;
              stripTitleInvalidChars(title);
            }
          }
        }
      }

      /* Detect if this is a HTML redirection (if no redirects TSV
         file specified)
       */
      std::string targetUrl;
      try {
        targetUrl = extractRedirectUrlFromHtml(head_children);
      } catch (std::string& error) {
        std::cerr << error << std::endl;
      }
      if (!targetUrl.empty()) {
        auto redirectUrl = computeAbsolutePath(url, decodeUrl(targetUrl));
        auto redirectUrlPath = directoryPath + "/" + redirectUrl;
        if (!fileExists(redirectUrlPath)) {
          throw std::runtime_error("'" + url + "' HTML redirection target path '"
                                   + redirectUrlPath + "' doesn't exist.");
        }
        return redirectUrl;
      }

      /* If no title, then compute one from the filename */
      if (title.empty()) {
        auto found = url.rfind("/");
        if (found != std::string::npos) {
          title = url.substr(found + 1);
          found = title.rfind(".");
          if (found != std::string::npos) {
            title = title.substr(0, found);
          }
        } else {
          title = url;
        }
        std::replace(title.begin(), title.end(), '_', ' ');
      }
    }
  }
  return "";
}

void ZimCreatorFS::adaptCss(std::string& data, const std::string& url) {
  /* Rewrite url() values in the CSS */
  size_t startPos = 0;
  size_t endPos = 0;
  std::string targetUrl;

  while ((startPos = data.find("url(", endPos))
         && startPos != std::string::npos) {

    /* URL delimiters */
    endPos = data.find(")", startPos);
    startPos = startPos + (data[startPos + 4] == '\''
                                   || data[startPos + 4] == '"'
                               ? 5
                               : 4);
    endPos = endPos - (data[endPos - 1] == '\''
                               || data[endPos - 1] == '"'
                           ? 1
                           : 0);
    targetUrl = data.substr(startPos, endPos - startPos);
    std::string startDelimiter = data.substr(startPos - 1, 1);
    std::string endDelimiter = data.substr(endPos, 1);

    if (targetUrl.substr(0, 5) == "data:") {
      continue;
    }

    /* Deal with URL with arguments (using '? ') */
    std::string path = targetUrl;
    size_t markPos = targetUrl.find("?");
    if (markPos != std::string::npos) {
      path = targetUrl.substr(0, markPos);
    }

    /* Embeded fonts need to be inline because Kiwix is
       otherwise not able to load same because of the
       same-origin security */
    std::string mimeType = getMimeTypeForFile(directoryPath, path);
    if (mimeType == "application/font-ttf"
        || mimeType == "application/font-woff"
        || mimeType == "application/font-woff2"
        || mimeType == "application/vnd.ms-opentype"
        || mimeType == "application/vnd.ms-fontobject") {
      try {
        std::string fontContent = getFileContent(
            directoryPath + "/" + computeAbsolutePath(url, path));
        replaceStringInPlaceOnce(
            data,
            startDelimiter + targetUrl + endDelimiter,
            startDelimiter + "data:" + mimeType + ";base64,"
                + base64_encode(reinterpret_cast<const unsigned char*>(
                                    fontContent.c_str()),
                                fontContent.length())
                + endDelimiter);
      } catch (...) {
      }
    }
  }
}
