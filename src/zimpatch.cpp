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
#include <map>
#include <zim/writer/creator.h>
#include <zim/blob.h>
#include <zim/archive.h>
#include <zim/item.h>
#include <zim/uuid.h>
#include <list>
#include <limits>
#include <algorithm>

#include "version.h"

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
    zim::Entry entry;
    bool isRd;
    zim::writer::Url redirectUrl;

public:
    explicit Article(zim::Entry h):
       entry(h),
       isRd(false)
    {}

    void setRedirectUrl(zim::writer::Url u)
    {
        redirectUrl=u;
        isRd=true;
    }

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
        return isRd;
    }

    virtual std::string getMimeType() const
    {
        return entry.getItem().getMimetype();
    }

    virtual zim::writer::Url getRedirectUrl() const
    {
        return redirectUrl;
    }

    zim::Blob getData() const
    {
        return entry.getItem().getData();
    }

    virtual zim::size_type getSize() const
    {
        return entry.getItem().getSize();
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
    zim::Archive start_archive;
    zim::Archive diff_archive;
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
      : start_archive(start_filename),
        diff_archive(diff_filename)
    {
        std::string id=diff_archive.getMetadata("endfileuid");
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
        std::string tempString=diff_archive.getMetadata("mainaurl");
        if(diff_archive.hasEntryByPath(tempString) || start_archive.hasEntryByPath(tempString))
        {
            mainUrl=zim::writer::Url(tempString);
        }

        tempString=diff_archive.getMetadata("layoutaurl");
        if(diff_archive.hasEntryByPath(tempString)
         ||start_archive.hasEntryByPath(tempString))
        {
            layoutUrl=zim::writer::Url(tempString);
        }

        std::string tmp="";
        std::vector<std::string> rdlist;
        tempString=diff_archive.getMetadata("redirectlist");
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

        std::string dllist = diff_archive.getMetadata("dlist");
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
        dlist.resize(start_archive.getEntryCount()+diff_archive.getEntryCount());
        for(unsigned int i=0;i<dlist.size();i++)
        {
            dlist[i]=0;
        }
        for(unsigned int i=0;i<delete_list.size();i++)
        {
            //Deleted articles will always be present in the start_file only.
            dlist[start_archive.getEntryByPath(delete_list[i]).getIndex()]=1;
        }
        delete_list.clear();
        return;
    }

    zim::Uuid getUuid() const
    {
        return fileUid;
    }
    zim::writer::Url getMainPage() const
    {
        return mainUrl;
    }
    zim::writer::Url getLayoutPage() const
    {
        return layoutUrl;
    }

    void create(const std::string& fname)
    {
        startZimCreation(fname);

        //Add all articles in File_1 that have not ben deleted.
        std::string url="";
        for (unsigned int index = 0; index < start_archive.getEntryCount(); index++)
        {
            auto entry=start_archive.getEntryByPath(index);
            if(dlist[index]==1)
            {
                continue;
            }

            auto tempArticle = std::make_shared<Article>(entry);
            //If the article is also present in file_2
            try {
                entry = diff_archive.getEntryByPath(entry.getPath());
                tempArticle= std::make_shared<Article>(entry);
            } catch(...) {}
            //[FIXME] Handle redirection
            try {
                tempArticle->setRedirectUrl(zim::writer::Url(redirectList.at(entry.getPath())));
            } catch (...) {}
            //std::cout<<"\nArticle: "<<tempArticle->getNamespace()<<"/"<<tempArticle->getUrl();
            //std::cout<<"\nIndex: "<<tempArticle->getIdx();
            //getchar();
            addArticle(tempArticle);
        }

        //Now adding new articles in file_2
        for (unsigned int index =0; index<+diff_archive.getEntryCount(); index++)
        {
            auto entry=diff_archive.getEntryByPath(index);
            //If the article is already in file_1, it has been added.
            if(start_archive.hasEntryByPath(entry.getPath())||isAdditionalMetadata(entry.getPath()))
            {
                continue;
            }

            auto tempArticle = std::make_shared<Article>(entry);
            //std::cout<<"\nID: "<<id;
            try {
                tempArticle->setRedirectUrl(zim::writer::Url(redirectList.at(entry.getPath())));
            } catch (...) {}
            //std::cout<<"\nArticle: "<<tempArticle->getNamespace()<<"/"<<tempArticle->getUrl();
            //std::cout<<"\nIndex: "<<tempArticle->getIdx();
            //std::cout<<"\nIsredirect: "<<tempArticle->isRedirect();
            //getchar();
            addArticle(tempArticle);
        }

        finishZimCreation();
    }
};

void usage()
{
    std::cout<<"\nzimpatch computes the end_file using a start_file and a diff_file (made by zimdiff).\n"
      "\nUsage: zimpatch [start_file] [diff_file] [output file]"
      "\nOption: -v, --version    print software version\n";
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
    zim::Archive startArchive(startFileName);
    zim::Archive diffArchive(diffFileName);
    std::vector<std::string >additionalMetadata;
    additionalMetadata.resize(6);
    additionalMetadata[0]="M/dlist";
    additionalMetadata[1]="M/startfileuid";
    additionalMetadata[2]="M/endfileuid";
    additionalMetadata[3]="M/mainaurl";
    additionalMetadata[4]="M/layoutaurl";
    additionalMetadata[5]="M/redirectlist";

    //Search in the ZIM file if the above articles are present:
    for (unsigned int i=0; i<additionalMetadata.size(); i++)
    {
        if(!diffArchive.hasEntryByPath(additionalMetadata[i]))        //If the article was not found in the file.
            return false;
    }

    //Check the UID of startFile and the value stored in startfileuid
    const char *startfileUID1=startArchive.getUuid().data;
    std::string startFileUID2=diffArchive.getEntryByPath(additionalMetadata[1]).getItem().getData();
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
    // Parsing arguments. There will be only three arguments, so no
    // detailed parsing is required.
    for(int i=0; i<argc; i++)
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
    }
    if(argc<4)
    {
        std::cout<<"\n[ERROR] Not enough Arguments provided\n";
        usage();
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
