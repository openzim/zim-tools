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
#include "article.h"
#include "../tools.h"

#include <fstream>
#include <dirent.h>
#include <sys/stat.h>
#include <regex>
#include <unistd.h>
#include <limits.h>
#include <cassert>

bool isVerbose();

ZimCreatorFS::ZimCreatorFS(std::string _directoryPath, std::string mainPage,
                           bool verbose, bool uniqueNamespace, bool zstd)
  : zim::writer::Creator(verbose, zstd ? zim::zimcompZstd : zim::zimcompLzma),
    directoryPath(_directoryPath),
    mainPage(mainPage),
    uniqueNamespace(uniqueNamespace)
{
  char buf[PATH_MAX];

  if (realpath(directoryPath.c_str(), buf) != buf) {
    throw std::invalid_argument(
          Formatter() << "Unable to canonicalize HTML directory path "
                      << directoryPath << ": " << strerror(errno));
  }

  canonical_basedir = buf;
}

zim::writer::Url ZimCreatorFS::getMainUrl() const
{
  return zim::writer::Url('A', mainPage);
}

void ZimCreatorFS::add_redirectArticles_from_file(const std::string& path)
{
  std::ifstream in_stream;
  std::string line;

  in_stream.open(path.c_str());
  int line_number = 1;
  while (std::getline(in_stream, line)) {
    std::regex line_regex("(.)\\t(.+)\\t(.+)\\t(.+)");
    std::smatch matches;
    if (!std::regex_search(line, matches, line_regex) || matches.size() != 5) {
      std::cerr << "zimwriterfs: line #" << line_number
                << " has invalid format in redirect file " << path << ": '"
                << line << "'" << std::endl;
      in_stream.close();
      exit(1);
    }

    auto redirectArticle = std::make_shared<RedirectArticle>(
      this,
      matches[1].str()[0],  // ns
      matches[2].str(),     // URL
      matches[3].str(),     // title
      matches[4].str());    // redirect URL
    addArticle(redirectArticle);
    ++line_number;
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
        addArticle(fullEntryName);
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
            addArticle(fullEntryName);
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

void ZimCreatorFS::addMetadata(const std::string& title, const std::string& content)
{
  auto article = std::make_shared<SimpleMetadataArticle>(title, content);
  addArticle(article);
}

void ZimCreatorFS::addArticle(const std::string& path)
{
  auto farticle = std::make_shared<FileArticle>(this, path);
  if (farticle->isInvalid()) {
    return;
  }
  addArticle(farticle);
}

void ZimCreatorFS::addArticle(std::shared_ptr<zim::writer::Article> article)
{
  Creator::addArticle(article);
  for (auto& handler: articleHandlers) {
      handler->handleArticle(article);
  }
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
  std::string source_mimeType = getMimeTypeForFile(directoryPath, source_url);
  const char source_ns = getNamespaceForMimeType(source_mimeType, uniqNamespace())[0];

  std::string target_url = std::string(resolved).substr(canonical_basedir.size() + 1);
  std::string target_mimeType = getMimeTypeForFile(directoryPath, target_url);
  const char target_ns = getNamespaceForMimeType(target_mimeType, uniqNamespace())[0];

  std::shared_ptr<zim::writer::Article> redirect_article(
        new RedirectArticle(this, source_ns, source_url, "",
                            zim::writer::Url(target_ns, target_url)));
  addArticle(redirect_article);
}

void ZimCreatorFS::finishZimCreation()
{
  for(auto& handler: articleHandlers) {
    Creator::addArticle(handler->getMetaArticle());
  }
  Creator::finishZimCreation();
}

void ZimCreatorFS::add_customHandler(IHandler* handler)
{
  articleHandlers.push_back(handler);
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

std::string ZimCreatorFS::computeNewUrl(const std::string& aid, const std::string& baseUrl, const std::string& targetUrl) const
{
  std::string filename = computeAbsolutePath(aid, targetUrl);
  std::string targetMimeType
      = getMimeTypeForFile(directoryPath, decodeUrl(removeLocalTagAndParameters(filename)));
  std::string newUrl
      = "/" + getNamespaceForMimeType(targetMimeType, uniqNamespace()) + "/" + filename;
  return computeRelativePath(baseUrl, newUrl);
}

void ZimCreatorFS::parseAndAdaptHtml(bool detectRedirects)
{
  GumboOutput* output = gumbo_parse(data.c_str());
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

      /* Detect if this is a redirection (if no redirects TSV file specified)
       */
      std::string targetUrl;
      try {
        targetUrl = detectRedirects
                        ? extractRedirectUrlFromHtml(head_children)
                        : "";
      } catch (std::string& error) {
        std::cerr << error << std::endl;
      }
      if (!targetUrl.empty()) {
        auto redirectUrl = computeAbsolutePath(url, decodeUrl(targetUrl));
        if (!fileExists(creator->basedir() + "/" + redirectUrl)) {
          redirectUrl.clear();
          invalid = true;
        } else {
          this->redirectUrl = zim::writer::Url(redirectUrl);
        }
      }

      /* If no title, then compute one from the filename */
      if (title.empty()) {
        auto path = _getFilename();
        auto found = path.rfind("/");
        if (found != std::string::npos) {
          title = path.substr(found + 1);
          found = title.rfind(".");
          if (found != std::string::npos) {
            title = title.substr(0, found);
          }
        } else {
          title = path;
        }
        std::replace(title.begin(), title.end(), '_', ' ');
      }
    }
  }

  /* Update links in the html to let them still be valid */
  std::map<std::string, bool> links;
  getLinks(root, links);
  std::string longUrl = std::string("/") + ns + "/" + url;

  /* If a link appearch to be duplicated in the HTML, it will
     occurs only one time in the links variable */
  for (auto& linkPair: links) {
    auto target = linkPair.first;
    if (!target.empty() && target[0] != '#' && target[0] != '?'
        && target.substr(0, 5) != "data:") {
      replaceStringInPlace(data,
                           "\"" + target + "\"",
                           "\"" + creator->computeNewUrl(url, longUrl, target) + "\"");
    }
  }
  gumbo_destroy_output(&kGumboDefaultOptions, output);
}

void ZimCreatorFS::adaptCss()
{
  /* Rewrite url() values in the CSS */
  size_t startPos = 0;
  size_t endPos = 0;
  std::string url;
  std::string longUrl = std::string("/") + ns + "/" + this->url;

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
    url = data.substr(startPos, endPos - startPos);
    std::string startDelimiter = data.substr(startPos - 1, 1);
    std::string endDelimiter = data.substr(endPos, 1);

    if (url.substr(0, 5) != "data:") {

      /* Deal with URL with arguments (using '? ') */
      std::string path = url;
      size_t markPos = url.find("?");
      if (markPos != std::string::npos) {
        path = url.substr(0, markPos);
      }

      /* Embeded fonts need to be inline because Kiwix is
         otherwise not able to load same because of the
         same-origin security */
      std::string mimeType = getMimeTypeForFile(creator->basedir(), path);
      if (mimeType == "application/font-ttf"
          || mimeType == "application/font-woff"
          || mimeType == "application/font-woff2"
          || mimeType == "application/vnd.ms-opentype"
          || mimeType == "application/vnd.ms-fontobject") {
        try {
          std::string fontContent = getFileContent(
              creator->basedir() + "/" + computeAbsolutePath(this->url, path));
          replaceStringInPlaceOnce(
              data,
              startDelimiter + url + endDelimiter,
              startDelimiter + "data:" + mimeType + ";base64,"
                  + base64_encode(reinterpret_cast<const unsigned char*>(
                                      fontContent.c_str()),
                                  fontContent.length())
                  + endDelimiter);
        } catch (...) {
        }
      } else {
        /* Deal with URL with arguments (using '? ') */
        if (markPos != std::string::npos) {
          endDelimiter = url.substr(markPos, 1);
        }

        replaceStringInPlaceOnce(
            data,
            startDelimiter + url + endDelimiter,
            startDelimiter + creator->computeNewUrl(this->url, longUrl, path) + endDelimiter);
      }
    }
  }
}
