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

#include <unistd.h>
#include <iostream>
#include <magic.h>

#include <zim/file.h>
#include <zim/fileiterator.h>

#include "gtest/gtest.h"

#include "../src/zimwriterfs/zimcreatorfs.h"
#include "../src/zimwriterfs/article.h"
#include "../src/tools.h"


// stub from zimwriterfs.cpp
bool inflateHtmlFlag = false;
bool isVerbose() { return false; }
magic_t magic;

class LibMagicInit
{
public:
  LibMagicInit()
  {
    if (! done) {
      magic = magic_open(MAGIC_MIME);
      magic_load(magic, NULL);
      done = true;
    }
  }
private:
  static bool done;
};

bool LibMagicInit::done = false;


class TempFile
{
public:
  TempFile(const char *name) { _name = "/tmp/"; _name += name; }
  ~TempFile() { unlink(_name.c_str()); }
  const char *path() { return _name.c_str(); }
private:
  std::string _name;
};


TEST(ZimCreatorFSTest, MinimalZim)
{
  LibMagicInit libmagic;

  std::string directoryPath = "data/minimal-content";
  ZimCreatorFS zimCreator(directoryPath, "index.html", false, false);

  TempFile out("minimal.zim");

  zimCreator.startZimCreation(out.path());
  zimCreator.visitDirectory(directoryPath);

  std::shared_ptr<zim::writer::Article> redirect_article(new RedirectArticle(&zimCreator, 'A', "index.html", "Start page", zim::writer::Url("A/hello.html")));
  zimCreator.addArticle(redirect_article);

  zimCreator.finishZimCreation();

  // verify the created .zim file with 'zimdump'
  zim::File zimfile(out.path());
  EXPECT_EQ(zimfile.getCountArticles(), 4u);

  zim::Article a1 = zimfile.getArticle('A', "index.html");
  EXPECT_TRUE(a1.isRedirect());

  zim::Article a2 = a1.getRedirectArticle();
  EXPECT_EQ(a2.getTitle(), "HTML title tag content");
}

TEST(ZimCreatorFSTest, SymlinkShouldCreateRedirectArticleEntry)
{
  LibMagicInit libmagic;

  std::string directoryPath = "data/with-symlink";
  ZimCreatorFS zimCreator(directoryPath, "hello.html", false, false);

  TempFile out("with-symlink.zim");

  zimCreator.startZimCreation(out.path());
  zimCreator.visitDirectory(directoryPath);
  zimCreator.finishZimCreation();


  // VERIFY the created .zim file with 'zimdump'
  zim::File zimfile(out.path());
  EXPECT_EQ(zimfile.getCountArticles(), 4u);

  zim::Article a1 = zimfile.getArticle('A', "symlink.html");
  EXPECT_TRUE(a1.isRedirect());

  zim::Article a2 = a1.getRedirectArticle();
  EXPECT_EQ(a2.getTitle(), "Another HTML file");

  zim::Article a3 = zimfile.getArticle('A', "symlink-outside.html");
  EXPECT_FALSE(a3.good());

  zim::Article a4 = zimfile.getArticle('A', "symlink-not-existing.html");
  EXPECT_FALSE(a4.good());

  zim::Article a5 = zimfile.getArticle('A', "symlink-self.html");
  EXPECT_FALSE(a5.good());
}

TEST(ZimCreatorFSTest, ThrowsErrorIfDirectoryNotExist)
{
  EXPECT_THROW({
    ZimCreatorFS zimCreator("Non-existing-dir", "index.html", false, false);
  }, std::invalid_argument );
}
