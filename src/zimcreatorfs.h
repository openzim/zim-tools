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
  virtual void handleArticle(const zim::writer::Article& article) = 0;
  virtual zim::writer::Article* getMetaArticle() = 0;
  virtual ~IHandler() = default;
};

class ZimCreatorFS : public zim::writer::Creator
{
 public:
  ZimCreatorFS(std::string mainPage, bool verbose)
    : zim::writer::Creator(verbose),
      mainPage(mainPage) {}
  virtual ~ZimCreatorFS() = default;
  virtual std::string getMainPage();
   virtual void add_customHandler(IHandler* handler);
   virtual void add_redirectArticles_from_file(const std::string& path);
   virtual void visitDirectory(const std::string& path);

   virtual void addArticle(const std::string& path);
   virtual void addArticle(const zim::writer::Article& article);
   virtual void finishZimCreation();

 private:
  std::vector<IHandler*> articleHandlers;
  std::string mainPage;
};

#endif  // OPENZIM_ZIMWRITERFS_ARTICLESOURCE_H
