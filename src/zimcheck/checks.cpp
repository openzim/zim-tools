#include "checks.h"
#include "../tools.h"

#include <map>
#include <unordered_map>
#include <list>
#include <sstream>
#define ZIM_PRIVATE
#include <zim/archive.h>
#include <zim/item.h>

namespace
{

std::unordered_map<LogTag, std::string> tagToStr = {
    {LogTag::ERROR,     "ERROR"},
    {LogTag::WARNING,   "WARNING"}
};

std::unordered_map<TestType, std::pair<LogTag, std::string>> errormapping = {
    { TestType::CHECKSUM,      {LogTag::ERROR, "Invalid checksum"}},
    { TestType::INTEGRITY,     {LogTag::ERROR, "Invalid low-level structure"}},
    { TestType::EMPTY,         {LogTag::ERROR, "Empty articles"}},
    { TestType::METADATA,      {LogTag::ERROR, "Missing metadata entries"}},
    { TestType::FAVICON,       {LogTag::ERROR, "Missing favicon"}},
    { TestType::MAIN_PAGE,     {LogTag::ERROR, "Missing mainpage"}},
    { TestType::REDUNDANT,     {LogTag::WARNING, "Redundant data found"}},
    { TestType::URL_INTERNAL,  {LogTag::ERROR, "Invalid internal links found"}},
    { TestType::URL_EXTERNAL,  {LogTag::ERROR, "Invalid external links found"}},
    { TestType::REDIRECT,      {LogTag::ERROR, "Redirect loop(s) exist"}},
    { TestType::OTHER,      {LogTag::ERROR, "Other errors found"}}
};

} // unnamed namespace

ErrorLogger::ErrorLogger(bool _jsonOutputMode)
  : reportMsgs(size_t(TestType::COUNT))
  , jsonOutputMode(_jsonOutputMode)
{
    testStatus.set();
    if ( jsonOutputMode ) {
      std::cout << "{" << std::flush;
    }
}

ErrorLogger::~ErrorLogger()
{
    if ( jsonOutputMode ) {
      std::cout << "\n}" << std::endl;
    }
}

void ErrorLogger::infoMsg(const std::string& msg) const {
  if ( !jsonOutputMode ) {
    std::cout << msg << std::endl;
  }
}

void ErrorLogger::setTestResult(TestType type, bool status) {
    testStatus[size_t(type)] = status;
}

void ErrorLogger::addReportMsg(TestType type, const std::string& message) {
    reportMsgs[size_t(type)].push_back(message);
}

void ErrorLogger::report(bool error_details) const {
    if ( !jsonOutputMode ) {
        for ( size_t i = 0; i < size_t(TestType::COUNT); ++i ) {
            const auto& testmsg = reportMsgs[i];
            if ( !testStatus[i] ) {
                auto &p = errormapping[TestType(i)];
                std::cout << "[" + tagToStr[p.first] + "] " << p.second << ":" << std::endl;
                for (auto& msg: testmsg) {
                    std::cout << "  " << msg << std::endl;
                }
            }
        }
    }
}

bool ErrorLogger::overallStatus() const {
    for ( size_t i = 0; i < size_t(TestType::COUNT); ++i ) {
        if (errormapping[TestType(i)].first == LogTag::ERROR) {
            if ( testStatus[i] == false ) {
                return false;
            }
        }
    }
    return true;
}


void test_checksum(zim::Archive& archive, ErrorLogger& reporter) {
    reporter.infoMsg("[INFO] Verifying Internal Checksum...");
    bool result = archive.check();
    reporter.setTestResult(TestType::CHECKSUM, result);
    if (!result) {
        reporter.infoMsg("  [ERROR] Wrong Checksum in ZIM archive");
        std::ostringstream ss;
        ss << "ZIM Archive Checksum in archive: " << archive.getChecksum() << std::endl;
        reporter.addReportMsg(TestType::CHECKSUM, ss.str());
    }
}

void test_integrity(const std::string& filename, ErrorLogger& reporter) {
    reporter.infoMsg("[INFO] Verifying ZIM-archive structure integrity...");
    zim::IntegrityCheckList checks;
    checks.set(); // enable all checks (including checksum)
    bool result = zim::validate(filename, checks);
    reporter.setTestResult(TestType::INTEGRITY, result);
    if (!result) {
        reporter.infoMsg("  [ERROR] ZIM file's low level structure is invalid");
    }
}


void test_metadata(const zim::Archive& archive, ErrorLogger& reporter) {
    reporter.infoMsg("[INFO] Searching for metadata entries...");
    static const char* const test_meta[] = {
        "Title",
        "Creator",
        "Publisher",
        "Date",
        "Description",
        "Language"};
    auto existing_metadata = archive.getMetadataKeys();
    auto begin = existing_metadata.begin();
    auto end = existing_metadata.end();
    for (auto &meta : test_meta) {
        if (std::find(begin, end, meta) == end) {
            reporter.setTestResult(TestType::METADATA, false);
            reporter.addReportMsg(TestType::METADATA, meta);
        }
    }
}

void test_favicon(const zim::Archive& archive, ErrorLogger& reporter) {
    reporter.infoMsg("[INFO] Searching for Favicon...");
    static const char* const favicon_paths[] = {"-/favicon.png", "I/favicon.png", "I/favicon", "-/favicon"};
    for (auto &path: favicon_paths) {
        if (archive.hasEntryByPath(path)) {
            return;
        }
    }
    reporter.setTestResult(TestType::FAVICON, false);
}

void test_mainpage(const zim::Archive& archive, ErrorLogger& reporter) {
    reporter.infoMsg("[INFO] Searching for main page...");
    bool testok = true;
    try {
      archive.getMainEntry();
    } catch(...) {
        testok = false;
    }
    reporter.setTestResult(TestType::MAIN_PAGE, testok);
    if (!testok) {
        std::ostringstream ss;
        ss << "Main Page Index stored in Archive Header: " << archive.getMainEntryIndex();
        reporter.addReportMsg(TestType::MAIN_PAGE, ss.str());
    }
}


void test_articles(const zim::Archive& archive, ErrorLogger& reporter, ProgressBar progress,
                   const EnabledTests checks) {
    reporter.infoMsg("[INFO] Verifying Articles' content...");
    // Article are store in a map<hash, list<index>>.
    // So all article with the same hash will be stored in the same list.
    std::map<unsigned int, std::list<zim::entry_index_type>> hash_main;

    int previousIndex = -1;

    progress.reset(archive.getEntryCount());
    for (auto& entry:archive.iterEfficient()) {
        progress.report();
        auto path = entry.getPath();
        char ns = archive.hasNewNamespaceScheme() ? 'C' : path[0];

        if (entry.isRedirect() || ns == 'M') {
            continue;
        }

        if (checks.isEnabled(TestType::EMPTY) && (ns == 'C' || ns=='A' || ns == 'I')) {
            auto item = entry.getItem();
            if (item.getSize() == 0) {
                std::ostringstream ss;
                ss << "Entry " << path << " is empty";
                reporter.addReportMsg(TestType::EMPTY, ss.str());
                reporter.setTestResult(TestType::EMPTY, false);
            }
        }

        auto item = entry.getItem();

        if (item.getSize() == 0) {
            continue;
        }

        std::string data;
        if (checks.isEnabled(TestType::REDUNDANT) || item.getMimetype() == "text/html")
            data = item.getData();

        if(checks.isEnabled(TestType::REDUNDANT))
            hash_main[adler32(data)].push_back( item.getIndex() );

        if (item.getMimetype() != "text/html")
            continue;

        std::vector<html_link> links;
        if (checks.isEnabled(TestType::URL_INTERNAL) ||
            checks.isEnabled(TestType::URL_EXTERNAL)) {
            links = generic_getLinks(data);
        }

        if(checks.isEnabled(TestType::URL_INTERNAL))
        {
            auto baseUrl = path;
            auto pos = baseUrl.find_last_of('/');
            baseUrl.resize( pos==baseUrl.npos ? 0 : pos );

            std::unordered_map<std::string, std::vector<std::string>> filtered;
            int nremptylinks = 0;
            for (const auto &l : links)
            {
                if (l.link.front() == '#' || l.link.front() == '?') continue;
                if (l.isInternalUrl() == false) continue;
                if (l.link.empty())
                {
                    nremptylinks++;
                    continue;
                }


                if (isOutofBounds(l.link, baseUrl))
                {
                    std::ostringstream ss;
                    ss << l.link << " is out of bounds. Article: " << path;
                    reporter.addReportMsg(TestType::URL_INTERNAL, ss.str());
                    reporter.setTestResult(TestType::URL_INTERNAL, false);
                    continue;
                }

                auto normalized = normalize_link(l.link, baseUrl);
                filtered[normalized].push_back(l.link);
            }

            if (nremptylinks)
            {
                std::ostringstream ss;
                ss << "Found " << nremptylinks << " empty links in article: " << path;
                reporter.addReportMsg(TestType::URL_INTERNAL, ss.str());
                reporter.setTestResult(TestType::URL_INTERNAL, false);
            }

            for(const auto &p: filtered)
            {
                const std::string link = p.first;
                if (!archive.hasEntryByPath(link)) {
                    int index = item.getIndex();
                    if (previousIndex != index)
                    {
                        std::ostringstream ss;
                        ss << "The following links:\n";
                        for (const auto &olink : p.second)
                            ss << "- " << olink << '\n';
                        ss << "(" << link << ") were not found in article " << path;
                        reporter.addReportMsg(TestType::URL_INTERNAL, ss.str());
                        previousIndex = index;
                    }
                    reporter.setTestResult(TestType::URL_INTERNAL, false);
                }
            }
        }

        if (checks.isEnabled(TestType::URL_EXTERNAL))
        {
            for (const auto &l: links)
            {
                if (l.attribute == "src" && l.isExternalUrl())
                {
                    std::ostringstream ss;
                    ss << l.link << " is an external dependence in article " << path;
                    reporter.addReportMsg(TestType::URL_EXTERNAL, ss.str());
                    reporter.setTestResult(TestType::URL_EXTERNAL, false);
                    break;
                }
            }
        }
    }

    if (checks.isEnabled(TestType::REDUNDANT))
    {
        reporter.infoMsg("[INFO] Searching for redundant articles...");
        reporter.infoMsg("  Verifying Similar Articles for redundancies...");
        std::ostringstream output_details;
        progress.reset(hash_main.size());
        for(const auto &it: hash_main)
        {
            progress.report();
            auto l = it.second;
            while ( !l.empty() ) {
                const auto e1 = archive.getEntryByPath(l.front());
                l.pop_front();
                if ( !l.empty() ) {
                    // The way we have constructed `l`, e1 MUST BE an item
                    const std::string s1 = e1.getItem().getData();
                    decltype(l) articlesDifferentFromE1;
                    for(auto other : l) {
                        auto e2 = archive.getEntryByPath(other);
                        std::string s2 = e2.getItem().getData();
                        if (s1 != s2 ) {
                            articlesDifferentFromE1.push_back(other);
                            continue;
                        }

                        reporter.setTestResult(TestType::REDUNDANT, false);
                        std::ostringstream ss;
                        ss << e1.getPath() << " and " << e2.getPath();
                        reporter.addReportMsg(TestType::REDUNDANT, ss.str());
                    }
                    l.swap(articlesDifferentFromE1);
                }
            }
        }
    }
}

void test_redirect_loop(const zim::Archive& archive, ErrorLogger& reporter) {
    reporter.infoMsg("[INFO] Checking for redirect loops...");

    int chained_redirection_limit = 50;

    for(auto& entry: archive.iterEfficient())
    {
        auto current_entry = entry;
        int redirections_done = 0;
        while(current_entry.isRedirect() && redirections_done < chained_redirection_limit)
        {
            current_entry = current_entry.getRedirectEntry();
            redirections_done++;
        }

        if(current_entry.isRedirect()){
            reporter.setTestResult(TestType::REDIRECT, false);
            std::ostringstream ss;
            ss << "Redirect loop exists from entry " << entry.getPath() << std::endl;
            reporter.addReportMsg(TestType::REDIRECT, ss.str());
        }
    }
}
