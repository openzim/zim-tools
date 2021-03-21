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
    OTHER,
    REDIRECT,
    COUNT
};

// Specialization of std::hash needed for our unordered_map. Can be removed in c++14
namespace std {
  template <> struct hash<TestType> {
    size_t operator() (const TestType &t) const { return size_t(t); }
  };
}

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

    const bool jsonOutputMode;
    const char* sep = "\n";
    std::string m_indentation = "  ";

    const std::string& indentation() const { return m_indentation; }

    static std::string formatForJSON(const std::string& s) {
      return "'" + s + "'";
    }

    static std::string formatForJSON(const char* s) {
      return formatForJSON(std::string(s));
    }

    static const char* formatForJSON(bool b) {
      return b ? "true" : "false";
    }

  public:
    explicit ErrorLogger(bool _jsonOutputMode = false);
    ~ErrorLogger();

    void infoMsg(const std::string& msg) const;

    template<class T>
    void addInfo(const std::string& key, const T& value) {
      if ( jsonOutputMode ) {
        std::cout << sep << indentation()
                  << "'" << key << "' : " << formatForJSON(value)
                  << std::flush;
        sep = ",\n";
      }
    }

    void setTestResult(TestType type, bool status);
    void addReportMsg(TestType type, const std::string& message);
    void report(bool error_details) const;
    bool overallStatus() const;
};


void test_checksum(zim::Archive& archive, ErrorLogger& reporter);
void test_integrity(const std::string& filename, ErrorLogger& reporter);
void test_metadata(const zim::Archive& archive, ErrorLogger& reporter);
void test_favicon(const zim::Archive& archive, ErrorLogger& reporter);
void test_mainpage(const zim::Archive& archive, ErrorLogger& reporter);
void test_articles(const zim::Archive& archive, ErrorLogger& reporter, ProgressBar progress,
                   const EnabledTests enabled_tests);
void test_redirect_loop(const zim::Archive& archive, ErrorLogger& reporter);

#endif
