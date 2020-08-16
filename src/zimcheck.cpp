/*
 * Copyright (C) 2006 Tommi Maekitalo
 * Copyright (C) Kiran Mathew Koshy
 * Copyright (C) Matthieu Gautier <mgautier@kymeria.fr>
 * Copyright (C) Emmanuel Engelhart <kelson@kiwix.org>
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
#include <unordered_map>

#include "progress.h"
#include "version.h"
#include "tools.h"

enum StatusCode : int {
   PASS = 0,
   FAIL = 1,
   EXCEPTION = 2
};

enum class LogTag { ERROR, WARNING };

// Specialization of std::hash needed for our unordered_map. Can be removed in c++14
namespace std {
  template <> struct hash<LogTag> {
    size_t operator() (const LogTag &t) const { return size_t(t); }
  };
}

static std::unordered_map<LogTag, std::string> tagToStr{ {LogTag::ERROR,     "ERROR"},
                                                         {LogTag::WARNING,   "WARNING"}};

enum class TestType {
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

// Specialization of std::hash needed for our unordered_map. Can be removed in c++14
namespace std {
  template <> struct hash<TestType> {
    size_t operator() (const TestType &t) const { return size_t(t); }
  };
}

static std::unordered_map<TestType, std::pair<LogTag, std::string>> errormapping = {
    { TestType::CHECKSUM,      {LogTag::ERROR, "Invalid checksum"}},
    { TestType::EMPTY,         {LogTag::ERROR, "Empty articles"}},
    { TestType::METADATA,      {LogTag::ERROR, "Missing metadata entries"}},
    { TestType::FAVICON,       {LogTag::ERROR, "Missing favicon"}},
    { TestType::MAIN_PAGE,     {LogTag::ERROR, "Missing mainpage"}},
    { TestType::REDUNDANT,     {LogTag::WARNING, "Redundant data found"}},
    { TestType::URL_INTERNAL,  {LogTag::ERROR, "Invalid internal links found"}},
    { TestType::URL_EXTERNAL,  {LogTag::ERROR, "Invalid external links found"}},
    { TestType::MIME,       {LogTag::ERROR, "Incoherent mimeType found"}},
    { TestType::OTHER,      {LogTag::ERROR, "Other errors found"}}
};

class ErrorLogger {
  private:
    std::unordered_map<TestType, std::vector<std::string>> errors;
    std::unordered_map<TestType, bool> testStatus;

  public:
    ErrorLogger()
    {
        for (const auto &m : errormapping) {
            testStatus[m.first] = true;
        }
    }

    void setTestResult(TestType type, bool status) {
        testStatus[type] = status;
    }

    void addError(TestType type, const std::string& message) {
        errors[type].push_back(message);
    }

    void report(bool error_details) const {
        for (auto status : testStatus) {
            if (status.second == false) {
                auto &p = errormapping[status.first];
                std::cout << "[" + tagToStr[p.first] + "] " << p.second << ":" << std::endl;
                for (auto& msg: errors.at(status.first)) {
                    std::cout << "  " << msg << std::endl;
                }
            }
        }
    }

    inline bool overalStatus() const {
        return std::all_of(testStatus.begin(), testStatus.end(), [](std::pair<TestType, bool> e){ return e.second == true;});
    }
};


std::vector<std::string> getDependencies(const std::string& page)           //Returns a vector of the links in a particular page. includes links under 'href' and 'src'
{
    return generic_getLinks(page, false);
}

inline bool isDataUrl(const std::string& input_string)
{
    static std::regex data_url_regex =
      std::regex("data:.+", std::regex_constants::icase);
    return std::regex_match(input_string, data_url_regex);
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
    return !isExternalUrl(input_string) && !isDataUrl(input_string);
}

void displayHelp()
{
    std::cout<<"\n"
             "zimcheck checks the quality of a ZIM file.\n\n"
             "Usage: zimcheck [options] zimfile\n"
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
             "zimcheck -A wikipedia.zim\n"
             "zimcheck --checksum --redundant wikipedia.zim\n"
             "zimcheck -F -R wikipedia.zim\n"
             "zimcheck -M --favicon wikipedia.zim\n";
    return;
}


void test_checksum(zim::File& f, ErrorLogger& reporter) {
    std::cout << "[INFO] Verifying Internal Checksum..." << std::endl;
    bool result = f.verify();
    reporter.setTestResult(TestType::CHECKSUM, result);
    if (!result) {
        std::cout << "  [ERROR] Wrong Checksum in ZIM file" << std::endl;
        std::ostringstream ss;
        ss << "ZIM File Checksum in file: " << f.getChecksum() << std::endl;
        reporter.addError(TestType::CHECKSUM, ss.str());
    }
}


void test_metadata(const zim::File& f, ErrorLogger& reporter) {
    std::cout << "[INFO] Searching for metadata entries..." << std::endl;
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
            reporter.setTestResult(TestType::METADATA, false);
            reporter.addError(TestType::METADATA, meta);
        }
    }
}

void test_favicon(const zim::File& f, ErrorLogger& reporter) {
    std::cout << "[INFO] Searching for Favicon..." << std::endl;
    static const char* const favicon_paths[] = {"-/favicon.png", "I/favicon.png", "I/favicon", "-/favicon"};
    for (auto &path: favicon_paths) {
        auto article = f.getArticleByUrl(path);
        if (article.good()) {
            return;
        }
    }
    reporter.setTestResult(TestType::FAVICON, false);
}

void test_mainpage(const zim::File& f, ErrorLogger& reporter) {
    std::cout << "[INFO] Searching for main page..." << std::endl;
    zim::Fileheader fh=f.getFileheader();
    bool testok = true;
    if( !fh.hasMainPage() )
        testok = false;
    else if( fh.getMainPage() > fh.getArticleCount() )
        testok = false;
    reporter.setTestResult(TestType::MAIN_PAGE, testok);
    if (!testok) {
        std::ostringstream ss;
        ss << "Main Page Index stored in File Header: " << fh.getMainPage();
        reporter.addError(TestType::MAIN_PAGE, ss.str());
    }
}


void test_articles(const zim::File& f, ErrorLogger& reporter, ProgressBar progress,
                   bool redundant_data, bool url_check, bool url_check_external, bool empty_check) {
    std::cout << "[INFO] Verifying Articles' content..." << std::endl;
    // Article are store in a map<hash, list<index>>.
    // So all article with the same hash will be stored in the same list.
    std::map<unsigned int, std::list<zim::article_index_type>> hash_main;

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
          ss << "Entry " << it->getLongUrl() << " is empty";
          reporter.addError(TestType::EMPTY, ss.str());
          reporter.setTestResult(TestType::EMPTY, false);
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

            auto links = generic_getLinks(it->getData());
            std::unordered_map<std::string, std::vector<std::string>> filtered;
            for (const auto &l : links)
            {
                if (l.front() == '#' || l.front() == '?') continue;
                if (isInternalUrl(l) == false) continue;

                if (isOutofBounds(l, baseUrl))
                {
                    std::ostringstream ss;
                    ss << l << " is out of bounds. Article: " << it->getLongUrl();
                    reporter.addError(TestType::URL_INTERNAL, ss.str());
                    continue;
                }

                auto normalized = normalize_link(l, baseUrl);
                filtered[normalized].push_back(l);
            }

            for(const auto &p: filtered)
            {
                std::string link = p.first;
                char nm = link[0];
                std::string shortUrl(link.substr(2));
                auto a = f.getArticle(nm, shortUrl);
                if (!a.good())
                {
                    int index = it->getIndex();
                    if (previousIndex != index)
                    {
                        std::ostringstream ss;
                        ss << "The following links:\n";
                        for (const auto &olink : p.second)
                            ss << "- " << olink << '\n';
                        ss << "(" << link << ") were not found in article " << it->getLongUrl();
                        reporter.addError(TestType::URL_INTERNAL, ss.str());
                        previousIndex = index;
                    }
                    reporter.setTestResult(TestType::URL_INTERNAL, false);
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
                    reporter.addError(TestType::URL_EXTERNAL, ss.str());
                    reporter.setTestResult(TestType::URL_EXTERNAL, false);
                    break;
                }
            }
        }
    }

    if (redundant_data)
    {
        std::cout << "[INFO] Searching for redundant articles..." << std::endl;
        std::cout << "  Verifying Similar Articles for redundancies..." << std::endl;
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

                    reporter.setTestResult(TestType::REDUNDANT, false);
                    std::ostringstream ss;
                    ss << a1.getTitle() << " (idx " << a1.getIndex() << ") and "
                       << a2.getTitle() << " (idx " << a2.getIndex() << ")";
                    reporter.addError(TestType::REDUNDANT, ss.str());
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
