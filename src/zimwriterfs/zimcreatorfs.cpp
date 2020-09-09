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
