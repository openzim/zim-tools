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
#include "resourceTools.h"

/* Constructor */
XapianIndexer::XapianIndexer(const std::string& language, const bool verbose) :
    language(language)
{
    setVerboseFlag(verbose);

    /* Build ICU Local object to retrieve ISO-639 language code (from
       ISO-639-3) */
    icu::Locale languageLocale(language.c_str());

    /* Configuring language base steemming */
    try {
        this->stemmer = Xapian::Stem(languageLocale.getLanguage());
        this->indexer.set_stemmer(this->stemmer);
        this->indexer.set_stemming_strategy(Xapian::TermGenerator::STEM_ALL);
    } catch (...) {
        std::cout << "No steemming for language '" << languageLocale.getLanguage() << "'" << std::endl;
    }

     /* Read the stopwords */
    std::string stopWord;
    this->stopwords = getResourceAsString("stopwords/"+language);
    std::istringstream file(this->stopwords);
    while (std::getline(file, stopWord, '\n')) {
        this->stopper.add(stopWord);
    }

    this->indexer.set_stopper(&(this->stopper));
    this->indexer.set_stopper_strategy(Xapian::TermGenerator::STOP_ALL);
}

XapianIndexer::~XapianIndexer(){
  if (!indexPath.empty()) {
    try {
      remove_all(indexPath + ".tmp");
      remove_all(indexPath);
    } catch(...) {
      /* Do not raise */
    }
  }
}

void XapianIndexer::indexingPrelude(const string indexPath_) {
    indexPath = indexPath_;
    this->writableDatabase = Xapian::WritableDatabase(indexPath + ".tmp", Xapian::DB_CREATE_OR_OVERWRITE);
    this->writableDatabase.set_metadata("valuesmap", "title:0;wordcount:1");
    this->writableDatabase.set_metadata("language", language);
    this->writableDatabase.set_metadata("stopwords", stopwords);
    this->writableDatabase.begin_transaction(true);
}

void XapianIndexer::index(const string &url,
                          const string &title,
                          const string &unaccentedTitle,
                          const string &keywords,
                          const string &content,
                          const string &wordCount) {

    /* Put the data in the document */
    Xapian::Document currentDocument;
    currentDocument.clear_values();
    currentDocument.add_value(0, title);
    currentDocument.add_value(1, wordCount);
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
}

void XapianIndexer::handleArticle(Article* article)
{
    indexerToken token;
    size_t found;
    MyHtmlParser htmlParser;

    if ( article->isRedirect() || article->getMimeType().find("text/html") != 0 )
        return;

    token.title = article->getTitle();
    token.url = std::string(1, article->getNamespace()) + '/' + article->getUrl();
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

