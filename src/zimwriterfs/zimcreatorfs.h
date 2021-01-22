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

#ifndef OPENZIM_ZIMWRITERFS_ZIMCREATORFS_H
#define OPENZIM_ZIMWRITERFS_ZIMCREATORFS_H

#include <vector>
#include <string>

#include <zim/writer/creator.h>

class IHandler
{
 public:
  virtual void handleItem(std::shared_ptr<zim::writer::Item> item) = 0;
  virtual std::string getName() const = 0;
  virtual std::string getData() const = 0;
  virtual ~IHandler() = default;
};

class ZimCreatorFS : public zim::writer::Creator
{
 public:
  ZimCreatorFS(std::string _directoryPath);
  virtual ~ZimCreatorFS() = default;

  virtual void add_customHandler(IHandler* handler);
  virtual void add_redirectArticles_from_file(const std::string& path);
  virtual void visitDirectory(const std::string& path);

  virtual void addFile(const std::string& path);
  virtual void addItem(std::shared_ptr<zim::writer::Item> item);
  virtual void finishZimCreation();

  void processSymlink(const std::string& curdir, const std::string& symlink_path);
  const std::string & basedir() const { return directoryPath; }
  const std::string & canonicalBaseDir() const { return canonical_basedir; }
  std::string parseAndAdaptHtml(std::string& data, std::string& title, const std::string& url);
  void adaptCss(std::string& data, const std::string& url);

  void addMetadata(const std::string& key, const std::string& content) {
    if ( !content.empty() ) {
      zim::writer::Creator::addMetadata(key, content);
    }
  }

 private:
  std::vector<IHandler*> itemHandlers;
  std::string directoryPath;  ///< html dir without trailing slash
  std::string canonical_basedir;
};

#endif  // OPENZIM_ZIMWRITERFS_ARTICLESOURCE_H
