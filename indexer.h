/*
 * Copyright 2014 Emmanuel Engelhart <kelson@kiwix.org>
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

#ifndef OPENZIM_ZIMWRITERFS_INDEXER_H
#define OPENZIM_ZIMWRITERFS_INDEXER_H

#include <string>
#include <vector>
#include <stack>
#include <queue>
#include <fstream>
#include <iostream>
#include <sstream>

#include <pthread.h>
#include <zim/file.h>
#include <zim/article.h>
#include <zim/fileiterator.h>
#include <zim/writer/zimcreator.h>

using namespace std;

struct indexerToken {
    string url;
    string accentedTitle;
    string title;
    string keywords;
    string content;
    string wordCount;
};

class Indexer {

    public:
        Indexer();
        virtual ~Indexer();

    bool start(const string indexPath);
    bool stop();
    bool isRunning();
    void setVerboseFlag(const bool value);
    void pushToIndexQueue(indexerToken &token);

  protected:
    virtual void indexingPrelude(const string indexPath) = 0;
    virtual void index(const string &url,
		       const string &title,
		       const string &unaccentedTitle,
		       const string &keywords,
		       const string &content,
		       const string &wordCount) = 0;
    virtual void flush() = 0;
    virtual void indexingPostlude() = 0;

    /* Others */
    unsigned int countWords(const string &text);

    /* Boost factor */
    unsigned int keywordsBoostFactor;
    inline unsigned int getTitleBoostFactor(const unsigned int contentLength) {
      return contentLength / 500 + 1;
    }

    /* Verbose */
    pthread_mutex_t verboseMutex;
    bool getVerboseFlag();
    bool verboseFlag;

  private:
    pthread_mutex_t threadIdsMutex;

    /* Index writting */
    pthread_t articleIndexer;
    pthread_mutex_t articleIndexerRunningMutex;
    static void *indexArticles(void *ptr);
    bool articleIndexerRunningFlag;
    bool isArticleIndexerRunning();
    void articleIndexerRunning(bool value);

    /* To index queue */
    std::queue<indexerToken> toIndexQueue;
    pthread_mutex_t toIndexQueueMutex;
    /* void pushToIndexQueue(indexerToken &token); is public */
    bool popFromToIndexQueue(indexerToken &token);
    bool isToIndexQueueEmpty();

    /* Article Count & Progression */
    unsigned int articleCount;
    pthread_mutex_t articleCountMutex;
    void setArticleCount(const unsigned int articleCount);
    unsigned int getArticleCount();

    /* Index path */
    pthread_mutex_t indexPathMutex;
    string indexPath;
    void setIndexPath(const string path);
    string getIndexPath();

    /* ZIM id */
    pthread_mutex_t zimIdMutex;
    string zimId;
    void setZimId(const string id);
    string getZimId();
  };

#endif //OPENZIM_ZIMWRITERFS_INDEXER_H
