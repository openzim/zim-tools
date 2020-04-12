/*
 * Copyright (C) Kiran Mathew Koshy
 * Copyright (C) Matthieu Gautier <mgautier@kymeria.fr>
 * Copyright (C) Emmanuel Engelhart <kelson@kiwix.org>
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
#include "version.h"

enum TestType {
    CHECKSUM,
    EMPTY,
    METADATA,
    FAVICON,
    MAIN_PAGE,
    REDUNDANT,
    URL_INTERNAL,
    URL_EXTERNAL,
    MIME,
    OTHER
};

enum StatusCode : int {
   PASS = 0,
   FAIL = 1,
   EXCEPTION = 2
};

static const char *errorString[OTHER+1] = {
    "Invalid checksum",
    "Missing metadata entries",
    "Missing favicon",
    "Missing mainpage",
    "Redundant data found",
    "Invalid internal links found",
    "Invalid external links found",
    "Incoherent mimeType found",
    "Other errors found"
};


class ErrorLogger {
  private:
    std::map<TestType, std::vector<std::string>> errors;
    bool testStatus[OTHER+1];

  public:
    ErrorLogger() {
        for (int testType=CHECKSUM; testType<=OTHER; ++testType) {
            testStatus[testType] = true;
        }
    }

    void setTestResult(TestType type, bool status) {
        testStatus[type] = status;
    }

    void addError(TestType type, const std::string& message) {
        errors[type].push_back(message);
    }

    void report(bool error_details) {
        for (int testType=CHECKSUM; testType<=OTHER; ++testType) {
            if (!testStatus[testType]) {
                std::cout << "[ERROR] " << errorString[testType] << " :" << std::endl;
                for (auto& msg: errors[(TestType)testType]) {
                    std::cout << "  " << msg << std::endl;
                }
            }
        }
    }

    bool overalStatus() {
        for (int testType=CHECKSUM; testType<=OTHER; ++testType) {
            if (!testStatus[testType]) {
                return false;
            }
        }
        return true;
    }
};


std::vector<std::string> getLinks(const std::string& page, bool withHref = true)           //Returns a vector of the links in a particular page. includes links under 'href' and 'src'
{
    const char* p = page.c_str();
    const char* linkStart;
    std::vector<std::string> links;

    while (*p) {
        if (withHref && strncmp(p, " href", 5) == 0) {
            p += 5;
        } else if (strncmp(p, " src", 4) == 0) {
            p += 4;
        } else {
            p += 1;
            continue;
        }

        while (*p == ' ')
            p += 1 ;
        if (*(p++) != '=')
            continue;
        while (*p == ' ')
            p += 1;
        char delimiter = *p++;
        if (delimiter != '\'' && delimiter != '"')
            continue;

        linkStart = p;
        // [TODO] Handle escape char
        while(*p != delimiter)
            p++;
        links.push_back(std::string(linkStart, p));
        p += 1;
    }
    return links;
}

std::vector<std::string> getDependencies(const std::string& page)           //Returns a vector of the links in a particular page. includes links under 'href' and 'src'
{
    return getLinks(page, false);
}

int adler32(std::string buf)                        //Adler32 Hash Function. Used to hash the BLOB data obtained from each article, for redundancy checks.
{                                                   //Please note that the adler32 hash function has a high number of collisions, and that the hash match is not taken as final.
    unsigned int s1 = 1;
    unsigned int s2 = 0;
    unsigned int sz=buf.size();
    for (size_t n = 0; n <sz; n++)
    {
        s1 = (s1 + buf[n]) % 65521;
        s2 = (s2 + s1) % 65521;
    }
    return (s2 << 16) | s1;
}

inline bool isExternalUrl(const std::string& input_string)
{
    // A string starting with "<scheme>://" or "geo:" or "tel:" or "javascript:" or "mailto:"
    static std::regex external_url_regex =
      std::regex("([^:/?#]+:\\/\\/|geo:|tel:|mailto:|javascript:).*",
                 std::regex_constants::icase);
    return std::regex_match(input_string, external_url_regex);
}

// Checks if a URL is an internal URL or not. Uses RegExp.
inline bool isInternalUrl(const std::string& input_string)
{
    return !isExternalUrl(input_string);
}

//Removes extra spaces from URLs. Usually done by the browser, so web authors sometimes tend to ignore it.
//Converts the %20 to space.Essential for comparing URLs.

std::string normalize_link(const std::string& input, const std::string& baseUrl)
{
    std::string output;
    output.reserve(baseUrl.size() + input.size() + 1);

    bool in_query = false;
    bool check_rel = false;
    const char* p = input.c_str();
    if ( *(p) == '/') {
      // This is an absolute url.
      p++;
    } else {
      //This is a relative url, use base url
      output = baseUrl;
      if (output.back() != '/')
          output += '/';
      check_rel = true;
    }

    //URL Decoding.
    while (*p)
    {
        if ( !in_query && check_rel ) {
            if (strncmp(p, "../", 3) == 0) {
                // We must go "up"
                // Remove the '/' at the end of output.
                output.resize(output.size()-1);
                // Remove the last part.
                auto pos = output.find_last_of('/');
                output.resize(pos==output.npos?0:pos);
                // Move after the "..".
                p += 2;
                check_rel = false;
                continue;
            }
            if (strncmp(p, "./", 2) == 0) {
                // We must simply skip this part
                // Simply move after the ".".
                p += 2;
                check_rel = false;
                continue;
            }
        }
        if ( *p == '#' )
            // This is a beginning of the #anchor inside a page. No need to decode more
            break;
        if ( *p == '%')
        {
            char ch;
            sscanf(p+1, "%2hhx", &ch);
            output += ch;
            p += 3;
            continue;
        }
        if ( *p == '?' ) {
            // We are in the query, so don't try to interprete '/' as path separator
            in_query = true;
        }
        if ( *p == '/') {
            check_rel = true;
            if (output.empty()) {
                // Do not add '/' at beginning of output
                p++;
                continue;
            }
        }
        output += *(p++);
    }
    return output;
}

void displayHelp()
{
    std::cout<<"\n"
             "zimcheck\n"
             "Written by : Kiran Mathew Koshy\n"
             "A to check the quality of a ZIM file\n."
             "To list the details of the error reported, add a flag -D.\n"
             "Usage:\n"
             "./zimcheckusage: ./zimcheck [options] zimfile\n"
             "options:\n"
             "-A , --all             run all tests. Default if no flags are given.\n"
             "-0 , --empty           Empty content\n"
             "-C , --checksum        Internal CheckSum Test\n"
             "-M , --metadata        MetaData Entries\n"
             "-F , --favicon         Favicon\n"
             "-P , --main            Main page\n"
             "-R , --redundant       Redundant data check\n"
             "-U , --url_internal    URL check - Internal URLs\n"
             "-X , --url_external    URL check - External URLs\n"
             "-E , --mime            MIME checks\n"
             "-D , --details         Details of error\n"
             "-B , --progress        Print progress report\n"
             "-H , --help            Displays Help\n"
             "-V , --version         Displays software version\n"
             "examples:\n"
             "./zimcheck -A wikipedia.zim\n"
             "./zimcheck --checksum --redundant wikipedia.zim\n"
             "./zimcheck -F -R wikipedia.zim\n"
             "./zimcheck -M --favicon wikipedia.zim\n";
    return;
}


void test_checksum(zim::File& f, ErrorLogger& reporter) {
    std::cout << "[INFO] Verifying Internal Checksum.. " << std::endl;
    bool result = f.verify();
    reporter.setTestResult(CHECKSUM, result);
    if( result )
        std::cout << "  [INFO] Internal checksum found correct" << std::endl;
    else
    {
        std::cout << "  [ERROR] Wrong Checksum in ZIM file" << std::endl;
        std::ostringstream ss;
        ss << "ZIM File Checksum in file: " << f.getChecksum() << std::endl;
        reporter.addError(CHECKSUM, ss.str());
    }
}


void test_metadata(const zim::File& f, ErrorLogger& reporter) {
    std::cout << "[INFO] Searching for metadata entries.." << std::endl;
    static const char* const test_meta[] = {
        "Title",
        "Creator",
        "Publisher",
        "Date",
        "Description",
        "Language"};
    for (auto &meta : test_meta) {
        auto article = f.getArticle('M', meta);
        if (!article.good()) {
            reporter.setTestResult(METADATA, false);
            reporter.addError(METADATA, meta);
        }
    }
}

void test_favicon(const zim::File& f, ErrorLogger& reporter) {
    std::cout << "[INFO] Searching for Favicon.." << std::endl;
    static const char* const favicon_paths[] = {"-/favicon.png", "I/favicon.png", "I/favicon", "-/favicon"};
    for (auto &path: favicon_paths) {
        auto article = f.getArticleByUrl(path);
        if (article.good()) {
            return;
        }
    }
    reporter.setTestResult(FAVICON, false);
}

void test_mainpage(const zim::File& f, ErrorLogger& reporter) {
    std::cout << "[INFO] Searching for main page.." << std::endl;
    zim::Fileheader fh=f.getFileheader();
    bool testok = true;
    if( !fh.hasMainPage() )
        testok = false;
    else if( fh.getMainPage() > fh.getArticleCount() )
        testok = false;
    reporter.setTestResult(MAIN_PAGE, testok);
    if (!testok) {
        std::ostringstream ss;
        ss << "Main Page Index stored in File Header: " << fh.getMainPage();
        reporter.addError(MAIN_PAGE, ss.str());
    }
}


void test_articles(const zim::File& f, ErrorLogger& reporter, ProgressBar progress,
                   bool redundant_data, bool url_check, bool url_check_external, bool empty_check) {
    std::cout << "[INFO] Verifying Articles' content.. " << std::endl;
    // Article are store in a map<hash, list<index>>.
    // So all article with the same hash will be stored in the same list.
    std::map<unsigned int, std::list<zim::article_index_type>> hash_main;

    std::string previousLink;
    int previousIndex = -1;

    progress.reset(f.getFileheader().getArticleCount());
    for (zim::File::const_iterator it = f.begin(); it != f.end(); ++it)
    {
        progress.report();

        if (it->getArticleSize() == 0 &&
            empty_check &&
            (it->getNamespace() == 'A' ||
             it->getNamespace() == 'I')) {
          std::ostringstream ss;
          ss << "Ewntry " << it->getLongUrl() << " is empty";
          reporter.addError(EMPTY, ss.str());
          reporter.setTestResult(EMPTY, false);
        }

        if (it->isRedirect() ||
            it->isLinktarget() ||
            it->isDeleted() ||
            it->getArticleSize() == 0 ||
            it->getNamespace() == 'M') {
            continue;
        }

        std::string data;
        if (redundant_data || it->getMimeType() == "text/html")
            data = it->getData();

        if(redundant_data)
            hash_main[adler32(data)].push_back( it->getIndex() );

        if (it->getMimeType() != "text/html")
            continue;

        if(url_check)
        {
            auto baseUrl = it->getLongUrl();
            auto pos = baseUrl.find_last_of('/');
            baseUrl.resize( pos==baseUrl.npos ? 0 : pos );

            auto links = getLinks(it->getData());
            for(auto olink: links)
            {
                if (olink.front() == '#')
                    continue;
                if (isInternalUrl(olink)) {
                    auto link = normalize_link(olink, baseUrl);
                    char nm = link[0];
                    std::string shortUrl(link.substr(2));
                    auto a = f.getArticle(nm, shortUrl);
                    if (!a.good())
                    {
                        int index = it->getIndex();
                        if ((previousLink != link) && (previousIndex != index) )
                        {
                            std::ostringstream ss;
                            ss << link << " (" << olink << ") was not found in article " << it->getLongUrl();
                            reporter.addError(URL_INTERNAL, ss.str());
                            previousLink = link;
                            previousIndex = index;
                        }
                        reporter.setTestResult(URL_INTERNAL, false);
                    }
                }
            }
        }

        if (url_check_external)
        {
            if (it->getMimeType() != "text/html")
                continue;

            auto links = getDependencies(it->getPage());
            for (auto &link: links)
            {
                if (isExternalUrl( link ))
                {
                    std::ostringstream ss;
                    ss << link << " is an external dependence in article " << it->getLongUrl();
                    reporter.addError(URL_EXTERNAL, ss.str());
                    reporter.setTestResult(URL_EXTERNAL, false);
                    break;
                }
            }
        }
    }

    if (redundant_data)
    {
        std::cout << "[INFO] Searching for redundant articles.." << std::endl;
        std::cout << "  Verifying Similar Articles for redundancies.." << std::endl;
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

                    reporter.setTestResult(REDUNDANT, false);
                    std::ostringstream ss;
                    ss << a1.getTitle() << " (idx " << a1.getIndex() << ") and "
                       << a2.getTitle() << " (idx " << a2.getIndex() << ")";
                    reporter.addError(REDUNDANT, ss.str());
                }
            }
        }
    }
}


int main (int argc, char **argv)
{
    // To calculate the total time taken by the program to run.
    time_t startTime,endTime;
    double  timeDiffference;
    time( &startTime);

    // The boolean values which will be used to store the output from
    // getopt_long().  These boolean values will be then read by the
    // program to execute the different parts of the program.

    bool run_all = false;
    bool checksum = false;
    bool metadata = false;
    bool favicon = false;
    bool main_page = false;
    bool redundant_data = false;
    bool url_check = false;
    bool url_check_external = false;
    bool empty_check = false;
    bool mime_check = false;
    bool error_details = false;
    bool no_args = true;
    bool help = false;

    std::string filename = "";
    ProgressBar progress(1);
    ErrorLogger error;

    StatusCode status_code = PASS;

    //Parsing through arguments using getopt_long(). Both long and short arguments are allowed.
    while (1)
    {
        static struct option long_options[] =
        {
            { "all",          no_argument, 0, 'A'},
            { "empty",        no_argument, 0, '0'},
            { "checksum",     no_argument, 0, 'C'},
            { "metadata",     no_argument, 0, 'M'},
            { "favicon",      no_argument, 0, 'F'},
            { "main",         no_argument, 0, 'P'},
            { "redundant",    no_argument, 0, 'R'},
            { "url_internal", no_argument, 0, 'U'},
            { "url_external", no_argument, 0, 'X'},
            { "mime",         no_argument, 0, 'E'},
            { "details",      no_argument, 0, 'D'},
            { "help",         no_argument, 0, 'H'},
            { "version",      no_argument, 0, 'V'},
            { 0, 0, 0, 0}
        };
        int option_index = 0;
        int c = getopt_long (argc, argv, "ACMFPRUXEDHBVacmfpruxedhbv",
                             long_options, &option_index);
        //c = getopt (argc, argv, "ACMFPRUXED");
        if(c == -1)
            break;
        switch (c)
        {
        case 'A':
        case 'a':
            run_all = true;
            no_args = false;
            break;
        case '0':
            empty_check = true;
            break;
        case 'C':
        case 'c':
            checksum = true;
            no_args = false;
            break;
        case 'M':
        case 'm':
            metadata = true;
            no_args = false;
            break;
        case 'B':
        case 'b':
            progress.set_progress_report(true);
            break;
        case 'F':
        case 'f':
            favicon = true;
            no_args = false;
            break;
        case 'P':
        case 'p':
            main_page = true;
            no_args = false;
            break;
        case 'R':
        case 'r':
            redundant_data = true;
            no_args = false;
            break;
        case 'U':
        case 'u':
            url_check = true;
            no_args = false;
            break;
        case 'X':
        case 'x':
            url_check_external = true;
            no_args = false;
            break;
        case 'E':
        case 'e':
            mime_check = true;
            no_args = false;
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
            {
              std::cerr<<"Option "<<(char)optopt<<" requires an argument.\n";
              displayHelp();
            }
            else if ( isprint (optopt) )
              std::cerr<<"Unknown option `"<<( char )optopt<<"'.\n";
            else
            {
                std::cerr<<"Unknown option\n";
                displayHelp();
            }
            return 1;
        case 'V':
        case 'v':
          version();
          return 0;
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
    if ( run_all || no_args )
    {
        checksum = metadata = favicon = main_page = redundant_data =
          url_check = url_check_external = mime_check = empty_check = true;
    }

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
        std::cout << "[INFO] Checking zim file " << filename << std::endl;
        zim::File f( filename );

        //Test 1: Internal Checksum
        if(checksum)
            test_checksum(f, error);

        //Test 2: Metadata Entries:
        //The file is searched for the compulsory metadata entries.
        if(metadata)
            test_metadata(f, error);

        //Test 3: Test for Favicon.
        if(favicon)
            test_favicon(f, error);


        //Test 4: Main Page Entry
        if(main_page)
            test_mainpage(f, error);

        /* Now we want to avoid to loop on the tests but on the article.
         *
         * If we loop of the tests we will have :
         *
         * for (test: tests) {
         *     for(article: articles) {
         *          data = article->getData();
         *          ...
         *     }
         * }
         *
         * And so we will get several the data of an article (and so decompression and so).
         * By looping on the articles first, we have :
         *
         * for (article: articles) {
         *     data = article->getData() {
         *     for (test: tests) {
         *         ...
         *     }
         * }
         */

        if ( redundant_data || url_check || url_check_external || empty_check )
          test_articles(f, error, progress, redundant_data, url_check, url_check_external, empty_check);


        //Test 8: Verifying MIME Types
        //MIME Checks is intended to verify that all the MIME types of all different articles are listed in the file header.
        //As of now, there is no method in the existing zimlib to get the list of MIME types listed in the file header.
        //A bug has been reported for the above problem, and once the bug is fixed, it will be used to add MIME checks to the zimcheck tool.
        /*
                if(mime_check)
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

        error.report(error_details);
        std::cout << "[INFO] Overall Test Status: ";
        if( error.overalStatus())
        {
            std::cout << "Pass" << std::endl;
            status_code = PASS;
        }
        else
        {
            std::cout << "Fail" << std::endl;
            status_code = FAIL;
        }
        time( &endTime );

        timeDiffference = difftime( endTime , startTime );
        std::cout << "[INFO] Total time taken by zimcheck: " << timeDiffference << " seconds."<<std::endl;

    }
    catch (const std::exception & e)
    {
        std::cerr << e.what() << std::endl;
        status_code = EXCEPTION;
    }

    return status_code;
}
