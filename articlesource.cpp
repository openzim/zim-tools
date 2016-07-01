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

#include "articlesource.h"
#include "article.h"
#include "tools.h"

#include <zim/blob.h>

#include <map>

bool isVerbose();

extern std::string welcome;


ArticleSource::ArticleSource(Queue<std::string>& filenameQueue):
    filenameQueue(filenameQueue),
    loopOverHandlerStarted(false)
{
}

void ArticleSource::init_redirectsQueue_from_file(const std::string& path){
    std::ifstream in_stream;
    std::string line;

    in_stream.open(path.c_str());
    while (std::getline(in_stream, line)) {
      redirectsQueue.push(line);
    }
    in_stream.close();
}

std::string ArticleSource::getMainPage() {
  return welcome;
}

Article *article = NULL;
const zim::writer::Article* ArticleSource::getNextArticle() {
  std::string path;

  if (article != NULL) {
    delete article;
    article = NULL;
  }

  if (!metadataQueue.empty()) {
    article = metadataQueue.front();
    metadataQueue.pop();
  } else if (!redirectsQueue.empty()) {
    std::string line = redirectsQueue.front();
    redirectsQueue.pop();
    article = new RedirectArticle(line);
  } else if (filenameQueue.popFromQueue(path)) {
    article = new FileArticle(path);
    while (article->isInvalid() && filenameQueue.popFromQueue(path)) {
      delete article;
      article = new FileArticle(path);
    };
    if (article->isInvalid()) {
      article = NULL;
    }
  }

  if (article == NULL) {
    if ( !loopOverHandlerStarted )
    {
        currentLoopHandler = articleHandlers.begin();
        loopOverHandlerStarted = true;
    } else {
        currentLoopHandler++;
    }
    if ( currentLoopHandler != articleHandlers.end() )
    {
        article = (*currentLoopHandler)->getMetaArticle();
    }
  }

  if (article != NULL)
  {
    for (std::vector<IHandler*>::iterator it = articleHandlers.begin();
         it != articleHandlers.end();
         ++it)
    {
      (*it)->handleArticle(article);
    }
  }

  return article;
}

void ArticleSource::add_customHandler(IHandler* handler)
{
    articleHandlers.push_back(handler);
}

void ArticleSource::add_metadataArticle(Article* article)
{
   metadataQueue.push(article);
}
