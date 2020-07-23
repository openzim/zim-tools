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

#include "version.h"

std::string NumberToString(int number)
{
  std::ostringstream ss;
  ss << number;
  return ss.str();
}

class Article : public zim::writer::Article         //Article class that will be passed to the zimwriter. Contains a zim::Article class, so it is easier to add a
{
    //article from an existing ZIM file.
    zim::Entry e;
public:
    explicit Article(const zim::Entry e):
      e(e)
    {}

    virtual zim::writer::Url getUrl() const
    {
        auto path = e.getPath();
        return zim::writer::Url(path[0], path.substr(2));
    }

    virtual std::string getTitle() const
    {
        return e.getTitle();
    }

    virtual bool isRedirect() const
    {
        return e.isRedirect();
    }

    virtual std::string getMimeType() const
    {
        return e.getItem().getMimetype();
    }

    virtual zim::writer::Url getRedirectUrl() const {
      auto redirectEntry = e.getRedirectEntry();
      auto redirectPath = redirectEntry.getPath();
      return zim::writer::Url(redirectPath[0], redirectPath.substr(2));
    }

    virtual std::string getParameter() const
    {
        return "";
    }

    zim::Blob getData() const
    {
        return e.getItem().getData();
    }

    zim::size_type getSize() const
    {
        return e.getItem().getSize();
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
        return false;
    }
};


class ArticleRaw : public zim::writer::Article  //Article class, stores all data in itself. Easier for adding new articles.
{
public:
    std::string _id;
    std::string _data;
    zim::writer::Url url;
    std::string title;
    std::string mimeType;
    zim::writer::Url redirectUrl;
    bool _isRedirect;
    ArticleRaw()  { }
    virtual zim::writer::Url getUrl() const
    {
        return url;
    }
    virtual std::string getTitle() const
    {
        return title;
    }
    virtual bool isRedirect() const
    {
        return _isRedirect;
    }
    virtual std::string getMimeType() const
    {
        return mimeType;
    }
    virtual zim::writer::Url getRedirectUrl() const
    {
        return redirectUrl;
    }
    zim::Blob getData() const
    {
        return zim::Blob(&_data[0], _data.size());
    }

    virtual zim::size_type getSize() const
    {
        return _data.size();
    }

    virtual std::string getFilename() const
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
        return false;
    }
};

class ZimCreatorDiff : public zim::writer::Creator
{
    std::shared_ptr<ArticleRaw> dlist = std::make_shared<ArticleRaw>();               //Metadata article containing list of articles to be deleted.
    std::shared_ptr<ArticleRaw> startFileUID = std::make_shared<ArticleRaw>();        //Metadata article containing start file UID
    std::shared_ptr<ArticleRaw> endFileUID = std::make_shared<ArticleRaw>();          //Metadata article containing end file UID
    std::shared_ptr<ArticleRaw> mainAurl = std::make_shared<ArticleRaw>();
    std::shared_ptr<ArticleRaw> layoutAurl = std::make_shared<ArticleRaw>();
    std::shared_ptr<ArticleRaw> redirectList = std::make_shared<ArticleRaw>();
    int fileSize;
    zim::Archive archive_1;
    zim::Archive archive_2;
    std::list<std::string > deleteList;

public:
    explicit ZimCreatorDiff(std::string filename_1="",std::string filename_2="")
      : archive_1(filename_1),
        archive_2(filename_2)
    {
        fileSize = archive_2.getEntryCount();
        deleteList.clear();
        std::string rdlist="";
        //Scanning Data from files, generating list of articles to be deleted
        for(auto& entry:archive_1.iterByPath())
        {
          try {
            archive_2.getEntryByPath(entry.getPath());
          } catch(...) {
            deleteList.push_back(entry.getPath());
          }
        }
        for(auto& entry:archive_2.iterByPath())
        {
            if(entry.isRedirect())
            {
                rdlist+=entry.getPath()+"\n"+entry.getRedirectEntry().getPath()+"\n";
            }
        }
        //Setting data in dlist,startFileUID,endFileUID, etc.

        //dlist article.
        //List of articles to be deleted from start_file.
        std::string dlListText="";
        for(std::list<std::string >::iterator it=deleteList.begin(); it!=deleteList.end(); ++it)
        {
            dlListText+=*it+"\n";
        }
        dlist->title="dlist";
        dlist->url=zim::writer::Url('M', "dlist");
        dlist->mimeType="text/plain";
        dlist->_data=dlListText;
        dlist->_id=NumberToString((long long)fileSize+1);
        dlist->_isRedirect=false;

        //StartFileUID
        //contains the UID of the start_file.
        startFileUID->title="startfileuid";
        startFileUID->url=zim::writer::Url('M', "startfileuid");
        startFileUID->mimeType="text/plain";
        const char *s=archive_1.getUuid().data;
        std::string st="";
        for(int i=0;i<16;i++)
        {
            st+=NumberToString((int)s[i]);
            st+="\n";
        }
        startFileUID->_data=st;
        startFileUID->_id=NumberToString((long long)fileSize+2);
        startFileUID->_isRedirect=false;

        //EndFileUID
        //contains the UID of the end_file.
        endFileUID->title="endfileuid";
        endFileUID->url=zim::writer::Url('M', "endfileuid");
        endFileUID->mimeType="text/plain";
        s=archive_2.getUuid().data;
        st="";
        for(int i=0;i<16;i++)
        {
            st+=NumberToString((int)s[i]);
            st+="\n";
        }
        endFileUID->_data=st;
        endFileUID->_id=NumberToString((long long)fileSize+3);
        endFileUID->_isRedirect=false;

        //Metadata article storing the MAIN Article for the new ZIM file.
        mainAurl->title="mainaurl";
        mainAurl->url=zim::writer::Url('M', "mainaurl");
        mainAurl->mimeType="text/plain";
        try {
          mainAurl->_data = archive_2.getMainEntry().getPath();
        } catch(...) {
          mainAurl->_data = "";
        }
        mainAurl->_id=NumberToString((long long)fileSize+4);
        mainAurl->_isRedirect=false;


        //Metadata entry storing the Layout Article for the new ZIM file.
        layoutAurl->title="layoutaurl";
        layoutAurl->url=zim::writer::Url('M', "layoutaurl");
        layoutAurl->mimeType="text/plain";
        /*try {
            layoutAurl->_data = archive_2.getLayoutEntry().getPath();
        } catch(...) {
            layoutAurl->_data="";
        }*/
        layoutAurl->_data="";
        layoutAurl->_id=NumberToString((long long)fileSize+5);
        layoutAurl->_isRedirect=false;

        //Meatadata entry for storing the redirect entries.
        redirectList->title="redirectlist";
        redirectList->url=zim::writer::Url('M', "redirectlist");
        redirectList->mimeType="text/plain";
        redirectList->_data=rdlist;
        redirectList->_id=NumberToString((long long)fileSize+6);
        redirectList->_isRedirect=false;
    }

    virtual void create(const std::string& fname)
    {
        startZimCreation(fname);
        //Add All articles in file_2 that are not in file_1 and those that are not the same in file_1

        //Articles are added frm file_2.
        //loop till an article read to be added is found.
        for(auto& entry2:archive_2.iterByPath())
        {
            //irt is the file pointer in file_2
            try {
                auto entry1 = archive_1.getEntryByPath(entry2.getPath());
                if (entry2.isRedirect() || entry1.isRedirect()) {
                  // [FIXME] Handle redirection !!!
                  continue;
                }
                if (std::string(entry2.getItem().getData()) != std::string(entry1.getItem().getData())) {
                    auto tempArticle = std::make_shared<Article>(entry2);
                    addArticle(tempArticle);
                    continue;
                }
            } catch(...) { //If the article is not present in FILe 1
                auto tempArticle = std::make_shared<Article>(entry2);
                addArticle(tempArticle);
                continue;
            }
        }
        //Add Metadata articles.
        addArticle(dlist);
        addArticle(startFileUID);
        addArticle(endFileUID);
        addArticle(mainAurl);
        addArticle(layoutAurl);
        addArticle(redirectList);
        finishZimCreation();
    }
};

void displayHelp()
{
    std::cout<<"\nzimdiff"
    "\nA tool to obtain the diff_file between two ZIM files, in order to facilitate incremental updates."
    "\nUsage: zimdiff [start_file] [end_file] [output file]"
    "\nOption: -v, --version    print software version\n";
    return;
}

int main(int argc, char* argv[])
{

    //Parsing arguments
    //There will be only two arguments, so no detailed parsing is required.
    std::cout<<"zimdiff\n";
    for(int i=0;i<argc;i++)
    {
        if(std::string(argv[i])=="-H" ||
           std::string(argv[i])=="--help" ||
           std::string(argv[i])=="-h")
        {
            displayHelp();
            return 0;
        }

        if(std::string(argv[i])=="--version" ||
           std::string(argv[i])=="-v")
        {
            version();
            return 0;
        }
    }
    if(argc<4)
    {
        std::cout<<"\n[ERROR] Not enough Arguments provided\n";
        return -1;
    }
    std::string filename_1 =argv[1];
    std::string filename_2 =argv[2];
    std::string op_file= argv[3];
    try
    {
        ZimCreatorDiff c(filename_1, filename_2);
        //Create the actual file.
        c.create(op_file);

    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
}
