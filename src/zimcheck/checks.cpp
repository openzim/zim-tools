#include "checks.h"
#include "../tools.h"

#include <map>
#include <unordered_map>
#include <list>
#include <sstream>
#define ZIM_PRIVATE
#include <zim/archive.h>
#include <zim/item.h>

// Specialization of std::hash needed for our unordered_map. Can be removed in c++14
namespace std {
  template <> struct hash<LogTag> {
    size_t operator() (const LogTag &t) const { return size_t(t); }
  };
}

// Specialization of std::hash needed for our unordered_map. Can be removed in c++14
namespace std {
  template <> struct hash<TestType> {
    size_t operator() (const TestType &t) const { return size_t(t); }
  };
}

// Specialization of std::hash needed for our unordered_map. Can be removed in c++14
namespace std {
  template <> struct hash<MsgId> {
    size_t operator() (const MsgId &msgid) const { return size_t(msgid); }
  };
}

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
    { TestType::FAVICON,       {LogTag::ERROR, "Favicon"}},
    { TestType::MAIN_PAGE,     {LogTag::ERROR, "Missing mainpage"}},
    { TestType::REDUNDANT,     {LogTag::WARNING, "Redundant data found"}},
    { TestType::URL_INTERNAL,  {LogTag::ERROR, "Invalid internal links found"}},
    { TestType::URL_EXTERNAL,  {LogTag::ERROR, "Invalid external links found"}},
    { TestType::REDIRECT,      {LogTag::ERROR, "Redirect loop(s) exist"}},
};

struct MsgInfo
{
  TestType check;
  std::string msgTemplate;
};

std::unordered_map<MsgId, MsgInfo> msgTable = {
  { MsgId::CHECKSUM,         { TestType::CHECKSUM, "ZIM Archive Checksum in archive: {{&archive_checksum}}\n" } },
  { MsgId::MAIN_PAGE,        { TestType::MAIN_PAGE, "Main Page Index stored in Archive Header: {{&main_page_index}}" } },
  { MsgId::EMPTY_ENTRY,      { TestType::EMPTY, "Entry {{&path}} is empty" } },
  { MsgId::OUTOFBOUNDS_LINK, { TestType::URL_INTERNAL, "{{&link}} is out of bounds. Article: {{&path}}" } },
  { MsgId::EMPTY_LINKS,      { TestType::URL_INTERNAL, "Found {{&count}} empty links in article: {{&path}}" } },
  { MsgId::DANGLING_LINKS,   { TestType::URL_INTERNAL, "The following links:\n{{#links}}- {{&value}}\n{{/links}}({{&normalized_link}}) were not found in article {{&path}}" } },
  { MsgId::EXTERNAL_LINK,    { TestType::URL_EXTERNAL, "{{&link}} is an external dependence in article {{&path}}" } },
  { MsgId::REDUNDANT_ITEMS,  { TestType::REDUNDANT, "{{&path1}} and {{&path2}}" } },
  { MsgId::MISSING_METADATA, { TestType::METADATA, "{{&metadata_type}}" } },
  { MsgId::REDIRECT_LOOP,    { TestType::REDIRECT, "Redirect loop exists from entry {{&entry_path}}\n"  } },
  { MsgId::MISSING_FAVICON,  { TestType::FAVICON, "Favicon is missing" } }
};

using kainjow::mustache::mustache;

template<typename T>
std::string toStr(T t)
{
  std::ostringstream ss;
  ss << t;
  return ss.str();
}

const char* toStr(TestType tt) {
  switch(tt) {
    case TestType::CHECKSUM:     return "checksum";
    case TestType::INTEGRITY:    return "integrity";
    case TestType::EMPTY:        return "empty";
    case TestType::METADATA:     return "metadata";
    case TestType::FAVICON:      return "favicon";
    case TestType::MAIN_PAGE:    return "main_page";
    case TestType::REDUNDANT:    return "redundant";
    case TestType::URL_INTERNAL: return "url_internal";
    case TestType::URL_EXTERNAL: return "url_external";
    case TestType::REDIRECT:     return "redirect";
    default:  throw std::logic_error("Invalid TestType");
  };
}

typedef std::map<MsgParams::key_type, MsgParams::mapped_type> SortedMsgParams;
SortedMsgParams sortedMsgParams(const MsgParams& msgParams)
{
  return SortedMsgParams(msgParams.begin(), msgParams.end());
}

} // unnamed namespace

namespace JSON
{

OutputStream& operator<<(OutputStream& out, const kainjow::mustache::data& d)
{
  if (d.is_string()) {
    out << d.string_value();
  } else if (d.is_list()) {
    out << startArray;
    for ( const auto& el : d.list_value() ) {
      out << el;
    }
    out << endArray;
  } else if (d.is_object()) {
    // HACK: kainjow::mustache::data wrapping a kainjow::mustache::object
    // HACK: doesn't provide direct access to the object or its keys.
    // HACK: So assuming that an object is only used to wrap a string or a list
    // HACK: under a hardcoded key 'value'
    const auto* v = d.get("value");
    if ( v )
      out << *v;
  } else {
    out << "!!! UNSUPPORTED DATA TYPE !!!";
  }
  return out;
}

} // namespace JSON

JSON::OutputStream& operator<<(JSON::OutputStream& out, TestType check)
{
  return out << toStr(check);
}

JSON::OutputStream& operator<<(JSON::OutputStream& out, EnabledTests checks)
{
  out << JSON::startArray;
  for ( size_t i = 0; i < size_t(TestType::COUNT); ++i ) {
      if ( checks.isEnabled(TestType(i)) ) {
          out << TestType(i);
      }
  }
  out << JSON::endArray;
  return out;
}

ErrorLogger::ErrorLogger(bool _jsonOutputMode)
  : reportMsgs(size_t(TestType::COUNT))
  , jsonOutputStream(_jsonOutputMode ? &std::cout : nullptr)
{
    testStatus.set();
    jsonOutputStream << JSON::startObject;
}

ErrorLogger::~ErrorLogger()
{
    jsonOutputStream << JSON::endObject;
}

void ErrorLogger::infoMsg(const std::string& msg) const {
  if ( !jsonOutputStream.enabled() ) {
    std::cout << msg << std::endl;
  }
}

void ErrorLogger::setTestResult(TestType type, bool status) {
    testStatus[size_t(type)] = status;
}

void ErrorLogger::addMsg(MsgId msgid, const MsgParams& msgParams)
{
  const MsgInfo& m = msgTable.at(msgid);
  setTestResult(m.check, false);
  reportMsgs[size_t(m.check)].push_back({msgid, msgParams});

}

std::string ErrorLogger::expand(const MsgIdWithParams& msg)
{
  const MsgInfo& m = msgTable.at(msg.msgId);
  mustache tmpl{m.msgTemplate};
  return tmpl.render(msg.msgParams);
}

void ErrorLogger::jsonOutput(const MsgIdWithParams& msg) const {
  const MsgInfo& m = msgTable.at(msg.msgId);
  jsonOutputStream << JSON::startObject;
  jsonOutputStream << JSON::property("check", m.check);
  jsonOutputStream << JSON::property("level", tagToStr[errormapping[m.check].first]);
  jsonOutputStream << JSON::property("message", expand(msg));

  for ( const auto& kv : sortedMsgParams(msg.msgParams) ) {
    jsonOutputStream << JSON::property(kv.first, kv.second);
  }
  jsonOutputStream << JSON::endObject;
}

void ErrorLogger::report(bool error_details) const {
    if ( jsonOutputStream.enabled() ) {
        jsonOutputStream << JSON::property("logs", JSON::startArray);
        for ( size_t i = 0; i < size_t(TestType::COUNT); ++i ) {
            for (const auto& msg: reportMsgs[i]) {
                jsonOutput(msg);
            }
        }
        jsonOutputStream << JSON::endArray;
    } else {
        for ( size_t i = 0; i < size_t(TestType::COUNT); ++i ) {
            const auto& testmsg = reportMsgs[i];
            if ( !testStatus[i] ) {
                auto &p = errormapping[TestType(i)];
                std::cout << "[" + tagToStr[p.first] + "] " << p.second << ":" << std::endl;
                for (auto& msg: testmsg) {
                    std::cout << "  " << expand(msg) << std::endl;
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
    if (!result) {
        reporter.infoMsg("  [ERROR] Wrong Checksum in ZIM archive");
        reporter.addMsg(MsgId::CHECKSUM, {{"archive_checksum", archive.getChecksum()}});
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
            reporter.addMsg(MsgId::MISSING_METADATA, {{"metadata_type", meta}});
        }
    }
}

void test_favicon(const zim::Archive& archive, ErrorLogger& reporter) {
    reporter.infoMsg("[INFO] Searching for Favicon...");

    const auto illustrationSizes = archive.getIllustrationSizes();
    if ( illustrationSizes.find(48) == illustrationSizes.end() )
      reporter.addMsg(MsgId::MISSING_FAVICON, {});
}

void test_mainpage(const zim::Archive& archive, ErrorLogger& reporter) {
    reporter.infoMsg("[INFO] Searching for main page...");
    bool testok = true;
    try {
      archive.getMainEntry();
    } catch(...) {
        testok = false;
    }
    if (!testok) {
        reporter.addMsg(MsgId::MAIN_PAGE, {{"main_page_index", toStr(archive.getMainEntryIndex())}});
    }
}

namespace
{

class ArticleChecker
{
public: // types
    typedef std::vector<html_link> LinkCollection;

public: // functions
    ArticleChecker(const zim::Archive& _archive, ErrorLogger& _reporter, EnabledTests _checks)
        : archive(_archive)
        , reporter(_reporter)
        , checks(_checks)
        , previousIndex(-1)
    {}


    void check(zim::Entry entry);
    void detect_redundant_articles(ProgressBar& progress);
    void check_internal_links(zim::Item item, const LinkCollection& links);
    void check_external_links(zim::Item item, const LinkCollection& links);

private: // types
    typedef std::vector<std::string> StringCollection;

    // collection of links grouped into sets of equivalent normalized links
    typedef std::unordered_map<std::string, StringCollection> GroupedLinkCollection;

private: // functions
    void check_internal_links(zim::Item item, const GroupedLinkCollection& groupedLinks);

private: // data
    const zim::Archive& archive;
    ErrorLogger& reporter;
    const EnabledTests checks;
    int previousIndex;

    // All article with the same hash will be recorded in the same bucket of
    // this hash table.
    std::map<unsigned int, std::list<zim::entry_index_type>> hash_main;
};

void ArticleChecker::check(zim::Entry entry)
{
    const auto path = entry.getPath();
    const char ns = archive.hasNewNamespaceScheme() ? 'C' : path[0];

    if (entry.isRedirect() || ns == 'M') {
        return;
    }

    if (checks.isEnabled(TestType::EMPTY) && (ns == 'C' || ns=='A' || ns == 'I')) {
        auto item = entry.getItem();
        if (item.getSize() == 0) {
            reporter.addMsg(MsgId::EMPTY_ENTRY, {{"path", path}});
        }
    }

    auto item = entry.getItem();

    if (item.getSize() == 0) {
        return;
    }

    std::string data;
    if (checks.isEnabled(TestType::REDUNDANT) || item.getMimetype() == "text/html")
        data = item.getData();

    if(checks.isEnabled(TestType::REDUNDANT))
        hash_main[adler32(data)].push_back( item.getIndex() );

    if (item.getMimetype() != "text/html")
        return;

    ArticleChecker::LinkCollection links;
    if (checks.isEnabled(TestType::URL_INTERNAL) ||
        checks.isEnabled(TestType::URL_EXTERNAL)) {
        links = generic_getLinks(data);
    }

    if(checks.isEnabled(TestType::URL_INTERNAL))
    {
        check_internal_links(item, links);
    }

    if (checks.isEnabled(TestType::URL_EXTERNAL))
    {
        check_external_links(item, links);
    }
}

void ArticleChecker::check_internal_links(zim::Item item, const LinkCollection& links)
{
    const auto path = item.getPath();
    auto baseUrl = path;
    auto pos = baseUrl.find_last_of('/');
    baseUrl.resize( pos==baseUrl.npos ? 0 : pos );

    ArticleChecker::GroupedLinkCollection groupedLinks;
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
            reporter.addMsg(MsgId::OUTOFBOUNDS_LINK, {{"link", l.link}, {"path", path}});
            continue;
        }

        auto normalized = normalize_link(l.link, baseUrl);
        groupedLinks[normalized].push_back(l.link);
    }

    if (nremptylinks)
    {
        reporter.addMsg(MsgId::EMPTY_LINKS, {{"count", toStr(nremptylinks)}, {"path", path}});
    }

    check_internal_links(item, groupedLinks);
}

void ArticleChecker::check_internal_links(zim::Item item, const GroupedLinkCollection& groupedLinks)
{
    const auto path = item.getPath();
    for(const auto &p: groupedLinks)
    {
        const std::string link = p.first;
        if (!archive.hasEntryByPath(link)) {
            int index = item.getIndex();
            if (previousIndex != index)
            {
                kainjow::mustache::list links;
                for (const auto &olink : p.second)
                    links.push_back({"value", olink});
                reporter.addMsg(MsgId::DANGLING_LINKS, {{"path", path}, {"normalized_link", link}, {"links", links}});
                previousIndex = index;
            }
            reporter.setTestResult(TestType::URL_INTERNAL, false);
        }
    }
}

void ArticleChecker::check_external_links(zim::Item item, const LinkCollection& links)
{
    const auto path = item.getPath();
    for (const auto &l: links)
    {
        if (l.attribute == "src" && l.isExternalUrl())
        {
            reporter.addMsg(MsgId::EXTERNAL_LINK, {{"link", l.link}, {"path", path}});
            break;
        }
    }
}

void ArticleChecker::detect_redundant_articles(ProgressBar& progress)
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

                    reporter.addMsg(MsgId::REDUNDANT_ITEMS, {{"path1", e1.getPath()}, {"path2", e2.getPath()}});
                }
                l.swap(articlesDifferentFromE1);
            }
        }
    }
}

} // unnamed namespace

void test_articles(const zim::Archive& archive, ErrorLogger& reporter, ProgressBar progress,
                   const EnabledTests checks) {
    ArticleChecker articleChecker(archive, reporter, checks);
    reporter.infoMsg("[INFO] Verifying Articles' content...");

    progress.reset(archive.getEntryCount());
    for (auto& entry:archive.iterEfficient()) {
        progress.report();
        articleChecker.check(entry);
    }

    if (checks.isEnabled(TestType::REDUNDANT))
    {
        articleChecker.detect_redundant_articles(progress);
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
            reporter.addMsg(MsgId::REDIRECT_LOOP, {{"entry_path", entry.getPath()}});
        }
    }
}
