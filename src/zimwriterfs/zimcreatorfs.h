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

#include <queue>
#include <string>
#include "article.h"

#include <zim/writer/creator.h>

class IHandler
{
 public:
  virtual void handleArticle(std::shared_ptr<zim::writer::Article> article) = 0;
  virtual std::shared_ptr<zim::writer::Article> getMetaArticle() = 0;
  virtual ~IHandler() = default;
};

class ZimCreatorFS : public zim::writer::Creator
{
 public:
  ZimCreatorFS(std::string _directoryPath, std::string mainPage, bool verbose, bool uniqueNamespace)
    : zim::writer::Creator(verbose),
      directoryPath(_directoryPath),
      mainPage(mainPage),
      uniqueNamespace(uniqueNamespace){}
  virtual ~ZimCreatorFS() = default;

  virtual zim::writer::Url getMainUrl() const;
  virtual void add_customHandler(IHandler* handler);
  virtual void add_redirectArticles_from_file(const std::string& path);
  virtual void visitDirectory(const std::string& path);

  virtual void addMetadata(const std::string& metadata, const std::string& content);
  virtual void addArticle(const std::string& path);
  virtual void addArticle(std::shared_ptr<zim::writer::Article> article);
  virtual void finishZimCreation();

  std::string computeNewUrl(const std::string& aid, const std::string& baseUrl, const std::string& targetUrl) const;
  const std::string & basedir() const { return directoryPath; }
  bool uniqNamespace() const { return uniqueNamespace; }

 private:
  std::vector<IHandler*> articleHandlers;
  std::string directoryPath;  ///< html dir without trailing slash
  std::string mainPage;
  bool uniqueNamespace;
};

#endif  // OPENZIM_ZIMWRITERFS_ARTICLESOURCE_H
