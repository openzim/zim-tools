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

#include <zim/archive.h>

#include "gtest/gtest.h"

#include "../src/zimwriterfs/zimcreatorfs.h"
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
  ZimCreatorFS zimCreator(directoryPath);
  zimCreator.setMainPath("index.html");

  TempFile out("minimal.zim");

  zimCreator.startZimCreation(out.path());
  zimCreator.visitDirectory(directoryPath);

  zimCreator.addRedirection("index.html", "Start page", "hello.html");

  zimCreator.finishZimCreation();

  // verify the created .zim file with 'zimdump'
  zim::Archive archive(out.path());
  EXPECT_EQ(archive.getEntryCount(), 3u);

  auto e1 = archive.getEntryByPath("index.html");
  EXPECT_TRUE(e1.isRedirect());

  auto e2 = e1.getRedirectEntry();
  EXPECT_EQ(e2.getTitle(), "HTML title tag content");
}

TEST(ZimCreatorFSTest, SymlinkShouldCreateRedirectEntry)
{
  LibMagicInit libmagic;

  std::string directoryPath = "data/with-symlink";
  ZimCreatorFS zimCreator(directoryPath);
  zimCreator.setMainPath("hello.html");

  TempFile out("with-symlink.zim");

  zimCreator.startZimCreation(out.path());
  zimCreator.visitDirectory(directoryPath);
  zimCreator.finishZimCreation();


  // VERIFY the created .zim file with 'zimdump'
  zim::Archive archive(out.path());
  EXPECT_EQ(archive.getEntryCount(), 3u);

  auto e1 = archive.getEntryByPath("symlink.html");
  EXPECT_TRUE(e1.isRedirect());

  auto e2 = e1.getRedirectEntry();
  EXPECT_EQ(e2.getTitle(), "Another HTML file");

  EXPECT_FALSE(archive.hasEntryByPath("symlink-outside.html"));

  EXPECT_FALSE(archive.hasEntryByPath("symlink-not-existing.html"));

  EXPECT_FALSE(archive.hasEntryByPath("symlink-self.html"));
}

TEST(ZimCreatorFSTest, ThrowsErrorIfDirectoryNotExist)
{
  EXPECT_THROW({
    ZimCreatorFS zimCreator("Non-existing-dir");
  }, std::invalid_argument );
}

bool operator==(const Redirect& a, const Redirect& b) {
  return a.path == b.path && a.title == b.title && a.target == b.target;
}

TEST(ZimCreatorFSTest, ParseRedirect)
{
  {
  std::stringstream ss;
  ss << "path\ttitle\ttarget\n";
  ss << "A/path/to/somewhere\tAn amazing title\tAnother/path";

  std::vector<Redirect> found;
  parse_redirectArticles(
    ss,
    [&](Redirect redirect)
     {found.push_back(redirect);}
  );

  const std::vector<Redirect> expected {
    {"path", "title", "target"},
    {"A/path/to/somewhere", "An amazing title", "Another/path"}
  };
  EXPECT_EQ(found, expected);
  }


  {
    std::stringstream ss;
    ss << "A/path\tOups, no target";
    EXPECT_THROW({
      parse_redirectArticles(
            ss,
            [&](Redirect redirect)
             {}
          );
    }, std::runtime_error);
  }
}
