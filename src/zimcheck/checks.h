#ifndef _ZIM_TOOL_ZIMFILECHECKS_H_
#define _ZIM_TOOL_ZIMFILECHECKS_H_

#include <unordered_map>
#include <vector>
#include <iostream>
#include <algorithm>
#include <bitset>

#include "../progress.h"

namespace zim {
  class Archive;
}

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
    INTEGRITY,
    EMPTY,
    METADATA,
    FAVICON,
    MAIN_PAGE,
    REDUNDANT,
    URL_INTERNAL,
    URL_EXTERNAL,
    MIME,
    OTHER,

    COUNT
};

// Specialization of std::hash needed for our unordered_map. Can be removed in c++14
namespace std {
  template <> struct hash<TestType> {
    size_t operator() (const TestType &t) const { return size_t(t); }
  };
}

static std::unordered_map<TestType, std::pair<LogTag, std::string>> errormapping = {
    { TestType::CHECKSUM,      {LogTag::ERROR, "Invalid checksum"}},
    { TestType::INTEGRITY,     {LogTag::ERROR, "Invalid low-level structure"}},
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

class EnabledTests {
    std::bitset<size_t(TestType::COUNT)> tests;

  public:
    EnabledTests() {}

    void enableAll() { tests.set(); }
    void enable(TestType tt) { tests.set(size_t(tt)); }
    bool isEnabled(TestType tt) const { return tests[size_t(tt)]; }
};

class ErrorLogger {
  private:
    // reportMsgs[i] holds messages for the i'th test/check
    std::vector<std::vector<std::string>> reportMsgs;

    // testStatus[i] corresponds to the status of i'th test
    std::bitset<size_t(TestType::COUNT)> testStatus;

  public:
    ErrorLogger()
      : reportMsgs(size_t(TestType::COUNT))
    {
        testStatus.set();
    }

    void setTestResult(TestType type, bool status) {
        testStatus[size_t(type)] = status;
    }

    void addReportMsg(TestType type, const std::string& message) {
        reportMsgs[size_t(type)].push_back(message);
    }

    void report(bool error_details) const {
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

    inline bool overallStatus() const {
        for ( size_t i = 0; i < size_t(TestType::COUNT); ++i ) {
            if (errormapping[TestType(i)].first == LogTag::ERROR) {
                if ( testStatus[i] == false ) {
                    return false;
                }
            }
        }
        return true;
    }
};


void test_checksum(zim::Archive& archive, ErrorLogger& reporter);
void test_integrity(const std::string& filename, ErrorLogger& reporter);
void test_metadata(const zim::Archive& archive, ErrorLogger& reporter);
void test_favicon(const zim::Archive& archive, ErrorLogger& reporter);
void test_mainpage(const zim::Archive& archive, ErrorLogger& reporter);
void test_articles(const zim::Archive& archive, ErrorLogger& reporter, ProgressBar progress,
                   const EnabledTests enabled_tests);

#endif
