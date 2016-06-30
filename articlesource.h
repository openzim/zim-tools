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
#include <zim/blob.h>

class Article;

class IHandler
{
    public:
        virtual void handleArticle(Article* article) = 0;
        virtual Article* getMetaArticle() = 0;
};

class ArticleSource : public zim::writer::ArticleSource {
  public:
    explicit ArticleSource(Queue<std::string>& filenameQueue);
    void add_metadataArticle(Article* article);
    virtual const zim::writer::Article* getNextArticle();
    virtual std::string getMainPage();
    virtual void add_customHandler(IHandler* handler);
    
    virtual void init_redirectsQueue_from_file(const std::string& path);
    
  private:
    std::queue<Article*>    metadataQueue;
    std::queue<std::string> redirectsQueue;
    Queue<std::string>&     filenameQueue;
    std::vector<IHandler*>  articleHandlers;
    std::vector<IHandler*>::iterator currentLoopHandler;
    bool                    loopOverHandlerStarted;
};

#endif //OPENZIM_ZIMWRITERFS_ARTICLESOURCE_H
