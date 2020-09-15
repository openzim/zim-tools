#include "zimfilechecks.h"
#include "tools.h"

#include <regex>
#include <map>
#include <unordered_map>
#include <list>
#include <sstream>
#include <zim/file.h>
#include <zim/fileiterator.h>

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


void test_checksum(zim::File& f, ErrorLogger& reporter) {
    std::cout << "[INFO] Verifying Internal Checksum..." << std::endl;
    bool result = f.verify();
    reporter.setTestResult(TestType::CHECKSUM, result);
    if (!result) {
        std::cout << "  [ERROR] Wrong Checksum in ZIM file" << std::endl;
        std::ostringstream ss;
        ss << "ZIM File Checksum in file: " << f.getChecksum() << std::endl;
        reporter.addReportMsg(TestType::CHECKSUM, ss.str());
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
            reporter.addReportMsg(TestType::METADATA, meta);
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
        reporter.addReportMsg(TestType::MAIN_PAGE, ss.str());
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
          reporter.addReportMsg(TestType::EMPTY, ss.str());
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
            int nremptylinks = 0;
            for (const auto &l : links)
            {
                if (l.front() == '#' || l.front() == '?') continue;
                if (isInternalUrl(l) == false) continue;
                if (l.empty())
                {
                    nremptylinks++;
                    continue;
                }


                if (isOutofBounds(l, baseUrl))
                {
                    std::ostringstream ss;
                    ss << l << " is out of bounds. Article: " << it->getLongUrl();
                    reporter.addReportMsg(TestType::URL_INTERNAL, ss.str());
                    reporter.setTestResult(TestType::URL_INTERNAL, false);
                    continue;
                }

                auto normalized = normalize_link(l, baseUrl);
                filtered[normalized].push_back(l);
            }

            if (nremptylinks)
            {
                std::ostringstream ss;
                ss << "Found " << nremptylinks << " empty links in article: " << it->getLongUrl();
                reporter.addReportMsg(TestType::URL_INTERNAL, ss.str());
                reporter.setTestResult(TestType::URL_INTERNAL, false);
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
                        reporter.addReportMsg(TestType::URL_INTERNAL, ss.str());
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
                    reporter.addReportMsg(TestType::URL_EXTERNAL, ss.str());
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
                    reporter.addReportMsg(TestType::REDUNDANT, ss.str());
                }
            }
        }
    }
}
