#ifndef _ZIM_TOOL_ZIMFILECHECKS_H_
#define _ZIM_TOOL_ZIMFILECHECKS_H_

#include <vector>
#include <iostream>
#include <bitset>

#include <mustache.hpp>

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
    REDIRECT,

    COUNT
};

class EnabledTests {
    std::bitset<size_t(TestType::COUNT)> tests;

  public:
    EnabledTests() {}

    void enableAll() { tests.set(); }
    void enable(TestType tt) { tests.set(size_t(tt)); }
    bool isEnabled(TestType tt) const { return tests[size_t(tt)]; }
};

enum class MsgId
{
  CHECKSUM,
  MAIN_PAGE,
  MISSING_METADATA,
  EMPTY_ENTRY,
  OUTOFBOUNDS_LINK,
  EMPTY_LINKS,
  DANGLING_LINKS,
  EXTERNAL_LINK,
  REDUNDANT_ITEMS,
  REDIRECT_LOOP,
  MISSING_FAVICON
};

using MsgParams = kainjow::mustache::object;

class ErrorLogger {
  private:
    struct MsgIdWithParams
    {
      MsgId msgId;
      MsgParams msgParams;
    };

    // reportMsgs[i] holds messages for the i'th test/check
    std::vector<std::vector<MsgIdWithParams>> reportMsgs;

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

    static const char* formatForJSON(TestType tt) {
        switch(tt) {
          case TestType::CHECKSUM:     return "'checksum'";
          case TestType::INTEGRITY:    return "'integrity'";
          case TestType::EMPTY:        return "'empty'";
          case TestType::METADATA:     return "'metadata'";
          case TestType::FAVICON:      return "'favicon'";
          case TestType::MAIN_PAGE:    return "'main_page'";
          case TestType::REDUNDANT:    return "'redundant'";
          case TestType::URL_INTERNAL: return "'url_internal'";
          case TestType::URL_EXTERNAL: return "'url_external'";
          case TestType::REDIRECT:     return "'redirect'";
          default:  throw std::logic_error("Invalid TestType");
        };
    }

    static std::string formatForJSON(EnabledTests checks) {
        std::string result;
        for ( size_t i = 0; i < size_t(TestType::COUNT); ++i ) {
            if ( checks.isEnabled(TestType(i)) ) {
                if ( !result.empty() )
                  result += ", ";
                result += formatForJSON(TestType(i));
            }
        }
        return "[" + result + "]";
    }

    static std::string expand(const MsgIdWithParams& msg);
    void jsonOutput(const MsgIdWithParams& msg) const;

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
    void addMsg(MsgId msgid, const MsgParams& msgParams);
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
