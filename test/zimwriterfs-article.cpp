/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * is provided AS IS, WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, and
 * NON-INFRINGEMENT.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */

#include <iostream>
#include <magic.h>

#include "../src/zimwriterfs/article.h"
#include "gtest/gtest.h"
#include "../src/tools.h"

// stub from zimwriterfs.cpp
std::string directoryPath;  // html dir without trailing slash
bool isVerbose() { return false; }
bool inflateHtmlFlag = false;
magic_t magic;


TEST(ArticleTest, SimpleMetadata)
{
  std::string t = "Example content";
  SimpleMetadataArticle article("Title", t);

  // test zim::writer::Article interface
  EXPECT_EQ(article.getUrl(), zim::writer::Url('M', "Title"));
  EXPECT_EQ(article.getTitle(), "");
  ASSERT_FALSE(article.isRedirect());
  ASSERT_FALSE(article.isLinktarget());
  ASSERT_FALSE(article.isDeleted());
  EXPECT_EQ(article.getMimeType(), "text/plain");
  ASSERT_TRUE(article.shouldCompress());
  ASSERT_FALSE(article.shouldIndex());
  EXPECT_EQ(article.getRedirectUrl(), zim::writer::Url());
  EXPECT_EQ(article.getSize(), t.size());
  ASSERT_EQ(article.getData(), zim::Blob(t.data(), t.size()));
  EXPECT_EQ(article.getFilename(), "");
}


TEST(ArticleTest, MetadataFaviconArticle)
{
  std::string fn = "favicon.png";

  MetadataFaviconArticle article("I/" + fn);

  // test zim::writer::Article interface
  EXPECT_EQ(article.getUrl(), zim::writer::Url('-', "favicon"));
  EXPECT_EQ(article.getTitle(), "");
  ASSERT_TRUE(article.isRedirect());
  EXPECT_FALSE(article.isLinktarget());
  EXPECT_FALSE(article.isDeleted());
  ASSERT_EQ(article.getMimeType(), "image/png");
  EXPECT_FALSE(article.shouldCompress());
  EXPECT_FALSE(article.shouldIndex());
  EXPECT_EQ(article.getRedirectUrl(), zim::writer::Url('I', fn));
  ASSERT_EQ(article.getSize(), 0);
  ASSERT_EQ(article.getData(), zim::Blob());
  EXPECT_EQ(article.getFilename(), "");
}

TEST(ArticleTest, MetadataDate)
{
  MetadataDateArticle article;

  // test zim::writer::Article interface
  EXPECT_EQ(article.getUrl(), zim::writer::Url('M', "Date"));
  EXPECT_EQ(article.getTitle(), "");
  ASSERT_FALSE(article.isRedirect());
  ASSERT_FALSE(article.isLinktarget());
  ASSERT_FALSE(article.isDeleted());
  EXPECT_EQ(article.getMimeType(), "text/plain");
  ASSERT_TRUE(article.shouldCompress());
  ASSERT_FALSE(article.shouldIndex());
  EXPECT_EQ(article.getRedirectUrl(), zim::writer::Url());
  EXPECT_TRUE(article.getSize() > 8 && article.getSize() < 15); // date string is about 10 chars
  EXPECT_EQ(article.getFilename(), "");
}

TEST(ArticleTest, FileArticlePng)
{
  directoryPath = "data/minimal-content";
  std::string fn = "favicon.png";
  unsigned int size = getFileSize(directoryPath + "/" + fn);
  std::string data = getFileContent(directoryPath + "/" + fn);

  FileArticle article(directoryPath + "/" + fn, false);

  // test zim::writer::Article interface
  EXPECT_EQ(article.getUrl(), zim::writer::Url('I', fn));
  EXPECT_EQ(article.getTitle(), "");
  ASSERT_FALSE(article.isRedirect());
  EXPECT_FALSE(article.isLinktarget());
  EXPECT_FALSE(article.isDeleted());
  ASSERT_EQ(article.getMimeType(), "image/png");
  EXPECT_FALSE(article.shouldCompress());
  EXPECT_FALSE(article.shouldIndex());
  EXPECT_EQ(article.getRedirectUrl(), zim::writer::Url());
  ASSERT_EQ(article.getSize(), size);

  // see FileArticle::getFilename()
  // after file content is read, getFilename() no more returns the filename
  EXPECT_EQ(article.getFilename(), directoryPath + "/" + fn);
  ASSERT_EQ(article.getData(), zim::Blob(data.data(), data.size()));
  EXPECT_EQ(article.getFilename(), "");
  // file size still should be returned:
  EXPECT_EQ(article.getSize(), size);
}

TEST(ArticleTest, FileArticleHTML)
{
  directoryPath = "data/minimal-content";
  std::string fn = "hello.html";
  unsigned int size = getFileSize(directoryPath + "/" + fn);
  std::string data = getFileContent(directoryPath + "/" + fn);

  FileArticle article(directoryPath + "/" + fn, false);

  // see FileArticle::getFilename() and the constructor
  // because HTML content are read right away, getFilename() always returns empty string
  EXPECT_EQ(article.getFilename(), "");

  // test zim::writer::Article interface
  EXPECT_EQ(article.getUrl(), zim::writer::Url('A', fn));
  EXPECT_EQ(article.getTitle(), "HTML title tag content");
  ASSERT_FALSE(article.isRedirect());
  EXPECT_FALSE(article.isLinktarget());
  EXPECT_FALSE(article.isDeleted());
  ASSERT_EQ(article.getMimeType(), "text/html");
  EXPECT_TRUE(article.shouldCompress());
  EXPECT_TRUE(article.shouldIndex());
  EXPECT_EQ(article.getRedirectUrl(), zim::writer::Url());
  ASSERT_EQ(article.getSize(), size);
  ASSERT_EQ(article.getData(), zim::Blob(data.data(), data.size()));
  EXPECT_EQ(article.getFilename(), "");
}

TEST(ArticleTest, RedirectArticle)
{
  RedirectArticle article('A', "index.html", "Start page", zim::writer::Url("A/home.html"));

  // test zim::writer::Article interface
  EXPECT_EQ(article.getUrl(), zim::writer::Url('A', "index.html"));
  EXPECT_EQ(article.getTitle(), "Start page");
  ASSERT_TRUE(article.isRedirect());
  EXPECT_FALSE(article.isLinktarget());
  EXPECT_FALSE(article.isDeleted());
  ASSERT_EQ(article.getMimeType(), "text/html");

  // FIXME: maybe this is shouldn't be that way:
  EXPECT_TRUE(article.shouldCompress());
  EXPECT_TRUE(article.shouldIndex());

  EXPECT_EQ(article.getRedirectUrl(), zim::writer::Url('A', "home.html"));
  ASSERT_EQ(article.getSize(), 0);
  ASSERT_EQ(article.getData(), zim::Blob());
  EXPECT_EQ(article.getFilename(), "");
}
