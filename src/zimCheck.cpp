/*
 * Copyright (C)  Kiran Mathew Koshy
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
#include <unistd.h>
#include <zim/file.h>
#include <getopt.h>
#include <zim/fileiterator.h>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <list>
#include <algorithm>
#include <regex>
#include <ctime>
#include "progress.h"
#include "zimCheck.h"

int main (int argc, char **argv)
{

    bool overall_status=true;       //Overall status of test- wether it is a fail or a pass.

    //To calculate the total time taken by the program to run.
    time_t startTime,endTime;
    double  timeDiffference;
    time( &startTime);

    //The boolean values which will be used to store the output from getopt_long().
    //These boolean values will be then read by the program to execute the different parts of the program.

    bool run_all = false;
    bool checksum = false;
    bool metadata = false;
    bool favicon = false;
    bool main_page = false;
    bool redundant_data = false;
    bool url_check = false;
    bool url_check_external = false;
    bool mime_check = false;
    bool error_details = false;
    bool no_args = false;
    bool help = false;
    int c;
    std::string filename = "";
    ProgressBar progress(1);
    opterr = 0;

    //Parsing through arguments using getopt_long(). Both long and short arguments are allowed.
    while (1)
    {
        static struct option long_options[] =
        {
            { "all",     no_argument,       0, 'A'},
            { "checksum",  no_argument,       0, 'C'},
            { "metadata",  no_argument,       0, 'M'},
            { "favicon",  no_argument,       0, 'F'},
            { "main",  no_argument,       0, 'P'},
            { "redundant",  no_argument,       0, 'R'},
            { "url_internal",  no_argument,       0, 'U'},
            { "url_external",  no_argument,       0, 'X'},
            { "mime",  no_argument,       0, 'E'},
            { "details",  no_argument,       0, 'D'},
            { "help",  no_argument,       0, 'H'},
            { "progress",  no_argument,       0, 'B'},
            { 0, 0, 0, 0}
        };
        int option_index = 0;
        c = getopt_long (argc, argv, "ACMFPRUXEDHBacmfpruxedhb",
                         long_options, &option_index);
        //c = getopt (argc, argv, "ACMFPRUXED");
        if(c == -1)
            break;
        switch (c)
        {
        case 'A':
        case 'a':
            run_all = true;
            break;
        case 'C':
        case 'c':
            checksum = true;
            break;
        case 'M':
        case 'm':
            metadata = true;
            break;
        case 'B':
        case 'b':
            progress.set_progress_report(true);
            break;
        case 'F':
        case 'f':
            favicon = true;
            break;
        case 'P':
        case 'p':
            main_page = true;
            break;
        case 'R':
        case 'r':
            redundant_data = true;
            break;
        case 'U':
        case 'u':
            url_check = true;
            break;
        case 'X':
        case 'x':
            url_check_external = true;
            break;
        case 'E':
        case 'e':
            mime_check = true;
            break;
        case 'D':
        case 'd':
            error_details = true;
            break;
        case 'H':
        case 'h':
            help=true;
            break;
        case '?':
            if (optopt == 'c')
                std::cerr<<"Option "<<(char)optopt<<" requires an argument.\n"
                         "options:\n"
                         "  -A , --all        run all tests. Default if no flags are given.\n"
                         "  -C        Internal CheckSum Test\n"
                         "  -M        MetaData Entries\n"
                         "  -F        Favicon\n"
                         "  -P        Main page\n"
                         "  -R        Redundant data check\n"
                         "  -U        URL checks\n"
                         "  -X        External Dependency check\n"
                         "  -E        MIME checks\n"
                         "  -D        Lists Details of the errors in the ZIM file.\n"
                         "\n"
                         "examples:\n"
                         "  " << argv[0] << " -A wikipedia.zim\n"
                         "  " << argv[0] << " -C wikipedia.zim\n"
                         "  " << argv[0] << " -F -R wikipedia.zim\n"
                         "  " << argv[0] << " -MI wikipedia.zim\n"
                         "  " << argv[0] << " -U wikipedia.zim\n"
                         "  " << argv[0] << " -R -U wikipedia.zim\n"
                         "  " << argv[0] << " -R -U -MI wikipedia.zim\n"
                         << std::flush;
            else if ( isprint (optopt) )
                std::cerr<<"Unknown option `"<<( char )optopt<<"'.\n";
            else
            {
                std::cerr<<"Unknown option\n";
                displayHelp();
            }
            return 1;
        default:
            abort ();
        }
    }

    //Displaying Help for --help argument
    if(help)
    {
        displayHelp();
        return -1;
    }

    //If no arguments are given to the program, all the tests are performed.
    if( (run_all || checksum || metadata || favicon || main_page || redundant_data || url_check || url_check_external || mime_check || help)==false)
        no_args = true;

    //Obtaining filename from argument list
    filename = "";
    for(int i = 0; i < argc; i++)
    {
        if( (argv[i][0] != '-') && (i != 0))
        {
            filename = argv[i];
        }
    }
    if(filename == "")
    {
        std::cerr<<"No file provided as argument\n";
        displayHelp();
        return -1;
    }
    //Tests.
    try
    {
        bool test_ = false;
        std::cout << "[INFO] Checking zim file " << filename << std::endl;
        zim::File f( filename );
        int articleCount = f.getFileheader().getArticleCount();
        //Test 1: Internal Checksum
        if( run_all || checksum || no_args)
        {
            std::cout << "[INFO] Verifying Internal Checksum.. " << std::endl;
            if( f.verify() )
                std::cout << "  [INFO] Internal checksum found correct" << std::endl;
            else
            {
                std::cout << "  [ERROR] Wrong Checksum in ZIM file" << std::endl;
                if(error_details)
                {
                    std::cout << "  [ERROR] ZIM File Checksum in file: " << f.getChecksum() << std::endl;
                }
            }
            overall_status&=test_;
        }

        //Test 2: Metadata Entries:
        //The file is searched for the compulsory metadata entries.
        if( run_all || metadata || no_args)
        {
            std::cout << "[INFO] Searching for metadata entries.." << std::endl;
            static const char* const test_meta[] = {
                "Title",
                "Creator",
                "Publisher",
                "Date",
                "Description",
                "Language"};
            bool missing_meta_data = false;
            for (auto &meta : test_meta) {
              auto article = f.getArticle('M', meta);
              if (!article.good()) {
                if (!missing_meta_data) {
                  std::cout << "  [ERROR] Missing metadata entries" << std::endl;
                  missing_meta_data = true;
                }
                std::cout << "  [ERROR] " << meta << " not found in Metadata" << std::endl;
              }
            }
            if (!missing_meta_data) {
              std::cout << "  [INFO] All essential metadata entries found" << std::endl;
            }
            overall_status &= !missing_meta_data;
        }

        //Test 3: Test for Favicon.
        if( run_all || favicon || no_args )
        {
            std::cout << "[INFO] Searching for Favicon.." << std::endl;
            bool found = false;
            static const char* const favicon_paths[] = {"-/favicon.png", "I/favicon.png", "I/favicon", "-/favicon"};
            for (auto &path: favicon_paths) {
              auto article = f.getArticleByUrl(path);
              if (article.good()) {
                found = true;
                break;
              }
            }
            if( found )
            {
                std::cout << "  [INFO] Favicon found." << std::endl;
            }
            else
            {
                std::cout << "  [ERROR] Favivon not found in ZIM file." << std::endl;
            }
            overall_status &= found;
        }


        //Test 4: Main Page Entry
        if( run_all || main_page || no_args)
        {
            test_ = true;
            std::cout << "[INFO] Searching for main page.." << std::endl;
            zim::Fileheader fh=f.getFileheader();
            if( !fh.hasMainPage() )
                test_ = false;
            else
            {
                if( fh.getMainPage() > fh.getArticleCount() )
                    test_ = false;
            }
            if( test_ )
                std::cout << "  [INFO] Main page found in ZIM file." << std::endl;
            else
            {
                std::cout << "  [ERROR] Main page article not found in file." << std::endl;
                if( error_details )
                {
                    std::cout << "  [ERROR] Main Page Index stored in File Header: " << fh.getMainPage() << std::endl;
                }
            }
            overall_status &= test_;
        }


        //Test 5: Redundant Data:
        //The entire file is parsed, and the articles are hashed and stores in separate hashmap of lists.
        //Article with the same hashes are then compared.
        //If they have the same content it is reported to the user.
        if( run_all || redundant_data || no_args)
        {
            std::cout << "[INFO] Searching for redundant articles.." << std::endl;
            test_ = false;
            // Article are store in a map<hash, list<index>>.
            // So all article with the same hash will be stored in the same list.
            std::map<unsigned int, std::list<zim::article_index_type>> hash_main;

            //Adding data to hash Tree.
            std::cout << "  Adding Data to Hash Tables from file..." << std::endl;
            progress.reset(articleCount);
            for (zim::File::const_iterator it = f.begin(); it != f.end(); ++it)
            {
                progress.report();
                if (it->isRedirect() ||
                    it->isLinktarget() ||
                    it->isDeleted() ||
                    it->getArticleSize() == 0 ||
                    it->getNamespace() == 'M') {
                    continue;
                }
                hash_main[ adler32(it->getData()) ].push_back( it->getIndex() );
            }
            std::cout << "  Verifying Similar Articles for redundancies.." << std::endl;
            test_ = true;
            std::ostringstream output_details;
            progress.reset(hash_main.size());
            for(auto &it: hash_main)
            {
                progress.report();
                auto l = it.second;
                // If only one article has this hash, no need to test.
                if(l.size() <= 1)
                    continue;
                for (auto current=l.begin(); current!=l.end(); current++) {
                    auto a1 = f.getArticle(*current);
                    std::string s1 = a1.getData();
                    for(auto other=std::next(current); other!=l.end(); other++) {
                        auto a2 = f.getArticle(*other);
                        std::string s2 = a2.getData();
			if (s1 != s2 )
                            continue;
                        test_ = false;
                        output_details << "    [ERROR] Articles "
                                       << a1.getTitle()
                                       << " (idx " << a1.getIndex()
                                       << ") and "
                                       << a2.getTitle()
                                       << " (idx " << a2.getIndex()
                                       << ") have the same content\n";
                    }
                }
            }
            if( test_ )
                std::cout << "  [INFO] No redundant articles found in ZIM file" << std::endl;
            else
            {
                std::cout << "  [ERROR] Redundant articles have been found in ZIM file." << std::endl;
                if( error_details )
                    std::cout << "  Details:\n" << output_details.str() << std::endl;
            }
            overall_status &= test_;
        }


        //Test 6: Verifying Internal URLs
        //All internal URLS are parsed, and compared with the existing articles in the zim file.
        //If the internal URL is not valid, an error is reported.
        //A list of Titles are collected from the file, and stored as a hash, similar to the way the hash is stored for redundancy check.
        //Each URL obtained is compared with the hash.


        if( run_all || url_check || no_args)
        {
            std::cout << "[INFO] Verifying internal URLs.." << std::endl;
            std::vector<std::string> urls[256];
            std::string ar;
            for ( zim::File::const_iterator it = f.begin(); it != f.end(); ++it)
            {
                urls[(int)it->getNamespace()].push_back( it->getUrl() );
            }
            for(auto& urlV: urls)
            {
                std::sort( urlV.begin() , urlV.end() );
            }
            progress.reset(articleCount);
            test_ = true;
            std::ostringstream output_details;
            std::string previousLink;
            int previousIndex = -1;
            std::vector<std::string> links;
            for (zim::File::const_iterator it = f.begin(); it != f.end(); ++it)
            {
                progress.report();

                if( it->isRedirect() || it->isLinktarget() || it->isDeleted())
                    continue;
                if( it->getMimeType() != "text/html")
                    continue;

                auto baseUrl = it->getLongUrl();
                auto pos = baseUrl.find_last_of('/');
                baseUrl.resize(pos==baseUrl.npos?0:pos);

                getLinks(it->getData(), &links);
                for(auto link: links)
                {
                    if(link.front() == '#')
                        continue;
                    if(isInternalUrl(link)) {
                        link = normalize_link(link, baseUrl);
                        char nm = link[0];
                        std::string shortUrl(link.substr(2));
                        auto& urlV = urls[(int)nm];
                        bool found = std::binary_search(urlV.begin(), urlV.end(), shortUrl);
                        if( !found)
                        {
                            if( error_details )
                            {
                                int index = it->getIndex();
                                if( (previousLink != link) && (previousIndex != index) )
                                {
                                    output_details << "    [ERROR] Article '"
                                                   << link
                                                   << "' was not found. Linked in Article "
                                                   << index  << "\n";
                                    previousLink = link;
                                    previousIndex = index;
                                }
                            }
                            test_ = false;
                        }
                    }
                }
            }
            if( test_ ) {
                std::cout << "  [INFO] All internal URLs are valid" << std::endl;
            } else {
                std::cout << "  [ERROR] Invalid internal URLs found in ZIM file." << std::endl;
                if( error_details) {
                    std::cout << "  Details:\n" << output_details.str() << std::endl;
                }
            }
            overall_status &= test_;
        }


        //Test 7: Checking for external Dependencies.
        //All external URLs are parsed, and non-wikipedia URLs are reported.
        if( run_all || url_check_external || no_args )
        {
            std::cout << "[INFO] Searching for External Dependencies:"<< std::endl;
            progress.reset(articleCount);
            std::list < std::string > externalDependencyList;
            test_ = true ;
            std::vector< std::string > links;
            for (zim::File::const_iterator it = f.begin(); it != f.end(); ++it)
            {
                progress.report();

                if( it->isRedirect() || it->isLinktarget() || it->isDeleted())
                    continue;
                if( it->getMimeType() != "text/html" )
                    continue;

                getDependencies( it -> getPage() , &links );
                for(auto &link: links)
                {
                    if( isExternalUrl( link ) )
                    {
                        test_ = false;
                        externalDependencyList.push_back( it -> getUrl() );
                    }
                }
            }
            if( test_)
                std::cout << "  [INFO] No external dependencies found." << std::endl;
            else
            {
                std::cout << "  [ERROR] External Dependencies were found in file." << std::endl;
                if(error_details)
                {
                    externalDependencyList.sort();
                    std::list< std::string >::iterator prev = externalDependencyList.begin();
                    std::cout << "    [ERROR] External dependencies were found in article '" << *prev << "'" << std::endl;
                    for(std::list < std::string >::iterator i = ( ++prev ); i != externalDependencyList.end(); ++i)
                    {
                        if( *i != *prev )
                            std::cout << "    [ERROR] External dependencies were found in article '" << *i << "'" << std::endl;
                        prev = i;
                    }
                }
            }
            overall_status &= test_;
        }

        //Test 8: Verifying MIME Types
        //MIME Checks is intended to verify that all the MIME types of all different articles are listed in the file header.
        //As of now, there is no method in the existing zimlib to get the list of MIME types listed in the file header.
        //A bug has been reported for the above problem, and once the bug is fixed, it will be used to add MIME checks to the zimcheck tool.
        /*
                if(run_all||mime_check||no_args)
                {
                    std::cout<<"\nTest 8: Verifying MIME Types.. \n"<<std::flush;
                    progress.reset(articleCount);
                    test_=true;
                    for (zim::File::const_iterator it = f.begin(); it != f.end(); ++it)
                    {
                        progress.report();
                    }
                    if(test_)
                        std::cout<<"\nPass\n";
                    else
                    {
                        std::cout<<"\nFail\n";
                    }
                }
        */
        std::cout << "[INFO] Overall Test Status: ";
        if( overall_status)
            std::cout << "Pass" << std::endl;
        else
            std::cout << "Fail" << std::endl;
        time( &endTime );

        timeDiffference = difftime( endTime , startTime );
        std::cout << "[INFO] Total time taken by zimcheck: " << timeDiffference << " seconds."<<std::endl;

    }
    catch (const std::exception & e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
