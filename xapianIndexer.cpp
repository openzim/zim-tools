/*
 * Copyright 2011 Emmanuel Engelhart <kelson@kiwix.org>
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

#include "xapianIndexer.h"
#include "tools.h"

#include <unistd.h>

/* Constructor */
XapianIndexer::XapianIndexer(const std::string& language, const bool verbose) {
    setVerboseFlag(verbose);
    readStopWords(language);
  /*
  stemmer(Xapian::Stem("french")) {
  this->indexer.set_stemmer(this->stemmer);
  */
}

void XapianIndexer::indexingPrelude(const string indexPath_) {
    indexPath = indexPath_;
    this->writableDatabase = Xapian::WritableDatabase(indexPath + ".tmp", Xapian::DB_CREATE_OR_OVERWRITE);
    this->writableDatabase.begin_transaction(true);

    /* Insert the stopwords */
    if (!this->stopWords.empty()) {
      std::vector<std::string>::iterator it = this->stopWords.begin();
      for( ; it != this->stopWords.end(); ++it) {
	this->stopper.add(*it);
      }

      this->indexer.set_stopper(&(this->stopper));
    }
}

void XapianIndexer::index(const string &url,
                          const string &title,
                          const string &unaccentedTitle,
                          const string &keywords,
                          const string &content,
                          const string &snippet,
                          const string &size,
                          const string &wordCount) {

    /* Put the data in the document */
    Xapian::Document currentDocument;
    currentDocument.clear_values();
    currentDocument.add_value(0, title);
    currentDocument.add_value(1, snippet);
    currentDocument.add_value(2, size);
    currentDocument.add_value(3, wordCount);
    currentDocument.set_data(url);
    indexer.set_document(currentDocument);

    /* Index the title */
    if (!unaccentedTitle.empty()) {
      this->indexer.index_text_without_positions(unaccentedTitle, this->getTitleBoostFactor(content.size()));
    }

    /* Index the keywords */
    if (!keywords.empty()) {
      this->indexer.index_text_without_positions(keywords, keywordsBoostFactor);
    }

    /* Index the content */
    if (!content.empty()) {
      this->indexer.index_text_without_positions(content);
    }

    /* add to the database */
    this->writableDatabase.add_document(currentDocument);
}

void XapianIndexer::flush() {
    this->writableDatabase.commit_transaction();
    this->writableDatabase.begin_transaction(true);
}

void XapianIndexer::indexingPostlude() {
    this->flush();
    this->writableDatabase.commit_transaction();
    this->writableDatabase.commit();
    this->writableDatabase.compact(indexPath, Xapian::DBCOMPACT_SINGLE_FILE);

    // commit is not available is old version of xapian and seems not mandatory there
    // this->writableDatabase.commit();
}

void XapianIndexer::handleArticle(Article* article)
{
    indexerToken token;
    size_t found;
    MyHtmlParser htmlParser;

    if ( article->isRedirect() || article->getMimeType().find("text/html") != 0 )
        return;

    token.title = article->getTitle();
    token.url = article->getUrl();
    zim::Blob article_content = article->getData();
    token.content = std::string(article_content.data(), article_content.size());

    /* The parser generate a lot of exceptions which should be avoided */
    try {
        htmlParser.parse_html(token.content, "UTF-8", true);
    } catch (...) {
    }

    /* If content does not have the noindex meta tag */
    /* Seems that the parser generates an exception in such case */
    found = htmlParser.dump.find("NOINDEX");

    if (found == string::npos) {
	/* Get the accented title */
	token.accentedTitle = (htmlParser.title.empty() ? token.title : htmlParser.title);

	/* count words */
	stringstream countWordStringStream;
	countWordStringStream << countWords(htmlParser.dump);
	token.wordCount = countWordStringStream.str();

	/* snippet */
	std::string snippet = std::string(htmlParser.dump, 0, 300);
	std::string::size_type last = snippet.find_last_of('.');
	if (last == snippet.npos)
	  last = snippet.find_last_of(' ');
	if (last != snippet.npos)
	  snippet = snippet.substr(0, last);
	token.snippet = snippet;

	/* size */
	stringstream sizeStringStream;
	sizeStringStream << token.content.size() / 1024;
	token.size = sizeStringStream.str();

	/* Remove accent */
	token.title = removeAccents(token.accentedTitle);
	token.keywords = removeAccents(htmlParser.keywords);
	token.content = removeAccents(htmlParser.dump);
	pushToIndexQueue(token);
    }
}

XapianMetaArticle* XapianIndexer::getMetaArticle()
{
     return new XapianMetaArticle(this);
}

zim::Blob XapianMetaArticle::getData() const
{
    if ( data.size() == 0 )
    {
        indexerToken token;
        indexer->pushToIndexQueue(token);
        /* Wait it index everything */
        int wait = 500;
        while ( indexer->isRunning() )
        {
            usleep(wait);
        }
        data = getFileContent(indexer->getIndexPath());
    }
    return zim::Blob(data.data(), data.size());
}

