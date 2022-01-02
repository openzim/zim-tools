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
#include <zim/archive.h>
#include <zim/item.h>
#include <list>
#include <algorithm>
#include <sstream>

#include "tools.h"

#include "version.h"

std::string NumberToString(int number)
{
  std::ostringstream ss;
  ss << number;
  return ss.str();
}


void create(const std::string& filename_1, const std::string& filename_2, const std::string& outpath)
{
  zim::writer::Creator zimCreator;
  zimCreator.startZimCreation(outpath);

  zim::Archive archive_1(filename_1);
  zim::Archive archive_2(filename_2);


  //dlist article.
  //List of articles to be deleted from start_file.
  std::string dlist;
  for(auto& entry:archive_1.iterByPath())
  {
    try {
      archive_2.getEntryByPath(entry.getPath());
    } catch(...) {
      dlist+=entry.getPath()+"\n";
    }
  }
  zimCreator.addMetadata("dlist", dlist);

  std::string redirectList;
  for(auto& entry:archive_2.iterByPath())
  {
    if(entry.isRedirect())
    {
      redirectList+=entry.getPath()+"\n"+entry.getRedirectEntry().getPath()+"\n";
    }
  }
  zimCreator.addMetadata("redirectlist", redirectList);

  //StartFileUID
  //contains the UID of the start_file.
  std::string startFileUID;
  const auto uuid1 = archive_1.getUuid();
  const char *s=uuid1.data;
  for(int i=0;i<16;i++)
  {
    startFileUID+=NumberToString((int)s[i]);
    startFileUID+="\n";
  }
  zimCreator.addMetadata("startfileuid", startFileUID);

  //EndFileUID
  //contains the UID of the end_file.
  std::string endFileUID;
  const auto uuid2 = archive_2.getUuid();
  s=uuid2.data;
  for(int i=0;i<16;i++)
  {
    endFileUID+=NumberToString((int)s[i]);
    endFileUID+="\n";
  }
  zimCreator.addMetadata("endfileuid", endFileUID);

  //Metadata article storing the MAIN Article for the new ZIM file.
  std::string mainAurl;
  try {
    mainAurl = archive_2.getMainEntry().getPath();
  } catch(...) {}
  zimCreator.addMetadata("mainaurl", mainAurl);

  //Add All articles in file_2 that are not in file_1 and those that are not the same in file_1

  //Articles are added frm file_2.
  //loop till an article read to be added is found.
  for(auto& entry2:archive_2.iterByPath())
  {
    try {
      auto entry1 = archive_1.getEntryByPath(entry2.getPath());
      if (entry2.isRedirect() || entry1.isRedirect()) {
        // [FIXME] Handle redirection !!!
        continue;
      }
      if (std::string(entry2.getItem().getData()) != std::string(entry1.getItem().getData())) {
        auto tmpItem = std::shared_ptr<zim::writer::Item>(new CopyItem(entry2.getItem()));
        zimCreator.addItem(tmpItem);
        continue;
      }
    } catch(...) { //If the article is not present in FILe 1
      if (entry2.isRedirect()) {
        zimCreator.addRedirection(entry2.getPath(), entry2.getTitle(), entry2.getRedirectEntry().getPath());
      } else {
        auto tmpItem = std::shared_ptr<zim::writer::Item>(new CopyItem(entry2.getItem()));
        zimCreator.addItem(tmpItem);
      }
      continue;
    }
  }
  zimCreator.finishZimCreation();
}

void usage()
{
    std::cout<<"\nzimdiff computes a diff_file between two ZIM files, in order to facilitate incremental updates.\n"
    "\nUsage: zimdiff [start_file] [end_file] [output file]"
    "\nOption: -v, --version    print software version\n";
    return;
}

int main(int argc, char* argv[])
{

    //Parsing arguments
    //There will be only two arguments, so no detailed parsing is required.
    for (int i=0;i<argc;i++)
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
            printVersions();
            return 0;
        }
    }
    if (argc<4)
    {
        std::cout<<"\n[ERROR] Not enough Arguments provided\n";
        usage();
        return -1;
    }
    std::string filename_1 =argv[1];
    std::string filename_2 =argv[2];
    std::string op_file= argv[3];
    try
    {
        create(filename_1, filename_2, op_file);
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
}
