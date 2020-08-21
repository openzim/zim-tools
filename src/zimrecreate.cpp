/*
 * Copyright (C) 2019-2020 Matthieu Gautier <mgautier@kymeria.fr>
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

#include "tools.h"
#include "version.h"


void create(const std::string& originFilename, const std::string& outFilename, bool zstdFlag)
{
  zim::Archive origin(originFilename);
  zim::writer::Creator zimCreator;
  zimCreator.configVerbose(true)
            // [TODO] Use the correct language
            .configIndexing(true, "eng")
            .configMinClusterSize(2048)
            .configCompression(zstdFlag ? zim::zimcompZstd : zim::zimcompLzma);

  std::cout << "starting zim creation" << std::endl;
  zimCreator.startZimCreation(outFilename);

  try {
    zimCreator.setMainPath(origin.getMainEntry().getPath());
  } catch(...) {}

  for(auto& entry:origin.iterEfficient())
  {
    auto path = entry.getPath();
    if (path[0] == 'Z' || path[0] == 'X') {
      // Index is recreated by zimCreator. Do not add it
    continue;
    }
    if (entry.isRedirect()) {
      zimCreator.addRedirection(entry.getPath(), entry.getTitle(), entry.getRedirectEntry().getPath());
    } else {
      auto tmpItem = std::shared_ptr<zim::writer::Item>(new CopyItem(entry.getItem()));
      zimCreator.addItem(tmpItem);
    }

  }
  zimCreator.finishZimCreation();
}

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
        create(originFilename, outputFilename, zstdFlag);
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
}
