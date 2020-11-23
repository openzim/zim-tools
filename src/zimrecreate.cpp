/*
 * Copyright (C) 2013 Kiran Mathew Koshy
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

#include <iostream>
#include <sstream>
#include <vector>
#include <zim/writer/creator.h>
#include <zim/blob.h>
#include <zim/item.h>
#include <zim/archive.h>
#include <list>
#include <algorithm>
#include <sstream>

#include "version.h"

class Article : public zim::writer::Article         //Article class that will be passed to the zimwriter. Contains a zim::Article class, so it is easier to add a
{
    zim::Entry entry;
public:
    explicit Article(const zim::Entry e):
      entry(e)
    {}

    virtual zim::writer::Url getUrl() const
    {
        return zim::writer::Url(entry.getPath());
    }

    virtual std::string getTitle() const
    {
        return entry.getTitle();
    }

    virtual bool isRedirect() const
    {
        return entry.isRedirect();
    }

    virtual std::string getMimeType() const
    {
        if (isRedirect()) { return ""; }
        return entry.getItem().getMimetype();
    }

    virtual zim::writer::Url getRedirectUrl() const {
      auto redirectEntry = entry.getRedirectEntry();
      return zim::writer::Url(redirectEntry.getPath());
    }

    zim::Blob getData() const
    {
        return entry.getItem().getData();
    }

    zim::size_type getSize() const
    {
        return entry.getItem().getSize();
    }

    std::string getFilename() const
    {
        return "";
    }

    bool shouldCompress() const
    {
        return getMimeType().find("text") == 0
            || getMimeType() == "application/javascript"
            || getMimeType() == "application/json"
            || getMimeType() == "image/svg+xml";
    }

    bool shouldIndex() const
    {
        return getMimeType().find("text/html") == 0;
    }
};

using pair_type = std::pair<zim::entry_index_type, zim::cluster_index_type>;

class ComparatorByCluster {
  public:
    ComparatorByCluster(const zim::Archive& origin):
      origin(origin) {
    }

    bool operator() (pair_type i, pair_type j) {
      return i.second < j.second;
    }
  const zim::Archive& origin;
};


class ZimRecreator : public zim::writer::Creator
{
    zim::Archive origin;

public:
  explicit ZimRecreator(std::string originFilename="", bool zstd = false) :
    zim::writer::Creator(true, zstd ? zim::zimcompZstd : zim::zimcompLzma),
    origin(originFilename)
    {
        // [TODO] Use the correct language
        setIndexing(true, "eng");
        setMinChunkSize(2048);
    }

    virtual void create(const std::string& fname)
    {
        std::cout << "starting zim creation" << std::endl;
        startZimCreation(fname);
        for(auto& entry:origin.iterEfficient())
        {
          auto path = entry.getPath();
          if (path[0] == 'Z' || path[0] == 'X') {
            // Index is recreated by zimCreator. Do not add it
            continue;
          }
          auto tempArticle = std::make_shared<Article>(entry);
          addArticle(tempArticle);
        }
        finishZimCreation();
    }

    virtual zim::writer::Url getMainUrl() const {
      try {
        return zim::writer::Url(origin.getMainEntry().getPath());
      } catch(...) {
        return zim::writer::Url();
      }
    }

    virtual zim::writer::Url getLayoutUrl() const {
      return zim::writer::Url();
    }
};

void usage()
{
    std::cout << "\nzimrecreate recreates a ZIM file from a existing ZIM.\n"
    "\nUsage: zimrecreate ORIGIN_FILE OUTPUT_FILE [Options]"
    "\nOptions:\n"
    "\t-v, --version    print software version\n"
    "\t-z, --zstd       use Zstandard as ZIM compression (lzma otherwise)\n";
    return;
}

int main(int argc, char* argv[])
{
    bool zstdFlag = false;

    //Parsing arguments
    //There will be only two arguments, so no detailed parsing is required.
    std::cout<<"zimrecreate\n";
    for(int i=0;i<argc;i++)
    {
        if(std::string(argv[i])=="-H" ||
           std::string(argv[i])=="--help" ||
           std::string(argv[i])=="-h")
        {
            usage();
            return 0;
        }

        if(std::string(argv[i])=="--version" ||
           std::string(argv[i])=="-v")
        {
            version();
            return 0;
        }

        if(std::string(argv[i])=="--zstd" ||
           std::string(argv[i])=="-z")
        {
            zstdFlag = true;
        }
    }
    if(argc<3)
    {
        std::cout<<"\n[ERROR] Not enough Arguments provided\n";
        usage();
        return -1;
    }
    std::string originFilename =argv[1];
    std::string outputFilename =argv[2];
    try
    {
        ZimRecreator c(originFilename, zstdFlag);
        //Create the actual file.
        c.create(outputFilename);
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
}
