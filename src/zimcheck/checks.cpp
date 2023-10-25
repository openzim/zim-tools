#define ZIM_PRIVATE
#include "checks.h"
#include "../tools.h"
#include "../concurrent_cache.h"
#include "../metadata.h"

#include <cassert>
#include <map>
#include <unordered_map>
#include <list>
#include <sstream>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <queue>
#include <optional>
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
    { TestType::METADATA,      {LogTag::ERROR, "Metadata errors"}},
    { TestType::FAVICON,       {LogTag::ERROR, "Favicon"}},
    { TestType::MAIN_PAGE,     {LogTag::ERROR, "Missing mainpage"}},
    { TestType::REDUNDANT,     {LogTag::WARNING, "Redundant data found"}},
    { TestType::URL_INTERNAL,  {LogTag::ERROR, "Invalid internal links found"}},
    { TestType::URL_EXTERNAL,  {LogTag::ERROR, "Invalid external links found"}},
    { TestType::URL_EMPTY,     {LogTag::WARNING, "Empty links found"}},
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
  { MsgId::DANGLING_LINKS,   { TestType::URL_INTERNAL, "The following links:\n{{#links}}- {{&value}}\n{{/links}}({{&normalized_link}}) were not found in article {{&path}}" } },
  { MsgId::EXTERNAL_LINK,    { TestType::URL_EXTERNAL, "{{&link}} is an external dependence in article {{&path}}" } },
  { MsgId::EMPTY_LINKS,      { TestType::URL_EMPTY, "Found {{&count}} empty links in article: {{&path}}" } },
  { MsgId::REDUNDANT_ITEMS,  { TestType::REDUNDANT, "{{&path1}} and {{&path2}}" } },
  { MsgId::METADATA,         { TestType::METADATA, "{{&error}}" } },
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
    case TestType::URL_EMPTY:    return "url_empty";
    case TestType::REDIRECT:     return "redirect";
    default:  throw std::logic_error("Invalid TestType");
  };
}

typedef std::map<MsgParams::key_type, MsgParams::mapped_type> SortedMsgParams;
SortedMsgParams sortedMsgParams(const MsgParams& msgParams)
{
  return SortedMsgParams(msgParams.begin(), msgParams.end());
}

bool areAliases(const zim::Item& i1, const zim::Item& i2)
{
    return i1.getClusterIndex() == i2.getClusterIndex() && i1.getBlobIndex() == i2.getBlobIndex();
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
    reporter.infoMsg("[INFO] Checking metadata...");
    zim::Metadata metadata;
    for ( const auto& key : archive.getMetadataKeys() ) {
        metadata.set(key, archive.getMetadata(key));
    }
    for (const auto &error : metadata.check()) {
        reporter.addMsg(MsgId::METADATA, {{"error", error}});
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
    ArticleChecker(const zim::Archive& _archive, ErrorLogger& _reporter, ProgressBar& _progress, EnabledTests _checks)
        : archive(_archive)
        , reporter(_reporter)
        , progress(_progress)
        , checks(_checks)
        , linkStatusCache(64*1024)
    {
        progress.reset(archive.getEntryCount());
    }


    void check(zim::Entry entry);
    void detect_redundant_articles();

private: // types
    typedef std::vector<std::string> StringCollection;

    // collection of links grouped into sets of equivalent normalized links
    typedef std::map<std::string, StringCollection> GroupedLinkCollection;

private: // functions
    void check_item(const zim::Item& item);
    void check_internal_links(zim::Item item, const LinkCollection& links);
    void check_internal_links(zim::Item item, const GroupedLinkCollection& groupedLinks);
    void check_external_links(zim::Item item, const LinkCollection& links);

    bool is_valid_internal_link(const std::string& link)
    {
      return linkStatusCache.getOrPut(link, [=](){
                return archive.hasEntryByPath(link);
      });
    }

private: // data
    const zim::Archive& archive;
    ErrorLogger& reporter;
    ProgressBar& progress;
    const EnabledTests checks;

    // All article with the same hash will be recorded in the same bucket of
    // this hash table.
    std::map<unsigned int, std::list<zim::entry_index_type>> hash_main;

    zim::ConcurrentCache<std::string, bool> linkStatusCache;
};

void ArticleChecker::check(zim::Entry entry)
{
    progress.report();

    const auto path = entry.getPath();
    const char ns = archive.hasNewNamespaceScheme() ? 'C' : path[0];

    if (entry.isRedirect() || ns == 'M') {
        return;
    }

    check_item(entry.getItem());
}

void ArticleChecker::check_item(const zim::Item& item)
{
    if (item.getSize() == 0) {
        if (checks.isEnabled(TestType::EMPTY)) {
            const auto path = item.getPath();
            const char ns = archive.hasNewNamespaceScheme() ? 'C' : path[0];
            if (ns == 'C' || ns=='A' || ns == 'I') {
                reporter.addMsg(MsgId::EMPTY_ENTRY, {{"path", path}});
            }
        }
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
        if (l.link.empty())
        {
            nremptylinks++;
            continue;
        }
        if (l.link.front() == '#' || l.link.front() == '?') continue;
        if (l.isInternalUrl() == false) continue;


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
        if (!is_valid_internal_link(link)) {
            kainjow::mustache::list links;
            for (const auto &olink : p.second)
                links.push_back({"value", olink});
            reporter.addMsg(MsgId::DANGLING_LINKS, {{"path", path}, {"normalized_link", link}, {"links", links}});
            break;
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

void ArticleChecker::detect_redundant_articles()
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
            // The way we have constructed `l`, e1 MUST BE an item
            const auto e1 = archive.getEntryByPath(l.front()).getItem();
            l.pop_front();
            if ( !l.empty() ) {
                std::optional<std::string> s1;
                decltype(l) articlesDifferentFromE1;
                for(auto other : l) {
                    // The way we have constructed `l`, e2 MUST BE an item
                    const auto e2 = archive.getEntryByPath(other).getItem();
                    if (areAliases(e1, e2)) {
                        continue;
                    }
                    if (!s1) {
                        s1 = e1.getData();
                    }
                    std::string s2 = e2.getData();
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

class TaskStream
{
public: // constants
    const static size_t MAX_SIZE = 1000;

public: // functions
    explicit TaskStream(ArticleChecker* ac)
        : articleChecker(*ac)
        , expectingMoreTasks(true)
    {
        thread = std::thread([this]() { this->processTasks(); });
    }

    ~TaskStream()
    {
        finish();
    }

    // Wait for all tasks to complete and terminate the worker thread.
    // The TaskStream object becomes unusable after call to finish().
    void finish()
    {
        noMoreTasks();
        if ( thread.joinable()) {
          thread.join();
        }
    }

    void addTask(zim::Entry entry)
    {
        assert(expectingMoreTasks);
        std::unique_lock<std::mutex> lock(mutex);

        while ( inIsBlocked() )
            waitUntilInIsUnblocked(lock);

        taskQueue.push(entry);
        unblockOut();
    }

    void noMoreTasks()
    {
        std::lock_guard<std::mutex> lock(mutex);
        expectingMoreTasks = false;
        unblockOut();
    }

private: // types
    typedef std::shared_ptr<zim::Entry> Task;

private: // functions
    void processTasks()
    {
        while ( true )
        {
            const auto t = getNextTask();
            if ( !t )
                break;
            articleChecker.check(*t);
        }
    }

    bool inIsBlocked() const
    {
        return taskQueue.size() >= MAX_SIZE;
    }

    bool outIsBlocked() const
    {
        return expectingMoreTasks && taskQueue.empty();
    }

    void waitUntilInIsUnblocked(std::unique_lock<std::mutex>& lock)
    {
        cv.wait(lock);
    }

    void waitUntilOutIsUnblocked(std::unique_lock<std::mutex>& lock)
    {
        cv.wait(lock);
    }

    void unblockIn()
    {
        cv.notify_one();
    }

    void unblockOut()
    {
        cv.notify_one();
    }

    Task getNextTask()
    {
        std::unique_lock<std::mutex> lock(mutex);
        if ( outIsBlocked() )
            waitUntilOutIsUnblocked(lock);

        Task t;
        if ( !taskQueue.empty() )
        {
            t = std::make_shared<zim::Entry>(taskQueue.front());
            taskQueue.pop();
            unblockIn();
        }
        return t;
    }

private: // data
    ArticleChecker& articleChecker;
    std::queue<zim::Entry> taskQueue;
    std::mutex mutex;
    std::thread thread;

    // cv is used for managing the blocking & unblocking of both ends of the
    // task queue. This is possible with a single std::condition_variable due
    // to the fact that both ends of the queue cannot be blocked at the same
    // time:
    // 1. when the input is blocked (because the queue is full), the consumer
    //    is free to proceed (thus unblocking the input).
    // 2. when the output is blocked (becuase the queue is empty while more data
    //    has been promised) the producer is welcome to use the input end.
    // Thus only the consumer or the producer waits on cv.
    std::condition_variable cv;

    std::atomic<bool> expectingMoreTasks;
};


zim::cluster_index_type getClusterIndexOfZimEntry(zim::Entry e)
{
    return e.isRedirect() ? 0 : e.getItem().getClusterIndex();
}

class TaskDispatcher
{
public: // functions
    explicit TaskDispatcher(ArticleChecker* ac, unsigned n)
        : currentCluster(-1)
    {
        while ( n-- )
            taskStreams.emplace_back(ac);
    }

    void addTask(zim::Entry entry)
    {
        // Assuming that the entries are passed in in cluster order
        // (which is currently the case for zim::Archive::iterEfficient())
        const auto entryCluster = getClusterIndexOfZimEntry(entry);
        if ( currentCluster != entryCluster )
        {
            taskStreams.splice(taskStreams.end(), taskStreams, taskStreams.begin());
            currentCluster = entryCluster;
        }
        taskStreams.begin()->addTask(entry);
    }

    // Wait for all tasks to complete and terminate the worker threads.
    // The TaskDispatcher object becomes unusable after call to finish().
    void finish()
    {
        taskStreams.clear();
    }

private: // data
    std::list<TaskStream> taskStreams;
    zim::cluster_index_type currentCluster;
};

} // unnamed namespace

void test_articles(const zim::Archive& archive, ErrorLogger& reporter, ProgressBar& progress,
                   const EnabledTests checks, int thread_count) {
    ArticleChecker articleChecker(archive, reporter, progress, checks);
    reporter.infoMsg("[INFO] Verifying Articles' content...");

    TaskDispatcher td(&articleChecker, thread_count);
    for (auto& entry:archive.iterEfficient()) {
        td.addTask(entry);
    }
    td.finish();

    if (checks.isEnabled(TestType::REDUNDANT))
    {
        articleChecker.detect_redundant_articles();
    }
}

namespace
{

class RedirectionTable
{
private: // types
    enum LoopStatus : uint8_t
    {
        UNKNOWN,
        LOOP,
        NONLOOP
    };

public: // functions
    explicit RedirectionTable(size_t entryCount)
    {
        loopStatus.reserve(entryCount);
        redirTable.reserve(entryCount);
    }

    void addRedirectionEntry(zim::entry_index_type targetEntryIndex)
    {
        redirTable.push_back(targetEntryIndex);
        loopStatus.push_back(LoopStatus::UNKNOWN);
    }

    void addItem()
    {
        redirTable.push_back(redirTable.size());
        loopStatus.push_back(LoopStatus::NONLOOP);
    }

    size_t size() const { return redirTable.size(); }

    bool isInRedirectionLoop(zim::entry_index_type i)
    {
        if ( loopStatus[i] == UNKNOWN )
        {
            resolveLoopStatus(i);
        }

        return loopStatus[i] == LOOP;
    }

private: // functions
    LoopStatus detectLoopStatus(zim::entry_index_type i) const
    {
        auto i1 = i;
        auto i2 = i;
        // Follow redirections until an entry with known loop status
        // is found.
        // i2 moves through redirections at twice the speed of i1
        // if i2 runs over i1 then they are both inside a redirection loop
        for (bool moveI1 = false ; ; moveI1 = !moveI1)
        {
            if ( loopStatus[i2] != LoopStatus::UNKNOWN )
                return loopStatus[i2];

            i2 = redirTable[i2];

            if ( i2 == i1 )
                return LoopStatus::LOOP;

            if ( moveI1 )
                i1 = redirTable[i1];
        }
    }

    void resolveLoopStatus(zim::entry_index_type i)
    {
        const LoopStatus s = detectLoopStatus(i);
        for ( ; loopStatus[i] == LoopStatus::UNKNOWN; i = redirTable[i] )
        {
            loopStatus[i] = s;
        }
    }

private: // data
    std::vector<zim::entry_index_type> redirTable;
    std::vector<LoopStatus> loopStatus;
};

} // unnamed namespace

void test_redirect_loop(const zim::Archive& archive, ErrorLogger& reporter) {
    reporter.infoMsg("[INFO] Checking for redirect loops...");

    RedirectionTable redirTable(archive.getAllEntryCount());
    for(const auto& entry: archive.iterByPath())
    {
        if ( entry.isRedirect() )
            redirTable.addRedirectionEntry(entry.getRedirectEntryIndex());
        else
            redirTable.addItem();
    }

    for(zim::entry_index_type i = 0; i < redirTable.size(); ++i )
    {
        if(redirTable.isInRedirectionLoop(i)){
            const auto entry = archive.getEntryByPath(i);
            reporter.addMsg(MsgId::REDIRECT_LOOP, {{"entry_path", entry.getPath()}});
        }
    }
}
