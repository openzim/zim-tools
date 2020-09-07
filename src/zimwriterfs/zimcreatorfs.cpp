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

bool isVerbose();

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
      case DT_LNK:
        {
          addArticle(fullEntryName);
        }
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
