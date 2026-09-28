#ifndef ZIM_TOOLS_TESTING_HELPERS_H
#define ZIM_TOOLS_TESTING_HELPERS_H

#include <iostream>
#include <sstream>
#include <streambuf>
#include <string>

class CapturedStdStream
{
  std::ostream& stream;
  std::ostringstream buffer;
  std::streambuf* const sbuf;
public:
  explicit CapturedStdStream(std::ostream& os)
    : stream(os)
    , sbuf(os.rdbuf())
  {
    stream.rdbuf(buffer.rdbuf());
  }

  CapturedStdStream(const CapturedStdStream&) = delete;

  ~CapturedStdStream()
  {
    stream.rdbuf(sbuf);
  }

  operator std::string() const { return buffer.str(); }
};

struct CapturedStdout : CapturedStdStream
{
  CapturedStdout() : CapturedStdStream(std::cout) {}
};

struct CapturedStderr : CapturedStdStream
{
  CapturedStderr() : CapturedStdStream(std::cerr) {}
};

#endif
