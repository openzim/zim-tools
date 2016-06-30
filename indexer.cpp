/*
 * Copyright 2011-2014 Emmanuel Engelhart <kelson@kiwix.org>
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

#include "indexer.h"
#include "resourceTools.h"
#include "pathTools.h"
#include <unistd.h>

  /* Count word */
  unsigned int Indexer::countWords(const string &text) {
    unsigned int numWords = 1;
    unsigned int length = text.size();

    for(unsigned int i=0; i<length;) {
      while(i<length && text[i] != ' ') {
	i++;
      }
      numWords++;
      i++;
    }

    return numWords;
  }

  /* Constructor */
  Indexer::Indexer() :
    keywordsBoostFactor(3),
    verboseFlag(false) {

    /* Initialize mutex */
    pthread_mutex_init(&threadIdsMutex, NULL);
    pthread_mutex_init(&toIndexQueueMutex, NULL);
    pthread_mutex_init(&articleIndexerRunningMutex, NULL);
    pthread_mutex_init(&articleCountMutex, NULL);
    pthread_mutex_init(&zimIdMutex, NULL);
    pthread_mutex_init(&indexPathMutex, NULL);
    pthread_mutex_init(&verboseMutex, NULL);
  }

  /* Destructor */
  Indexer::~Indexer() {
  }

  /* Read the stopwords */
  void Indexer::readStopWords(const string languageCode) {
    std::string stopWord;
    std::istringstream file(getResourceAsString("stopwords/" + languageCode));

    this->stopWords.clear();

    while (getline(file, stopWord, '\n')) {
      this->stopWords.push_back(stopWord);
    }
  }

  /* Article indexer methods */
  void *Indexer::indexArticles(void *ptr) {
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    Indexer *self = (Indexer *)ptr;
    unsigned int indexedArticleCount = 0;
    indexerToken token;

    self->indexingPrelude(self->getIndexPath());

    while (self->popFromToIndexQueue(token)) {
      self->index(token.url,
		  token.accentedTitle,
		  token.title,
		  token.keywords,
		  token.content,
		  token.snippet,
		  token.size,
		  token.wordCount
		  );

      indexedArticleCount += 1;

      /* Make a hard-disk flush every 10.000 articles */
      if (indexedArticleCount % 5000 == 0) {
	self->flush();
      }

      /* Test if the thread should be cancelled */
      pthread_testcancel();
    }
    self->indexingPostlude();

    /* Write content id file */
    string path = appendToDirectory(self->getIndexPath(), "content.id");
    writeTextFile(path, self->getZimId());

    usleep(100);

    self->articleIndexerRunning(false);
    pthread_exit(NULL);
    return NULL;
  }

  void Indexer::articleIndexerRunning(bool value) {
    pthread_mutex_lock(&articleIndexerRunningMutex);
    this->articleIndexerRunningFlag = value;
    pthread_mutex_unlock(&articleIndexerRunningMutex);
  }

  bool Indexer::isArticleIndexerRunning() {
    pthread_mutex_lock(&articleIndexerRunningMutex);
    bool retVal = this->articleIndexerRunningFlag;
    pthread_mutex_unlock(&articleIndexerRunningMutex);
    return retVal;
  }

  /* ToIndexQueue methods */
  bool Indexer::isToIndexQueueEmpty() {
    pthread_mutex_lock(&toIndexQueueMutex);
    bool retVal = this->toIndexQueue.empty();
    pthread_mutex_unlock(&toIndexQueueMutex);
    return retVal;
  }

  void Indexer::pushToIndexQueue(indexerToken &token) {
    pthread_mutex_lock(&toIndexQueueMutex);
    this->toIndexQueue.push(token);
    pthread_mutex_unlock(&toIndexQueueMutex);
    usleep(int(this->toIndexQueue.size() / 200) / 10 * 1000);
  }

  bool Indexer::popFromToIndexQueue(indexerToken &token) {
    while (this->isToIndexQueueEmpty()) {
      usleep(500);
      if (this->getVerboseFlag()) {
	std::cout << "Waiting... ToIndexQueue is empty for now..." << std::endl;
      }

      pthread_testcancel();
    }

    pthread_mutex_lock(&toIndexQueueMutex);
    token = this->toIndexQueue.front();
    this->toIndexQueue.pop();
    pthread_mutex_unlock(&toIndexQueueMutex);

    if (token.title == ""){
        //This is a empty token, end of the queue.
        return false;
    }
    return true;
  }

  /* Index methods */
  void Indexer::setIndexPath(const string path) {
    pthread_mutex_lock(&indexPathMutex);
    this->indexPath = path;
    pthread_mutex_unlock(&indexPathMutex);
  }

  string Indexer::getIndexPath() {
    pthread_mutex_lock(&indexPathMutex);
    string retVal = this->indexPath;
    pthread_mutex_unlock(&indexPathMutex);
    return retVal;
  }

  void Indexer::setArticleCount(const unsigned int articleCount) {
    pthread_mutex_lock(&articleCountMutex);
    this->articleCount = articleCount;
    pthread_mutex_unlock(&articleCountMutex);
  }

  unsigned int Indexer::getArticleCount() {
    pthread_mutex_lock(&articleCountMutex);
    unsigned int retVal = this->articleCount;
    pthread_mutex_unlock(&articleCountMutex);
    return retVal;
  }

  void Indexer::setZimId(const string id) {
    pthread_mutex_lock(&zimIdMutex);
    this->zimId = id;
    pthread_mutex_unlock(&zimIdMutex);
  }

  string Indexer::getZimId() {
    pthread_mutex_lock(&zimIdMutex);
    string retVal = this->zimId;
    pthread_mutex_unlock(&zimIdMutex);
    return retVal;
  }

  /* Manage */
  bool Indexer::start(const string indexPath) {
    if (this->getVerboseFlag()) {
      std::cout << "Indexing starting..." <<std::endl;
    }

    this->setArticleCount(0);
    this->setIndexPath(indexPath);

    pthread_mutex_lock(&threadIdsMutex);

    this->articleIndexerRunning(true);
    pthread_create(&(this->articleIndexer), NULL, Indexer::indexArticles, (void*)this);
    pthread_detach(this->articleIndexer);
    pthread_mutex_unlock(&threadIdsMutex);

    return true;
  }

  bool Indexer::isRunning() {
      if (this->getVerboseFlag()) {
	std::cout << "isArticleIndexer running: " << (this->isArticleIndexerRunning() ? "yes" : "no") << std::endl;
      }

    return this->isArticleIndexerRunning();
  }

  bool Indexer::stop() {
    if (this->isRunning()) {
      bool isArticleIndexerRunning = this->isArticleIndexerRunning();

      pthread_mutex_lock(&threadIdsMutex);

      if (isArticleIndexerRunning) {
	pthread_cancel(this->articleIndexer);
	this->articleIndexerRunning(false);
      }

      pthread_mutex_unlock(&threadIdsMutex);
    }

    return true;
  }

  /* Manage the verboseFlag */
  void Indexer::setVerboseFlag(const bool value) {
    pthread_mutex_lock(&verboseMutex);
    this->verboseFlag = value;
    pthread_mutex_unlock(&verboseMutex);
  }

  bool Indexer::getVerboseFlag() {
    bool value;
    pthread_mutex_lock(&verboseMutex);
    value = this->verboseFlag;
    pthread_mutex_unlock(&verboseMutex);
    return value;
  }
