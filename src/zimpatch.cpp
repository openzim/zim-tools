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

#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <zim/writer/creator.h>
#include <zim/blob.h>
#include <zim/article.h>
#include <zim/file.h>
#include <zim/fileiterator.h>
#include <zim/uuid.h>
#include <list>
#include <limits>
#include <algorithm>


std::string NumberToString(int number)
{
  std::ostringstream ss;
  ss << number;
  return ss.str();
}

/* Article clas that will be passed to the zimwriter.
 * Contains a zim::Article class, so it is easier to add a existing article
 */
class Article: public zim::writer::Article
{
    //article from an existing ZIM file.
    std::string _id;
    zim::Article Ar;
    bool isRd;
    zim::writer::Url redirectUrl;

public:
    Article():
      _id(""),
      isRd(false)
    {}

    explicit Article(const std::string& id):
      _id(id),
      isRd(false)
    {}

    explicit Article(const zim::Article a,int id):
      _id(NumberToString(id)),
       Ar(a),
       isRd(false)
    {}

    void setRedirectUrl(zim::writer::Url u)
    {
        redirectUrl=u;
        isRd=true;
        return;
    }

    virtual zim::writer::Url getUrl() const
    {
        return zim::writer::Url(Ar.getNamespace(), Ar.getUrl());
    }

    virtual std::string getTitle() const
    {
        return Ar.getTitle();
    }

    virtual bool isRedirect() const
    {
        return isRd;
    }

    virtual std::string getMimeType() const
    {
        return Ar.getMimeType();
    }

    virtual zim::writer::Url getRedirectUrl() const
    {
        return redirectUrl;
    }

    virtual std::string getParameter() const
    {
        return Ar.getParameter();
    }

    zim::Blob getData() const
    {
        return Ar.getData();
    }

    virtual zim::size_type getSize() const
    {
        return Ar.getArticleSize();
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

class ZimCreatorPatch : public zim::writer::Creator
{
    zim::File start_file;
    zim::File diff_file;
    zim::Uuid fileUid;
    zim::writer::Url mainUrl;
    zim::writer::Url layoutUrl;
    std::vector<std::string> delete_list;
    std::vector< int > dlist;
    std::map<std::string, std::string> redirectList;

    bool isAdditionalMetadata(std::string url)
    {
        if(url=="M/dlist")
            return true;
        if(url=="M/startfileuid")
            return true;
        if(url=="M/endfileuid")
            return true;
        if(url=="M/mainaurl")
            return true;
        if(url=="M/layoutaurl")
            return true;
        if(url=="M/redirectlist")
            return true;
        return false;
    }

public:
    explicit ZimCreatorPatch(std::string start_filename="",std::string diff_filename="")
    {
        start_file=zim::File(start_filename);
        diff_file=zim::File(diff_filename);
        std::string id=diff_file.getArticleByUrl("M/endfileuid").getPage();
        std::string temp="";
        unsigned int k=0;
        char tempArray[16];
        for(unsigned int i=0;i<id.size();i++)
        {
            if(id[i]=='\n')
            {
                tempArray[k]=(char) atoi(temp.c_str());
                //std::cout<<"\n"<<temp;
                temp="";
                k++;
            }
            else
            {
                temp+=id[i];
            }
        }
        fileUid=zim::Uuid(tempArray);
        std::string tempString=diff_file.getArticleByUrl("M/mainaurl").getPage();
        if(diff_file.getArticleByUrl(tempString).good()
         ||start_file.getArticleByUrl(tempString).good())
        {
            mainUrl=zim::writer::Url(tempString);
        }

        tempString=diff_file.getArticleByUrl("M/layoutaurl").getPage();
        if(diff_file.getArticleByUrl(tempString).good()
         ||start_file.getArticleByUrl(tempString).good())
        {
            layoutUrl=zim::writer::Url(tempString);
        }

        std::string tmp="";
        std::vector<std::string> rdlist;
        tempString=diff_file.getArticleByUrl("M/redirectlist").getPage();
        for(unsigned int i=0;i<tempString.size();i++)
        {
            if(tempString[i]!='\n')
            {
                tmp+=tempString[i];
            }
            else
            {
                rdlist.push_back(tmp);
                tmp="";
            }
        }
        for(unsigned i=0;i<rdlist.size();i=i+2)
        {
            redirectList[rdlist[i]] = rdlist[i+1];
        }

        zim::Article a = diff_file.getArticleByUrl("M/dlist");
        std::string dllist=a.getPage();
        tmp="";
        //Computing list of deleted articles and storing them in a vector.
        for(unsigned int i=0; i<dllist.size(); i++)
        {
            if(dllist[i]!='\n')
                tmp+=dllist[i];
            else
            {
                delete_list.push_back(tmp);
                tmp="";
            }
        }

        //Process dlist.
        std::cout<<"\nProcessing Delete list..\n"<<std::flush;
        dlist.resize(start_file.getFileheader().getArticleCount()+diff_file.getFileheader().getArticleCount());
        for(unsigned int i=0;i<dlist.size();i++)
        {
            dlist[i]=0;
        }
        for(unsigned int i=0;i<delete_list.size();i++)
        {
            //Deleted articles will always be present in the start_file only.
            dlist[start_file.getArticleByUrl(delete_list[i]).getIndex()]=1;
        }
        delete_list.clear();
        return;
    }

    zim::Uuid getUuid()
    {
        return fileUid;
    }
    zim::writer::Url getMainPage()
    {
        return mainUrl;
    }
    zim::writer::Url getLayoutPage()
    {
        return layoutUrl;
    }

    void create(const std::string& fname)
    {
        startZimCreation(fname);

        //Add all articles in File_1 that have not ben deleted.
        std::string url="";
        for (unsigned int index = 0; index < start_file.getFileheader().getArticleCount(); index++)
        {
            int id=index;
            zim::Article tmpAr=start_file.getArticle(index);
            if(dlist[id]==1)
            {
                continue;
            }

            Article tempArticle(tmpAr,id);
            //If the article is also present in file_2
            if(diff_file.getArticleByUrl(tmpAr.getLongUrl()).good())
            {
                tmpAr=diff_file.getArticleByUrl(tmpAr.getLongUrl());
                tempArticle=Article(tmpAr,start_file.getFileheader().getArticleCount()+diff_file.getArticleByUrl(tmpAr.getLongUrl()).getIndex());
                id=tmpAr.getIndex()+start_file.getFileheader().getArticleCount();
            }
            try {
                tempArticle.setRedirectUrl(zim::writer::Url(redirectList.at(tmpAr.getLongUrl())));
            } catch (...) {}
            //std::cout<<"\nArticle: "<<tempArticle.getNamespace()<<"/"<<tempArticle.getUrl();
            //std::cout<<"\nIndex: "<<tempArticle.getIdx();
            //getchar();
            addArticle(tempArticle);
        }

        //Now adding new articles in file_2
        for (unsigned int index =0; index<+diff_file.getFileheader().getArticleCount(); index++)
        {
            int id=start_file.getFileheader().getArticleCount()+index;
            zim::Article tmpAr=diff_file.getArticle(index);
            //If the article is already in file_1, it has been added.
            if(start_file.getArticleByUrl(tmpAr.getLongUrl()).good()||isAdditionalMetadata(tmpAr.getLongUrl()))
            {
                continue;
            }

            Article tempArticle(tmpAr,id);
            //std::cout<<"\nID: "<<id;
            try {
                tempArticle.setRedirectUrl(zim::writer::Url(redirectList.at(tmpAr.getLongUrl())));
            } catch (...) {}
            //std::cout<<"\nArticle: "<<tempArticle.getNamespace()<<"/"<<tempArticle.getUrl();
            //std::cout<<"\nIndex: "<<tempArticle.getIdx();
            //std::cout<<"\nIsredirect: "<<tempArticle.isRedirect();
            //getchar();
            addArticle(tempArticle);

        }
        finishZimCreation();
    }
};

void displayHelp()
{
    std::cout<<"\nzimpatch"
             "\nA tool to compute the end_file using a atart_file and a diff_file."
             "\nUsage: zimpatch [start_file] [diff_file] [output file]  \n";
    return;
}

//A series of checks to be executed before the patch process begins, in order to confirm that the correct files are being used.
bool isAdditionalMetadata(std::string url)
{
    if(url=="M/dlist")
        return true;
    if(url=="M/startfileuid")
        return true;
    if(url=="M/endfileuid")
        return true;
    if(url=="M/mainaurl")
        return true;
    if(url=="M/layoutaurl")
        return true;
    if(url=="M/redirectlist")
        return true;
    return false;
}

bool checkDiffFile(std::string startFileName, std::string diffFileName)
{
    zim::File startFile(startFileName);
    zim::File diffFile(diffFileName);
    std::vector<std::string >additionalMetadata;
    additionalMetadata.resize(6);
    additionalMetadata[0]="M/dlist";
    additionalMetadata[1]="M/startfileuid";
    additionalMetadata[2]="M/endfileuid";
    additionalMetadata[3]="M/mainaurl";
    additionalMetadata[4]="M/layoutaurl";
    additionalMetadata[5]="M/redirectlist";

    //Search in the ZIM file if the above articles are present:
    for(unsigned int i=0;i<additionalMetadata.size();i++)
    {
        if(!diffFile.getArticleByUrl(additionalMetadata[i]).good())        //If the article was not found in the file.
            return false;
    }

    //Check the UID of startFile and the value stored in startfileuid
    const char *startfileUID1=startFile.getFileheader().getUuid().data;
    std::string startFileUID2=diffFile.getArticleByUrl(additionalMetadata[1]).getPage();
    std::string temp="";
    unsigned int k=0;
    char tempArray[16];
    for(unsigned int i=0;i<startFileUID2.size();i++)
    {
        if(startFileUID2[i]=='\n')
        {
            tempArray[k]=(char) atoi(temp.c_str());
            //std::cout<<"\n"<<temp;
            temp="";
            k++;
        }
        else
        {
            temp+=startFileUID2[i];
        }
    }
    startFileUID2=tempArray;
    //std::cout<<"\nuid 1: "<<startFileUID;
    //std::cout<<"\nuid 2: "<<tempArray[15];
    bool compare=true;
    for(int i=0;i<16;i++)
    {
        if(tempArray[i]!=startfileUID1[i])
            compare=false;
    }
    if(!compare)
        return false;
    return true;
}

int main(int argc, char* argv[])
{

    //Parsing arguments
    //There will be only three arguments, so no detailed parsing is required.
    std::cout<<"zimpatch\nVERSION "<<VERSION<<"\n"<<std::flush;
    for(int i=0; i<argc; i++)
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
        displayHelp();
        return -1;
    }

    //Strings containing the filenames of the start_file, diff_file and end_file.
    std::string start_filename =argv[1];
    std::string diff_filename =argv[2];
    std::string end_filename= argv[3];
    std::cout<<"\nStart File: "<<start_filename;
    std::cout<<"\nDiff File: "<<diff_filename;
    std::cout<<"\nEnd File: "<<end_filename<<"\n";
    try
    {
        //Callling zimwriter to create the diff_file
        if(!checkDiffFile(start_filename,diff_filename))
        {
            std::cout<<"\n[ERROR]: The diff file provided does not match the given start file.";
            return 0;
        }

        ZimCreatorPatch c(start_filename, diff_filename);
        c.setMinChunkSize(2048);
        //Create the article source class, from which the content for the file will be read.
        //Create the actual file.
        c.create(end_filename);

    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
}
