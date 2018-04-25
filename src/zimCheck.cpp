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
#include <string>
#include <vector>
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
            test_ = false;
            for (zim::File::const_iterator it = f.begin(); it != f.end(); ++it)
            {
                if(it -> getNamespace() == '-')
                    if(it -> getTitle() == "favicon.png" )
                        test_ = true;
            }
            if( test_ )
            {
                std::cout << "  [INFO] Favicon found." << std::endl;
            }
            else
            {
                std::cout << "  [ERROR] Favivon not found in ZIM file." << std::endl;
            }
            overall_status &= test_;
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
        //The entire file is parsed, and the articles are hashed and stores in separate linked lists(std::list), one each for each namespace.
        //The lists are then sorted according to the hashes, and the articles with the same hashes are compared.
        //A list of pairs of indexes of articles(those that have the same hash) are then created.
        //Once the list of articles are created, they are compared one by one to see if they have the same content.If they do, it is reported to the user.
        if( run_all || redundant_data || no_args)
        {
            std::cout << "[INFO] Searching for redundant articles.." << std::endl;
            test_ = false;
            int max = 0;
            int k;
            progress.reset(articleCount);
            for (zim::File::const_iterator it = f.begin(); it != f.end(); ++it)
            {
                progress.report();
                k= it -> getArticleSize();
                if( k > max )
                    max = k;
            }
            //Data Storage system

            //The hashes are stored as a pair: The first element in the pair- unsigned int contains the hash obtained from the adler32 function.
            //The second element, an int, contains the index of the article in the ZIM file.
            //   std::pair<unsigned int, int>

            std::vector< std::list< std::pair< unsigned int, int> > >hash_main;

            //Allocating Double Hash Tree.
            hash_main.resize( max + 1 );

            //List of Articles to be compared against
            std::pair< unsigned int, int > article;

            //Adding data to hash Tree.
            //std::cout << "Adding Data to Hash Tables from file..." << std::endl;
            int i = 0;
            std::string ar;
            zim::Blob bl;
            progress.reset(articleCount);
            int sz = 0;
            for (zim::File::const_iterator it = f.begin(); it != f.end(); ++it)
            {
                progress.report();
                bl = it-> getData();
                sz = bl.size();
                ar.clear();
                for(int j=0; j < sz; j++)
                    ar += bl.data()[j];
                article.first = adler32(ar);
                article.second = i;
                hash_main[ ar.size() ].push_back( article );
                i++;
            }
            //Checking through hash tree for redundancies.
            //Sorting the hash tree.
            //std::cout << " Sorting Hash Tree..." << std::endl;
            int hash_main_size = hash_main.size();
            progress.reset(hash_main_size);
            for(int i = 0; i < hash_main_size; i++)
            {
                progress.report();
                hash_main[i].sort();
            }
            std::vector< std::pair< int , int > > to_verify;     //Vector containing list of pairs of articles whose hash has been found to be the same.
            //The above list of pairs of articles will be compared directly for redundancies.


            progress.reset(hash_main_size);
            for(int i = 0; i< hash_main_size; i++)
            {
                progress.report();
                std::list< std::pair< unsigned int, int > >::iterator it = hash_main[i].begin();
                std::pair< unsigned int, int> prev;
                prev.second = it -> second;
                prev.first = it -> first;
                ++it;
                for (; it != hash_main[i].end(); ++it)
                {
                    if(it -> first == prev.first)
                    {
                        if(it -> second != prev.second)
                        {
                            to_verify.push_back( std::make_pair( it->second , prev.second ));
                        }
                    }
                    prev.second = it -> second;
                    prev.first = it -> first;
                }
            }
            test_ = true;
            //std::cout << "  Verifying Similar Articles for redundancies.." << std::endl;
            progress.reset(to_verify.size());
            std::string output_details;
            for(unsigned int i=0; i < to_verify.size(); i++)
            {
                progress.report();
                zim::File::const_iterator it = f.begin();
                std::string s1, s2;
                for(int k = 0; k< to_verify[i].first; k++)
                    ++it;
                s1 = it -> getPage();
                it = f.begin();
                for(int k = 0; k < to_verify[i].second; k++)
                    ++it;
                s2 = it -> getPage();
                if( s1 == s2 )
                {
                    test_ = false;
                    output_details += "    [ERROR] Articles ";
                    output_details += std::to_string( (long long)to_verify[i].first );
                    output_details += " and ";
                    output_details += std::to_string( (long long)to_verify[i].second );
                    output_details += " have the same content\n";
                }
            }
            if( test_ )
                std::cout << "  [INFO] No redundant articles found in ZIM file" << std::endl;
            else
            {
                std::cout << "  [ERROR] Redundant articles have been found in ZIM file." << std::endl;
                if( error_details )
                    std::cout << "  Details:\n" << output_details << std::endl;
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
            std::vector < std::vector <std::string> >titles;
            titles.resize( 256 );
            std::string ar;
            for ( zim::File::const_iterator it = f.begin(); it != f.end(); ++it)
            {
                titles[ ( int )it->getNamespace() ].push_back( it->getTitle() );
            }
            for(int i = 0; i < 256; i++)
            {
                std::sort( titles[i].begin() , titles[i].end() );
            }
            progress.reset(articleCount);
            test_ = true;
            std::string output_details;
            std::string previousLink;
            int previousIndex = -1;
            int index;
            std::vector < std::string > links;
            for (zim::File::const_iterator it = f.begin(); it != f.end(); ++it)
            {
                progress.report();
                if( it -> getMimeType() == "text/html")
                {
                    getLinks( it -> getPage() , &links);
                    for(unsigned int i = 0; i < links.size(); i++)
                    {
                        //std::cout<<"\n"<<links[i]<<std::flush;
                        links[i] = process_links( links[i] );
                        //std::cout<<"\n"<<links[i]<<std::flush;
                        if(isInternalUrl( &links[i] ) )
                        {
                            bool found = false;
                            int nm = ( int )( links[i] )[1];
                            if(std::binary_search( titles[ nm ].begin(), titles[nm].end(),( links[i]).substr( 3 )))
                                found = true;
                            if( !found)
                            {
                                if( error_details )
                                {
                                    index = it -> getIndex();
                                    if(( previousLink != links[i]) && ( previousIndex != index))
                                    {
                                        output_details += "    [ERROR] Article '";
                                        output_details += links[i];
                                        output_details += "' was not found. Linked in Article ";
                                        output_details += std::to_string(index ) + "\n";
                                        previousLink = links[i];
                                        previousIndex = index;
                                    }
                                }
                                test_ = false;
                            }
                        }
                    }
                }
            }
            if( test_ )
                std::cout << "  [INFO] All internal URLs are valid" << std::endl;
            else
            {
                std::cout << "  [ERROR] Invalid internal URLs found in ZIM file." << std::endl;
                if( error_details)
                {
                    std::cout << "  Details:\n" << output_details << std::endl;
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
                if( it -> getMimeType() == "text/html" )
                {
                    getDependencies( it -> getPage() , &links );
                    for(unsigned int i=0; i< links.size(); i++)
                    {
                        if( isExternalUrl( &links[i] ) )
                        {
                            test_ = false;
                            externalDependencyList.push_back( it -> getUrl() );
                        }
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
