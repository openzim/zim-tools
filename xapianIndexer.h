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

#ifndef OPENZIM_ZIMWRITERFS_XAPIANINDEXER_H
#define OPENZIM_ZIMWRITERFS_XAPIANINDEXER_H


#include "indexer.h"
#include "articlesource.h"
#include "article.h"

#include <xapian.h>
#include "xapian/myhtmlparse.h"
#include <zim/blob.h>

class XapianIndexer;

class XapianMetaArticle : public Article {
    private:
        XapianIndexer* indexer;
        mutable std::string data;
    public:
        XapianMetaArticle(XapianIndexer* indexer):
            indexer(indexer)
        {
            ns = 'Z';
            aid = url = "/Z/fulltextIndex/xapian";
            title = "Xapian Fulltext Index";
            mimeType = "application/octet-stream+xapian";
        };
        virtual zim::Blob getData() const;
};

class XapianIndexer : public Indexer, public IHandler {
    public:
        XapianIndexer(const std::string& language, bool verbose);
        std::string getIndexPath() { return indexPath; }

    protected:
        void indexingPrelude(const string indexPath);
        void index(const string &url,
                   const string &title,
                   const string &unaccentedTitle,
                   const string &keywords,
                   const string &content,
                   const string &snippet,
                   const string &size,
                   const string &wordCount);
        void flush();
        void indexingPostlude();
        void handleArticle(Article* article);
        XapianMetaArticle* getMetaArticle();
        zim::Blob getData();

        Xapian::WritableDatabase writableDatabase;
        Xapian::Stem stemmer;
        Xapian::SimpleStopper stopper;
        Xapian::TermGenerator indexer;
        std::string indexPath;
};

#endif // OPENZIM_ZIMWRITERFS_XAPIANINDEXER_H
