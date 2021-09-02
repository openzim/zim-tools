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

/**
 * A PatchItem. This patch html and css content to remove the namespcae from the links.
 */
class PatchItem : public zim::writer::Item
{
    //article from an existing ZIM file.
    zim::Item item;

  public:
    explicit PatchItem(const zim::Item item):
      item(item)
    {}

    virtual std::string getPath() const
    {
      auto path = item.getPath();
      if (path.length() > 2 && path[1] == '/') {
        path = path.substr(2, std::string::npos);
      }
      return path;
    }

    virtual std::string getTitle() const
    {
        return item.getTitle();
    }

    virtual std::string getMimeType() const
    {
        return item.getMimetype();
    }

    std::unique_ptr<zim::writer::ContentProvider> getContentProvider() const
    {
        auto mimetype = getMimeType();
        if ( mimetype.find("text/html") == std::string::npos
          && mimetype.find("text/css") == std::string::npos) {
            return std::unique_ptr<zim::writer::ContentProvider>(new ItemProvider(item));
        }

        std::string content = item.getData();
        // This is a really poor url rewriting to remove the starting "../<NS>/"
        // and replace the "../../<NS/" by "../" :
        // - Performance may be better
        // - We only fix links in articles in "root" path (`foo.html`) and in one subdirectory (`bar/foo.hmtl`)
        //   Deeper articles are not fixed (`bar/baz/foo.html`).
        // - We may change content starting by `'../A/` even if they are not links
        // - We don't handle links where we go upper in the middle of the link : `../foo/../I/image.png`
        // - ...
        // However, this should patch most of the links in our zim files.
        for (std::string prefix: {"'", "\""}) {
          for (auto ns : {'A','I','J','-'}) {
            replaceStringInPlace(content, prefix+"../../"+ns+"/", prefix+"../");
            replaceStringInPlace(content, prefix+"../"+ns+"/", prefix);
          }
        }
        return std::unique_ptr<zim::writer::ContentProvider>(new zim::writer::StringProvider(content));
    }

  zim::writer::Hints getHints() const {
    return { { zim::writer::HintKeys::FRONT_ARTICLE, guess_is_front_article(item.getMimetype()) } };
  }
};


void create(const std::string& originFilename, const std::string& outFilename, bool withFtIndexFlag, unsigned long nbThreads)
{
  zim::Archive origin(originFilename);
  zim::writer::Creator zimCreator;
  zimCreator.configVerbose(true)
            // [TODO] Use the correct language
            .configIndexing(withFtIndexFlag, "eng")
            .configClusterSize(2048*1024)
            .configNbWorkers(nbThreads);

  std::cout << "starting zim creation" << std::endl;
  zimCreator.startZimCreation(outFilename);

  auto fromNewNamespace = origin.hasNewNamespaceScheme();

  try {
    auto mainPath = origin.getMainEntry().getItem(true).getPath();
    if (!fromNewNamespace) {
      mainPath = mainPath.substr(2, std::string::npos);
    }
    zimCreator.setMainPath(mainPath);
  } catch(...) {}

  try {
    auto illustration = origin.getIllustrationItem();
    zimCreator.addIllustration(48, illustration.getData());
  } catch(...) {}

  for(auto& metakey:origin.getMetadataKeys()) {
    if (metakey == "Counter" ) {
      // Counter is already added by libzim
      continue;
    }
    auto metadata = origin.getMetadata(metakey);
    auto metaProvider = std::unique_ptr<zim::writer::ContentProvider>(new zim::writer::StringProvider(metadata));
    zimCreator.addMetadata(metakey, std::move(metaProvider), "text/plain");
  }


  for(auto& entry:origin.iterEfficient()) {
    if (fromNewNamespace) {
      //easy, just "copy" the item.
      if (entry.isRedirect()) {
        zimCreator.addRedirection(entry.getPath(), entry.getTitle(), entry.getRedirectEntry().getPath(), {{zim::writer::HintKeys::FRONT_ARTICLE, 1}});
      } else {
        auto tmpItem = std::shared_ptr<zim::writer::Item>(new CopyItem(entry.getItem()));
        zimCreator.addItem(tmpItem);
      }
      continue;
    }

    // We have to adapt the content to drop the namespace.

    auto path = entry.getPath();
    if (path[0] == 'Z' || path[0] == 'X' || path[0] == 'M') {
      // Index is recreated by zimCreator. Do not add it
      continue;
    }

    path = path.substr(2, std::string::npos);
    if (entry.isRedirect()) {
      auto redirectPath = entry.getRedirectEntry().getPath();
      redirectPath = redirectPath.substr(2, std::string::npos);
      zimCreator.addRedirection(path, entry.getTitle(), redirectPath);
    } else {
      auto tmpItem = std::shared_ptr<zim::writer::Item>(new PatchItem(entry.getItem()));
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
    "\t-v, --version           print software version\n"
    "\t-j, --withoutFTIndex    don't create and add a fulltext index of the content to the ZIM\n"
    "\t-J, --threads <number>  count of threads to utilize (default: 4)\n";
    return;
}

int main(int argc, char* argv[])
{
    bool withFtIndexFlag = true;
    unsigned long nbThreads = 4;

    //Parsing arguments
    //There will be only two arguments, so no detailed parsing is required.
    std::cout << "zimrecreate" << std::endl;;
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

        if(std::string(argv[i])=="--withoutFTIndex" ||
           std::string(argv[i])=="-j")
        {
            withFtIndexFlag = false;
        }

        if(std::string(argv[i])=="-J" ||
           std::string(argv[i])=="--threads")
        {
            if(argc<5)
            {
                std::cout << std::endl << "[ERROR] Not enough Arguments provided" << std::endl;
                usage();
                return -1;
            }
            try
            {
                nbThreads = std::stoul(argv[i+1]);
            }
            catch (...)
            {
                std::cerr << "The number of workers should be a number" << std::endl;
                usage();
                return -1;
            }
        }
    }

    if(argc<3)
    {
        std::cout << std::endl << "[ERROR] Not enough Arguments provided" << std::endl;
        usage();
        return -1;
    }
    std::string originFilename = argv[1];
    std::string outputFilename = argv[2];
    try
    {
        create(originFilename, outputFilename, withFtIndexFlag, nbThreads);
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
}
