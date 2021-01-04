#include "gtest/gtest.h"

#include "zim/zim.h"
#include "zim/archive.h"
#include "../src/zimcheck/checks.h"


TEST(zimfilechecks, test_checksum)
{
    std::string fn = "data/zimfiles/wikibooks_be_all_nopic_2017-02.zim";

    zim::Archive archive(fn);
    ErrorLogger logger;

    test_checksum(archive, logger);

    ASSERT_TRUE(logger.overallStatus());
}

TEST(zimfilechecks, test_metadata)
{
    std::string fn = "data/zimfiles/wikibooks_be_all_nopic_2017-02.zim";

    zim::Archive archive(fn);
    ErrorLogger logger;

    test_metadata(archive, logger);

    ASSERT_TRUE(logger.overallStatus());
}

TEST(zimfilechecks, test_favicon)
{
    std::string fn = "data/zimfiles/wikibooks_be_all_nopic_2017-02.zim";

    zim::Archive archive(fn);
    ErrorLogger logger;

    test_favicon(archive, logger);

    ASSERT_TRUE(logger.overallStatus());
}

TEST(zimfilechecks, test_mainpage)
{
    std::string fn = "data/zimfiles/wikibooks_be_all_nopic_2017-02.zim";

    zim::Archive archive(fn);
    ErrorLogger logger;

    test_mainpage(archive, logger);

    ASSERT_TRUE(logger.overallStatus());
}

TEST(zimfilechecks, test_articles)
{
    std::string fn = "data/zimfiles/wikibooks_be_all_nopic_2017-02.zim";

    zim::Archive archive(fn);
    ErrorLogger logger;
    ProgressBar progress(1);
    EnabledTests all_checks; all_checks.enableAll();
    test_articles(archive, logger, progress, all_checks);

    ASSERT_TRUE(logger.overallStatus());
}

class CapturedStdout
{
  std::ostringstream buffer;
  std::streambuf* const sbuf;
public:
  CapturedStdout()
    : sbuf(std::cout.rdbuf())
  {
    std::cout.rdbuf(buffer.rdbuf());
  }

  CapturedStdout(const CapturedStdout&) = delete;

  ~CapturedStdout()
  {
    std::cout.rdbuf(sbuf);
  }

  operator std::string() const { return buffer.str(); }
};

int zimcheck (const std::vector<const char*>& args);

TEST(zimcheck, nooptions_goodzimfile)
{
    CapturedStdout zimcheck_output;
    ASSERT_EQ(0, zimcheck({
      "zimcheck",
      "data/zimfiles/good.zim"
    }));

    ASSERT_EQ(
      "[INFO] Checking zim file data/zimfiles/good.zim" "\n"
      "[INFO] Verifying ZIM-archive structure integrity..." "\n"
      "[INFO] Avoiding redundant checksum test (already performed by the integrity check)." "\n"
      "[INFO] Searching for metadata entries..." "\n"
      "[INFO] Searching for Favicon..." "\n"
      "[INFO] Searching for main page..." "\n"
      "[INFO] Verifying Articles' content..." "\n"
      "[INFO] Searching for redundant articles..." "\n"
      "  Verifying Similar Articles for redundancies..." "\n"
      "[INFO] Overall Test Status: Pass" "\n"
      "[INFO] Total time taken by zimcheck: 0 seconds." "\n"
      , std::string(zimcheck_output)
    );
}
