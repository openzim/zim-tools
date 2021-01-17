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

const std::string zimcheck_help_message(
  "\n"
  "zimcheck checks the quality of a ZIM file.\n\n"
  "Usage: zimcheck [options] zimfile\n"
  "options:\n"
  "-A , --all             run all tests. Default if no flags are given.\n"
  "-0 , --empty           Empty content\n"
  "-C , --checksum        Internal CheckSum Test\n"
  "-I , --integrity       Low-level correctness/integrity checks\n"
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
  "zimcheck -M --favicon wikipedia.zim\n"
);

TEST(zimcheck, help)
{
    {
      CapturedStdout zimcheck_output;
      ASSERT_EQ(-1, zimcheck({"zimcheck", "-h"}));
      ASSERT_EQ(zimcheck_help_message, std::string(zimcheck_output));
    }
    {
      CapturedStdout zimcheck_output;
      ASSERT_EQ(-1, zimcheck({"zimcheck", "-H"}));
      ASSERT_EQ(zimcheck_help_message, std::string(zimcheck_output));
    }
    {
      CapturedStdout zimcheck_output;
      ASSERT_EQ(-1, zimcheck({"zimcheck", "--help"}));
      ASSERT_EQ(zimcheck_help_message, std::string(zimcheck_output));
    }
}


TEST(zimcheck, checksum_goodzimfile)
{
    CapturedStdout zimcheck_output;
    ASSERT_EQ(0, zimcheck({
      "zimcheck",
      "-C",
      "data/zimfiles/good.zim"
    }));

    ASSERT_EQ(
      "[INFO] Checking zim file data/zimfiles/good.zim" "\n"
      "[INFO] Verifying Internal Checksum..." "\n"
      "[INFO] Overall Test Status: Pass" "\n"
      "[INFO] Total time taken by zimcheck: 0 seconds." "\n"
      , std::string(zimcheck_output)
    );
}

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

TEST(zimcheck, bad_checksum)
{
    CapturedStdout zimcheck_output;
    ASSERT_EQ(1, zimcheck({
      "zimcheck",
      "-C",
      "data/zimfiles/bad_checksum.zim"
    }));

    ASSERT_EQ(
      "[INFO] Checking zim file data/zimfiles/bad_checksum.zim" "\n"
      "[INFO] Verifying Internal Checksum..." "\n"
      "  [ERROR] Wrong Checksum in ZIM archive" "\n"
      "[ERROR] Invalid checksum:" "\n"
      "  ZIM Archive Checksum in archive: 00000000000000000000000000000000" "\n"
      "" "\n"
      "[INFO] Overall Test Status: Fail" "\n"
      "[INFO] Total time taken by zimcheck: 0 seconds." "\n"
      , std::string(zimcheck_output)
    );
}

TEST(zimcheck, nooptions_poorzimfile)
{
    CapturedStdout zimcheck_output;
    ASSERT_EQ(1, zimcheck({
      "zimcheck",
      "data/zimfiles/poor.zim"
    }));

    ASSERT_EQ(
      "[INFO] Checking zim file data/zimfiles/poor.zim" "\n"
      "[INFO] Verifying ZIM-archive structure integrity..." "\n"
      "[INFO] Avoiding redundant checksum test (already performed by the integrity check)." "\n"
      "[INFO] Searching for metadata entries..." "\n"
      "[INFO] Searching for Favicon..." "\n"
      "[INFO] Searching for main page..." "\n"
      "[INFO] Verifying Articles' content..." "\n"
      "[INFO] Searching for redundant articles..." "\n"
      "  Verifying Similar Articles for redundancies..." "\n"
      "[ERROR] Empty articles:" "\n"
      "  Entry empty.html is empty" "\n"
      "[ERROR] Missing metadata entries:" "\n"
      "  Title" "\n"
      "  Description" "\n"
      "[ERROR] Missing favicon:" "\n"
      "[ERROR] Missing mainpage:" "\n"
      "  Main Page Index stored in Archive Header: 4294967295" "\n"
      "[WARNING] Redundant data found:" "\n"
      "  article1.html and redundant_article.html" "\n"
      "[ERROR] Invalid internal links found:" "\n"
      "  The following links:" "\n"
      "- A/non_existent.html" "\n"
      "(/A/non_existent.html) were not found in article dangling_link.html" "\n"
      "  Found 1 empty links in article: empty_link.html" "\n"
      "  ../../oops.html is out of bounds. Article: outofbounds_link.html" "\n"
      "[ERROR] Invalid external links found:" "\n"
      "  http://a.io/pic.png is an external dependence in article external_link.html" "\n"
      "[INFO] Overall Test Status: Fail" "\n"
      "[INFO] Total time taken by zimcheck: 0 seconds." "\n"
      , std::string(zimcheck_output)
    );
}
