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
#include "../tools.h"
#include "tools.h"

#include <fstream>
#include <dirent.h>
#include <sys/stat.h>
#include <regex>

bool isVerbose();

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

    auto path = matches[1].str() + "/" +  matches[2].str();
    auto title = matches[3].str();
    auto redirectUrl = matches[4].str();
    addRedirection(path, title, redirectUrl);
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
          addFile(fullEntryName);
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
  auto url = path.substr(rootDir.size()+1);
  auto mimetype = getMimeTypeForFile(url);
  auto ns = getNamespaceForMimeType(mimetype);
  auto itemPath = ns + "/" + url;
  auto title = std::string{};

  std::unique_ptr<zim::writer::Item> item;
  if ( mimetype.find("text/html") != std::string::npos
    || mimetype.find("text/css") != std::string::npos) {
    auto content = getFileContent(path);

    if (mimetype.find("text/html") != std::string::npos) {
      auto redirectUrl = parseAndAdaptHtml(content, title, ns[0], url, detectRedirects);
      if (!redirectUrl.empty()) {
        // This is a redirect.
        addRedirection(itemPath, title, redirectUrl);
        return;
      }
    } else {
      adaptCss(content, ns[0], url);
    }
    item.reset(new zim::writer::StringItem(itemPath, mimetype, title, content));
  } else {
    item.reset(new zim::writer::FileItem(itemPath, mimetype, title, path));
  }
  addItem(std::move(item));
}

void ZimCreatorFS::addItem(std::shared_ptr<zim::writer::Item> item)
{
  Creator::addItem(item);
  for (auto& handler: itemHandlers) {
      handler->handleItem(item);
  }
}

void ZimCreatorFS::finishZimCreation()
{
  for(auto& handler: itemHandlers) {
    Creator::addMetadata(handler->getName(), handler->getData());
  }
  Creator::finishZimCreation();
}

void ZimCreatorFS::add_customHandler(IHandler* handler)
{
  itemHandlers.push_back(handler);
}
