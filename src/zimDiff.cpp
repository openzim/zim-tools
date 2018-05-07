/*
 * Copyright (C) 2013 Kiran Mathew Koshy
 *
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


#define VERSION "0.6.0.0"
#include <iostream>
#include <sstream>
#include <vector>
#include <zim/writer/zimcreator.h>
#include <zim/blob.h>
#include <zim/article.h>
#include <zim/file.h>
#include <zim/fileiterator.h>
#include <list>
#include <algorithm>
#include <sstream>

std::string NumberToString(int number)
{
  std::ostringstream ss;
  ss << number;
  return ss.str();
}


class Article : public zim::writer::Article         //Article class that will be passed to the zimwriter. Contains a zim::Article class, so it is easier to add a
{
    //article from an existing ZIM file.
    std::string _id;
    zim::Article Ar;
    bool isRd;
public:
    Article():
      isRd(false)
    {}

    explicit Article(const std::string& id):
      _id(id),
      isRd(false)
    {}

    explicit Article(const zim::Article a):
      _id(NumberToString(a.getIndex())),
      Ar(a),
      isRd(false)
    {}

    void setRedirect()
    {
        isRd=true;
    }

    virtual std::string getAid() const
    {
        return _id;
    }

    virtual char getNamespace() const
    {
        return Ar.getNamespace();
    }

    virtual std::string getUrl() const
    {
        return Ar.getUrl();
    }

    virtual std::string getTitle() const
    {
        return Ar.getTitle();
    }

    virtual bool isRedirect() const
    {
        return false;
    }

    virtual std::string getMimeType() const
    {
        if(isRd)
        {
            return "redirect";
        }
        else
        {
            return Ar.getMimeType();
        }
    }

    virtual std::string getRedirectAid() const
    {
        //return std::to_string((long long)Ar.getRedirectIndex()+1);
        return "";
    }

    virtual std::string getParameter() const
    {
        return Ar.getParameter();
    }

    zim::Blob getData() const
    {
        return Ar.getData();
    }

    zim::size_type getSize() const
    {
        return Ar.getArticleSize();
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
    std::string Url;
    std::string Title;
    std::string MimeType;
    std::string RedirectAid;
    char nm;
    bool IsRedirect;
    ArticleRaw()  { }
    virtual std::string getAid() const
    {
        return _id;
    }
    virtual char getNamespace() const
    {
        return nm;
    }
    virtual std::string getUrl() const
    {
        return Url;
    }
    virtual std::string getTitle() const
    {
        return Title;
    }
    virtual bool isRedirect() const
    {
        return IsRedirect;
    }
    virtual std::string getMimeType() const
    {
        return MimeType;
    }
    virtual std::string getRedirectAid() const
    {
        return RedirectAid;
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

class ZimCreatorDiff : public zim::writer::ZimCreator
{
    ArticleRaw dlist;               //Metadata article containing list of articles to be deleted.
    ArticleRaw startFileUID;        //Metadata article containing start file UID
    ArticleRaw endFileUID;          //Metadata article containing end file UID
    ArticleRaw mainAurl;
    ArticleRaw layoutAurl;
    ArticleRaw redirectList;
    int fileSize;
    zim::File file_1;
    zim::File file_2;
    std::list<std::string > deleteList;

public:
    explicit ZimCreatorDiff(std::string filename_1="",std::string filename_2="")
    {
        file_1 = zim::File(filename_1);
        file_2 = zim::File(filename_2);
        fileSize = file_2.getFileheader().getArticleCount();
        deleteList.clear();
        std::string rdlist="";
        //Scanning Data from files, generating list of articles to be deleted
        for(zim::File::const_iterator it = file_1.begin(); it != file_1.end(); ++it)
        {
            if(!file_2.getArticleByUrl(it->getLongUrl()).good())
                deleteList.push_back(it->getLongUrl());
        }
        for(zim::File::const_iterator it=file_2.begin();it!=file_2.end();++it)
        {
            if(it->isRedirect())
            {
                rdlist+=it->getLongUrl()+"\n"+it->getRedirectArticle().getLongUrl()+"\n";
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
        dlist.Title="dlist";
        dlist.Url="dlist";
        dlist.MimeType="text/plain";
        dlist._data=dlListText;
        dlist.nm='M';
        dlist._id=NumberToString((long long)fileSize+1);
        dlist.IsRedirect=false;
        dlist.RedirectAid="";

        //StartFileUID
        //contains the UID of the start_file.
        startFileUID.Title="startfileuid";
        startFileUID.Url="startfileuid";
        startFileUID.MimeType="text/plain";
        const char *s=file_1.getFileheader().getUuid().data;
        std::string st="";
        for(int i=0;i<16;i++)
        {
            st+=NumberToString((int)s[i]);
            st+="\n";
        }
        startFileUID._data=st;
        startFileUID.nm='M';
        startFileUID._id=NumberToString((long long)fileSize+2);
        startFileUID.IsRedirect=false;
        startFileUID.RedirectAid="";

        //EndFileUID
        //contains the UID of the end_file.
        endFileUID.Title="endfileuid";
        endFileUID.Url="endfileuid";
        endFileUID.MimeType="text/plain";
        s=file_2.getFileheader().getUuid().data;
        st="";
        for(int i=0;i<16;i++)
        {
            st+=NumberToString((int)s[i]);
            st+="\n";
        }
        endFileUID._data=st;
        endFileUID.nm='M';
        endFileUID._id=NumberToString((long long)fileSize+3);
        endFileUID.IsRedirect=false;
        endFileUID.RedirectAid="";

        //Metadata article storing the MAIN Article for the new ZIM file.
        mainAurl.Title="mainaurl";
        mainAurl.Url="mainaurl";
        mainAurl.MimeType="text/plain";
        if(file_2.getFileheader().hasMainPage())
            mainAurl._data=file_2.getArticle(file_2.getFileheader().getMainPage()).getLongUrl();
        else
            mainAurl._data="";
        mainAurl.nm='M';
        mainAurl._id=NumberToString((long long)fileSize+4);
        mainAurl.IsRedirect=false;
        mainAurl.RedirectAid="";


        //Metadata entry storing the Layout Article for the new ZIM file.
        layoutAurl.Title="layoutaurl";
        layoutAurl.Url="layoutaurl";
        layoutAurl.MimeType="text/plain";
        if(file_2.getFileheader().hasLayoutPage())
            layoutAurl._data=file_2.getArticle(file_2.getFileheader().getLayoutPage()).getLongUrl();
        else
            layoutAurl._data="";
        layoutAurl.nm='M';
        layoutAurl._id=NumberToString((long long)fileSize+5);
        layoutAurl.IsRedirect=false;
        layoutAurl.RedirectAid="";



        //Meatadata entry for storing the redirect entries.
        redirectList.Title="redirectlist";
        redirectList.Url="redirectlist";
        redirectList.MimeType="text/plain";
        redirectList._data=rdlist;
        redirectList.nm='M';
        redirectList._id=NumberToString((long long)fileSize+6);
        redirectList.IsRedirect=false;
        redirectList.RedirectAid="";
        return;
    }

    virtual void create(const std::string& fname)
    {
        startZimCreation(fname);
        //Add All articles in file_2 that are not in file_1 and those that are not the same in file_1

        //Articles are added frm file_2.
        //loop till an article read to be added is found.
        for(auto& article1: file_2)
        {
            //irt is the file pointer in file_2
            if(!file_1.getArticleByUrl(article1.getLongUrl()).good()) //If the article is not present in FILe 1
            {
                Article tempArticle(article1);
                if(article1.isRedirect())
                    tempArticle.setRedirect();
                addArticle(tempArticle);
                continue;
            }
            //if this place of the loop is reached, it is sure that the article is presentin file_1
            if(!(file_1.getArticleByUrl(article1.getLongUrl()).getData()==article1.getData()))      //if the data stored in the same article is different in file_1 and 2
            {
                Article tempArticle(article1);
                if(article1.isRedirect())
                    tempArticle.setRedirect();
                addArticle(tempArticle);
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
    "\nUsage: zimdiff [start_file] [end_file] [output file]  \n";
    return;
}

int main(int argc, char* argv[])
{

    //Parsing arguments
    //There will be only two arguments, so no detailed parsing is required.
    std::cout<<"zimdiff\n";
    for(int i=0;i<argc;i++)
    {
        if(std::string(argv[i])=="-h")
        {
            displayHelp();
            return 0;
        }

        if(std::string(argv[i])=="-H")
        {
            displayHelp();
            return 0;
        }

        if(std::string(argv[i])=="--help")
        {
            displayHelp();
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
