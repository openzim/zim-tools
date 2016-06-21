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

#ifndef OPENZIM_ZIMWRITERFS_ARTICLESOURCE_H
#define OPENZIM_ZIMWRITERFS_ARTICLESOURCE_H

#include <string>
#include <queue>
#include <fstream>
#include "queue.h"

#include <zim/writer/zimcreator.h>

class ArticleSource : public zim::writer::ArticleSource {
  public:
    explicit ArticleSource(Queue<std::string>& filenameQueue);
    virtual const zim::writer::Article* getNextArticle();
    virtual zim::Blob getData(const std::string& aid);
    virtual std::string getMainPage();
    
    virtual void init_redirectsQueue_from_file(const std::string& path);
    
  private:
    std::queue<std::string> metadataQueue;
    std::queue<std::string> redirectsQueue;
    Queue<std::string>&     filenameQueue;
};

#endif //OPENZIM_ZIMWRITERFS_ARTICLESOURCE_H
