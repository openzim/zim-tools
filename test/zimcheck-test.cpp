#include <sstream>

#include "gtest/gtest.h"

#include "zim/zim.h"
#include "zim/archive.h"
#include "../src/zimcheck/checks.h"

std::string getLine(std::string str) {
  std::istringstream f(str);
  std::string line;
  std::getline(f, line);
  return line;
}

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
    std::string fn = "data/zimfiles/good.zim";

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

TEST(zimfilechecks, test_redirect_loop_pass)
{
  std::string fn = "data/zimfiles/wikibooks_be_all_nopic_2017-02.zim";

  zim::Archive archive(fn);
  ErrorLogger logger;

  test_redirect_loop(archive, logger);

  ASSERT_TRUE(logger.overallStatus());
}

TEST(zimfilechecks, test_redirect_loop_fail)
{
  ErrorLogger logger;
  std::string poor_zim = "data/zimfiles/poor.zim";
  zim::Archive archive_poor(poor_zim);
  test_redirect_loop(archive_poor, logger);
  ASSERT_FALSE(logger.overallStatus());
}

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
  "-D , --details         Details of error\n"
  "-B , --progress        Print progress report\n"
  "-J , --json            Output in JSON format\n"
  "-H , --help            Displays Help\n"
  "-V , --version         Displays software version\n"
  "-L , --redirect_loop   Checks for the existence of redirect loops\n"
  "-W , --threads         count of threads to utilize (default: 1)\n"
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

TEST(zimcheck, version)
{
    std::string version = "zim-tools " + std::string(VERSION);

    {
      CapturedStdout zimcheck_output;
      ASSERT_EQ(0, zimcheck({"zimcheck", "-v"}));
      ASSERT_EQ(version, getLine(std::string(zimcheck_output)));
    }
    {
      CapturedStdout zimcheck_output;
      ASSERT_EQ(0, zimcheck({"zimcheck", "-V"}));
      ASSERT_EQ(version, getLine(std::string(zimcheck_output)));
    }
    {
      CapturedStdout zimcheck_output;
      ASSERT_EQ(0, zimcheck({"zimcheck", "--version"}));
      ASSERT_EQ(version, getLine(std::string(zimcheck_output)));
    }
}

TEST(zimcheck, nozimfile)
{
    const std::string expected_stderr = "No file provided as argument\n";
    {
      CapturedStdout zimcheck_output;
      CapturedStderr zimcheck_stderr;
      ASSERT_EQ(-1, zimcheck({"zimcheck"}));
      ASSERT_EQ(expected_stderr, std::string(zimcheck_stderr));
      ASSERT_EQ(zimcheck_help_message, std::string(zimcheck_output));
    }
}

const char GOOD_ZIMFILE[] = "data/zimfiles/good.zim";
const char POOR_ZIMFILE[] = "data/zimfiles/poor.zim";
const char BAD_CHECKSUM_ZIMFILE[] = "data/zimfiles/bad_checksum.zim";

using CmdLineImpl = std::vector<const char*>;
struct CmdLine : CmdLineImpl {
  CmdLine(const std::initializer_list<value_type>& il)
    : CmdLineImpl(il)
  {}
};

std::ostream& operator<<(std::ostream& out, const CmdLine& c)
{
  out << "Test context:\n";
  for ( const auto& a : c )
    out << " " << a;
  out << std::endl;
  return out;
}

const char EMPTY_STDERR[] = "";

void test_zimcheck_single_option(std::vector<const char*> optionAliases,
                                 const char* zimfile,
                                 int expected_exit_code,
                                 const std::string& expected_stdout,
                                 const std::string& expected_stderr)
{
    for ( const char* opt : optionAliases )
    {
        CapturedStdout zimcheck_output;
        CapturedStderr zimcheck_stderr;
        const CmdLine cmdline{"zimcheck", opt, zimfile};
        EXPECT_EQ(expected_exit_code, zimcheck(cmdline)) << cmdline;
        EXPECT_EQ(expected_stderr, std::string(zimcheck_stderr)) << cmdline;
        EXPECT_EQ(expected_stdout, std::string(zimcheck_output)) << cmdline;
    }
}

TEST(zimcheck, integrity_goodzimfile)
{
    const std::string expected_output(
        "[INFO] Checking zim file data/zimfiles/good.zim" "\n"
        "[INFO] Zimcheck version is " VERSION "\n"
        "[INFO] Verifying ZIM-archive structure integrity..." "\n"
        "[INFO] Overall Test Status: Pass" "\n"
        "[INFO] Total time taken by zimcheck: <3 seconds." "\n"
    );

    test_zimcheck_single_option(
        {"-i", "-I", "--integrity"},
        GOOD_ZIMFILE,
        0,
        expected_output,
        EMPTY_STDERR
    );
}

TEST(zimcheck, checksum_goodzimfile)
{
    const std::string expected_output(
        "[INFO] Checking zim file data/zimfiles/good.zim" "\n"
        "[INFO] Zimcheck version is " VERSION "\n"
        "[INFO] Verifying Internal Checksum..." "\n"
        "[INFO] Overall Test Status: Pass" "\n"
        "[INFO] Total time taken by zimcheck: <3 seconds." "\n"
    );

    test_zimcheck_single_option(
        {"-c", "-C", "--checksum"},
        GOOD_ZIMFILE,
        0,
        expected_output,
        EMPTY_STDERR
    );
}

TEST(zimcheck, metadata_goodzimfile)
{
    const std::string expected_output(
        "[INFO] Checking zim file data/zimfiles/good.zim" "\n"
        "[INFO] Zimcheck version is " VERSION "\n"
        "[INFO] Checking metadata..." "\n"
        "[INFO] Overall Test Status: Pass" "\n"
        "[INFO] Total time taken by zimcheck: <3 seconds." "\n"
    );

    test_zimcheck_single_option(
        {"-m", "-M", "--metadata"},
        GOOD_ZIMFILE,
        0,
        expected_output,
        EMPTY_STDERR
    );
}

TEST(zimcheck, favicon_goodzimfile)
{
    const std::string expected_output(
        "[INFO] Checking zim file data/zimfiles/good.zim" "\n"
        "[INFO] Zimcheck version is " VERSION "\n"
        "[INFO] Searching for Favicon..." "\n"
        "[INFO] Overall Test Status: Pass" "\n"
        "[INFO] Total time taken by zimcheck: <3 seconds." "\n"
    );

    test_zimcheck_single_option(
        {"-f", "-F", "--favicon"},
        GOOD_ZIMFILE,
        0,
        expected_output,
        EMPTY_STDERR
    );
}

TEST(zimcheck, mainpage_goodzimfile)
{
    const std::string expected_output(
        "[INFO] Checking zim file data/zimfiles/good.zim" "\n"
        "[INFO] Zimcheck version is " VERSION "\n"
        "[INFO] Searching for main page..." "\n"
        "[INFO] Overall Test Status: Pass" "\n"
        "[INFO] Total time taken by zimcheck: <3 seconds." "\n"
    );

    test_zimcheck_single_option(
        {"-p", "-P", "--main"},
        GOOD_ZIMFILE,
        0,
        expected_output,
        EMPTY_STDERR
    );
}

TEST(zimcheck, article_content_goodzimfile)
{
    const std::string expected_output(
        "[INFO] Checking zim file data/zimfiles/good.zim" "\n"
        "[INFO] Zimcheck version is " VERSION "\n"
        "[INFO] Verifying Articles' content..." "\n"
        "[INFO] Overall Test Status: Pass" "\n"
        "[INFO] Total time taken by zimcheck: <3 seconds." "\n"
    );

    test_zimcheck_single_option(
        {
          "-0", "--empty",              // Any of these options triggers
          "-u", "-U", "--url_internal", // checking of the article contents.
          "-x", "-X", "--url_external"  // For a good ZIM file there is no
        },                              // difference in the output.
        GOOD_ZIMFILE,
        0,
        expected_output,
        EMPTY_STDERR
    );
}

TEST(zimcheck, redundant_articles_goodzimfile)
{
    const std::string expected_output(
        "[INFO] Checking zim file data/zimfiles/good.zim" "\n"
        "[INFO] Zimcheck version is " VERSION "\n"
        "[INFO] Verifying Articles' content..." "\n"
        "[INFO] Searching for redundant articles..." "\n"
        "  Verifying Similar Articles for redundancies..." "\n"
        "[INFO] Overall Test Status: Pass" "\n"
        "[INFO] Total time taken by zimcheck: <3 seconds." "\n"
    );

    test_zimcheck_single_option(
        {"-r", "-R", "--redundant"},
        GOOD_ZIMFILE,
        0,
        expected_output,
        EMPTY_STDERR
    );
}

TEST(zimcheck, redirect_loop_goodzimfile)
{
  const std::string expected_output(
    "[INFO] Checking zim file data/zimfiles/good.zim" "\n"
    "[INFO] Zimcheck version is " VERSION "\n"
    "[INFO] Checking for redirect loops..." "\n"
    "[INFO] Overall Test Status: Pass" "\n"
    "[INFO] Total time taken by zimcheck: <3 seconds." "\n"
  );

  test_zimcheck_single_option(
    {"-l", "-L", "--redirect_loop"},
    GOOD_ZIMFILE,
    0,
    expected_output,
    EMPTY_STDERR
  );
}

const std::string ALL_CHECKS_OUTPUT_ON_GOODZIMFILE(
      "[INFO] Checking zim file data/zimfiles/good.zim" "\n"
      "[INFO] Zimcheck version is " VERSION "\n"
      "[INFO] Verifying ZIM-archive structure integrity..." "\n"
      "[INFO] Avoiding redundant checksum test (already performed by the integrity check)." "\n"
      "[INFO] Checking metadata..." "\n"
      "[INFO] Searching for Favicon..." "\n"
      "[INFO] Searching for main page..." "\n"
      "[INFO] Verifying Articles' content..." "\n"
      "[INFO] Searching for redundant articles..." "\n"
      "  Verifying Similar Articles for redundancies..." "\n"
      "[INFO] Checking for redirect loops..." "\n"
      "[INFO] Overall Test Status: Pass" "\n"
      "[INFO] Total time taken by zimcheck: <3 seconds." "\n"
);

TEST(zimcheck, nooptions_goodzimfile)
{
    CapturedStdout zimcheck_output;
    ASSERT_EQ(0, zimcheck({"zimcheck", GOOD_ZIMFILE}));

    ASSERT_EQ(ALL_CHECKS_OUTPUT_ON_GOODZIMFILE, std::string(zimcheck_output));
}

TEST(zimcheck, all_checks_goodzimfile)
{
    test_zimcheck_single_option(
        {"-a", "-A", "--all"},
        GOOD_ZIMFILE,
        0,
        ALL_CHECKS_OUTPUT_ON_GOODZIMFILE,
        EMPTY_STDERR
    );
}

TEST(zimcheck, invalid_option)
{
    {
      CapturedStdout zimcheck_output;
      CapturedStderr zimcheck_stderr;
      ASSERT_EQ(1, zimcheck({"zimcheck", "-z", GOOD_ZIMFILE}));
      ASSERT_EQ("Unknown option `-z'\n", std::string(zimcheck_stderr));
      ASSERT_EQ(zimcheck_help_message, std::string(zimcheck_output));
    }
}

TEST(zimcheck, invalid_long_option)
{
    {
      CapturedStdout zimcheck_output;
      CapturedStderr zimcheck_stderr;
      ASSERT_EQ(1, zimcheck({"zimcheck", "--oops", GOOD_ZIMFILE}));
      ASSERT_EQ("Unknown option `--oops'\n", std::string(zimcheck_stderr));
      ASSERT_EQ(zimcheck_help_message, std::string(zimcheck_output));
    }
}

TEST(zimcheck, json_goodzimfile)
{
    CapturedStdout zimcheck_output;
    ASSERT_EQ(0, zimcheck({
      "zimcheck",
      "--json",
      "data/zimfiles/good.zim"
    }));

    ASSERT_EQ(
      "{"                                                           "\n"
      "  \"zimcheck_version\" : \"" VERSION "\","                   "\n"
      "  \"checks\" : ["                                            "\n"
      "    \"checksum\","                                           "\n"
      "    \"integrity\","                                          "\n"
      "    \"empty\","                                              "\n"
      "    \"metadata\","                                           "\n"
      "    \"favicon\","                                            "\n"
      "    \"main_page\","                                          "\n"
      "    \"redundant\","                                          "\n"
      "    \"url_internal\","                                       "\n"
      "    \"url_external\","                                       "\n"
      "    \"redirect\""                                            "\n"
      "  ],"                                                        "\n"
      "  \"file_name\" : \"data/zimfiles/good.zim\","               "\n"
      "  \"file_uuid\" : \"00000000-0000-0000-0000-000000000000\"," "\n"
      "  \"status\" : true,"                                        "\n"
      "  \"logs\" : ["                                              "\n"
      "  ]"                                                         "\n"
      "}" "\n"
      , std::string(zimcheck_output)
    );
}

TEST(zimcheck, bad_checksum)
{
    const std::string expected_output(
      "[INFO] Checking zim file data/zimfiles/bad_checksum.zim" "\n"
      "[INFO] Zimcheck version is " VERSION "\n"
      "[INFO] Verifying Internal Checksum..." "\n"
      "  [ERROR] Wrong Checksum in ZIM archive" "\n"
      "[ERROR] Invalid checksum:" "\n"
      "  ZIM Archive Checksum in archive: 00000000000000000000000000000000" "\n"
      "" "\n"
      "[INFO] Overall Test Status: Fail" "\n"
      "[INFO] Total time taken by zimcheck: <3 seconds." "\n"
    );

    test_zimcheck_single_option(
        {"-c", "-C", "--checksum"},
        BAD_CHECKSUM_ZIMFILE,
        1,
        expected_output,
        EMPTY_STDERR
    );
}

TEST(zimcheck, metadata_poorzimfile)
{
    const std::string expected_stdout(
      "[INFO] Checking zim file data/zimfiles/poor.zim" "\n"
      "[INFO] Zimcheck version is " VERSION "\n"
      "[INFO] Checking metadata..." "\n"
      "[ERROR] Metadata errors:" "\n"
      "  Missing mandatory metadata: Title" "\n"
      "  Missing mandatory metadata: Description" "\n"
      "  Missing mandatory metadata: Illustration_48x48@1" "\n"
      "  Language must contain at least 3 characters" "\n"
      "  Language doesn't match regex: \\w{3}(,\\w{3})*" "\n"
      "[INFO] Overall Test Status: Fail" "\n"
      "[INFO] Total time taken by zimcheck: <3 seconds." "\n"
    );

    test_zimcheck_single_option(
        {"-m", "-M", "--metadata"},
        POOR_ZIMFILE,
        1,
        expected_stdout,
        EMPTY_STDERR
    );
}

TEST(zimcheck, favicon_poorzimfile)
{
    const std::string expected_stdout(
      "[INFO] Checking zim file data/zimfiles/poor.zim" "\n"
      "[INFO] Zimcheck version is " VERSION "\n"
      "[INFO] Searching for Favicon..." "\n"
      "[ERROR] Favicon:" "\n"
      "  Favicon is missing" "\n"
      "[INFO] Overall Test Status: Fail" "\n"
      "[INFO] Total time taken by zimcheck: <3 seconds." "\n"
    );

    test_zimcheck_single_option(
        {"-f", "-F", "--favicon"},
        POOR_ZIMFILE,
        1,
        expected_stdout,
        EMPTY_STDERR
    );
}

TEST(zimcheck, mainpage_poorzimfile)
{
    const std::string expected_stdout(
      "[INFO] Checking zim file data/zimfiles/poor.zim" "\n"
      "[INFO] Zimcheck version is " VERSION "\n"
      "[INFO] Searching for main page..." "\n"
      "[ERROR] Missing mainpage:" "\n"
      "  Main Page Index stored in Archive Header: 4294967295" "\n"
      "[INFO] Overall Test Status: Fail" "\n"
      "[INFO] Total time taken by zimcheck: <3 seconds." "\n"
    );

    test_zimcheck_single_option(
        {"-p", "-P", "--main"},
        POOR_ZIMFILE,
        1,
        expected_stdout,
        EMPTY_STDERR
    );
}

TEST(zimcheck, empty_items_poorzimfile)
{
    const std::string expected_stdout(
      "[INFO] Checking zim file data/zimfiles/poor.zim" "\n"
      "[INFO] Zimcheck version is " VERSION "\n"
      "[INFO] Verifying Articles' content..." "\n"
      "[ERROR] Empty articles:" "\n"
      "  Entry empty.html is empty" "\n"
      "[INFO] Overall Test Status: Fail" "\n"
      "[INFO] Total time taken by zimcheck: <3 seconds." "\n"
    );

    test_zimcheck_single_option(
        {"-0", "--empty"},
        POOR_ZIMFILE,
        1,
        expected_stdout,
        EMPTY_STDERR
    );
}

TEST(zimcheck, internal_url_check_poorzimfile)
{
    const std::string expected_stdout(
      "[INFO] Checking zim file data/zimfiles/poor.zim" "\n"
      "[INFO] Zimcheck version is " VERSION "\n"
      "[INFO] Verifying Articles' content..." "\n"
      "[ERROR] Invalid internal links found:" "\n"
      "  The following links:" "\n"
      "- A/non_existent.html" "\n"
      "(A/non_existent.html) were not found in article dangling_link.html" "\n"
      "  Found 1 empty links in article: empty_link.html" "\n"
      "  ../../oops.html is out of bounds. Article: outofbounds_link.html" "\n"
      "[INFO] Overall Test Status: Fail" "\n"
      "[INFO] Total time taken by zimcheck: <3 seconds." "\n"
    );

    test_zimcheck_single_option(
        {"-u", "-U", "--url_internal"},
        POOR_ZIMFILE,
        1,
        expected_stdout,
        EMPTY_STDERR
    );
}

TEST(zimcheck, external_url_check_poorzimfile)
{
    const std::string expected_stdout(
      "[INFO] Checking zim file data/zimfiles/poor.zim" "\n"
      "[INFO] Zimcheck version is " VERSION "\n"
      "[INFO] Verifying Articles' content..." "\n"
      "[ERROR] Invalid external links found:" "\n"
      "  http://a.io/pic.png is an external dependence in article external_image_http.html" "\n"
      "  https://a.io/pic.png is an external dependence in article external_image_https.html" "\n"
      "  //a.io/pic.png is an external dependence in article external_image_protocol_relative.html" "\n"
      "[INFO] Overall Test Status: Fail" "\n"
      "[INFO] Total time taken by zimcheck: <3 seconds." "\n"
    );

    test_zimcheck_single_option(
        {"-x", "-X", "--url_external"},
        POOR_ZIMFILE,
        1,
        expected_stdout,
        EMPTY_STDERR
    );
}

TEST(zimcheck, redundant_poorzimfile)
{
    const std::string expected_stdout(
      "[INFO] Checking zim file data/zimfiles/poor.zim" "\n"
      "[INFO] Zimcheck version is " VERSION "\n"
      "[INFO] Verifying Articles' content..." "\n"
      "[INFO] Searching for redundant articles..." "\n"
      "  Verifying Similar Articles for redundancies..." "\n"
      "[WARNING] Redundant data found:" "\n"
      "  article1.html and redundant_article.html" "\n"
      "[INFO] Overall Test Status: Pass" "\n"
      "[INFO] Total time taken by zimcheck: <3 seconds." "\n"
    );

    test_zimcheck_single_option(
        {"-r", "-R", "--redundant"},
        POOR_ZIMFILE,
        0,
        expected_stdout,
        EMPTY_STDERR
    );
}

TEST(zimcheck, redirect_loop_poorzimfile)
{
  const std::string expected_output(
    "[INFO] Checking zim file data/zimfiles/poor.zim" "\n"
    "[INFO] Zimcheck version is " VERSION "\n"
    "[INFO] Checking for redirect loops..." "\n"
    "[ERROR] Redirect loop(s) exist:" "\n"
    "  Redirect loop exists from entry redirect_loop.html" "\n"
    "" "\n"
    "  Redirect loop exists from entry redirect_loop2.html" "\n"
    "" "\n"
    "  Redirect loop exists from entry redirect_loop3.html" "\n"
    "" "\n"
    "[INFO] Overall Test Status: Fail" "\n"
    "[INFO] Total time taken by zimcheck: <3 seconds." "\n"
  );

  test_zimcheck_single_option(
    {"-l", "-L", "--redirect_loop"},
    POOR_ZIMFILE,
    1,
    expected_output,
    EMPTY_STDERR
  );
}

const std::string ALL_CHECKS_OUTPUT_ON_POORZIMFILE(
      "[INFO] Checking zim file data/zimfiles/poor.zim" "\n"
      "[INFO] Zimcheck version is " VERSION "\n"
      "[INFO] Verifying ZIM-archive structure integrity..." "\n"
      "[INFO] Avoiding redundant checksum test (already performed by the integrity check)." "\n"
      "[INFO] Checking metadata..." "\n"
      "[INFO] Searching for Favicon..." "\n"
      "[INFO] Searching for main page..." "\n"
      "[INFO] Verifying Articles' content..." "\n"
      "[INFO] Searching for redundant articles..." "\n"
      "  Verifying Similar Articles for redundancies..." "\n"
      "[INFO] Checking for redirect loops..." "\n"
      "[ERROR] Empty articles:" "\n"
      "  Entry empty.html is empty" "\n"
      "[ERROR] Metadata errors:" "\n"
      "  Missing mandatory metadata: Title" "\n"
      "  Missing mandatory metadata: Description" "\n"
      "  Missing mandatory metadata: Illustration_48x48@1" "\n"
      "  Language must contain at least 3 characters" "\n"
      "  Language doesn't match regex: \\w{3}(,\\w{3})*" "\n"
      "[ERROR] Favicon:" "\n"
      "  Favicon is missing" "\n"
      "[ERROR] Missing mainpage:" "\n"
      "  Main Page Index stored in Archive Header: 4294967295" "\n"
      "[WARNING] Redundant data found:" "\n"
      "  article1.html and redundant_article.html" "\n"
      "[ERROR] Invalid internal links found:" "\n"
      "  The following links:" "\n"
      "- A/non_existent.html" "\n"
      "(A/non_existent.html) were not found in article dangling_link.html" "\n"
      "  Found 1 empty links in article: empty_link.html" "\n"
      "  ../../oops.html is out of bounds. Article: outofbounds_link.html" "\n"
      "[ERROR] Invalid external links found:" "\n"
      "  http://a.io/pic.png is an external dependence in article external_image_http.html" "\n"
      "  https://a.io/pic.png is an external dependence in article external_image_https.html" "\n"
      "  //a.io/pic.png is an external dependence in article external_image_protocol_relative.html" "\n"
      "[ERROR] Redirect loop(s) exist:" "\n"
      "  Redirect loop exists from entry redirect_loop.html" "\n"
      "" "\n"
      "  Redirect loop exists from entry redirect_loop2.html" "\n"
      "" "\n"
      "  Redirect loop exists from entry redirect_loop3.html" "\n"
      "" "\n"
      "[INFO] Overall Test Status: Fail" "\n"
      "[INFO] Total time taken by zimcheck: <3 seconds." "\n"
);

TEST(zimcheck, nooptions_poorzimfile)
{
    CapturedStdout zimcheck_output;
    ASSERT_EQ(1, zimcheck({"zimcheck", POOR_ZIMFILE}));

    ASSERT_EQ(ALL_CHECKS_OUTPUT_ON_POORZIMFILE, std::string(zimcheck_output));
}

TEST(zimcheck, all_checks_poorzimfile)
{
    test_zimcheck_single_option(
        {"-a", "-A", "--all"},
        POOR_ZIMFILE,
        1,
        ALL_CHECKS_OUTPUT_ON_POORZIMFILE,
        EMPTY_STDERR
    );
}

TEST(zimcheck, json_bad_checksum)
{
    CapturedStdout zimcheck_output;
    ASSERT_EQ(1, zimcheck({
      "zimcheck",
      "--json",
      "-C",
      "data/zimfiles/bad_checksum.zim"
    }));

    ASSERT_EQ(
      "{"                                                                   "\n"
      "  \"zimcheck_version\" : \"" VERSION "\","                           "\n"
      "  \"checks\" : ["                                                    "\n"
      "    \"checksum\""                                                    "\n"
      "  ],"                                                                "\n"
      "  \"file_name\" : \"data/zimfiles/bad_checksum.zim\","               "\n"
      "  \"file_uuid\" : \"00000000-0000-0000-0000-000000000000\","         "\n"
      "  \"status\" : false,"                                               "\n"
      "  \"logs\" : ["                                                      "\n"
      "    {"                                                               "\n"
      "      \"check\" : \"checksum\","                                     "\n"
      "      \"level\" : \"ERROR\","                                        "\n"
      "      \"message\" : \"ZIM Archive Checksum in archive: 00000000000000000000000000000000\\n\"," "\n"
      "      \"archive_checksum\" : \"00000000000000000000000000000000\""   "\n"
      "    }"                                                               "\n"
      "  ]"                                                                 "\n"
      "}"                                                                   "\n"
      , std::string(zimcheck_output)
    );
}

TEST(zimcheck, json_poorzimfile)
{
    CapturedStdout zimcheck_output;
    ASSERT_EQ(1, zimcheck({"zimcheck", "--json", POOR_ZIMFILE}));

    ASSERT_EQ(
      "{"                                                                   "\n"
      "  \"zimcheck_version\" : \"" VERSION "\","                           "\n"
      "  \"checks\" : ["                                                    "\n"
      "    \"checksum\","                                                   "\n"
      "    \"integrity\","                                                  "\n"
      "    \"empty\","                                                      "\n"
      "    \"metadata\","                                                   "\n"
      "    \"favicon\","                                                    "\n"
      "    \"main_page\","                                                  "\n"
      "    \"redundant\","                                                  "\n"
      "    \"url_internal\","                                               "\n"
      "    \"url_external\","                                               "\n"
      "    \"redirect\""                                                    "\n"
      "  ],"                                                                "\n"
      "  \"file_name\" : \"data/zimfiles/poor.zim\","                       "\n"
      "  \"file_uuid\" : \"00000000-0000-0000-0000-000000000000\","         "\n"
      "  \"status\" : false,"                                               "\n"
      "  \"logs\" : ["                                                      "\n"
      "    {"                                                               "\n"
      "      \"check\" : \"empty\","                                        "\n"
      "      \"level\" : \"ERROR\","                                        "\n"
      "      \"message\" : \"Entry empty.html is empty\","                  "\n"
      "      \"path\" : \"empty.html\""                                     "\n"
      "    },"                                                              "\n"
      "    {"                                                               "\n"
      "      \"check\" : \"metadata\","                                     "\n"
      "      \"level\" : \"ERROR\","                                        "\n"
      "      \"message\" : \"Missing mandatory metadata: Title\","          "\n"
      "      \"error\" : \"Missing mandatory metadata: Title\""             "\n"
      "    },"                                                              "\n"
      "    {"                                                               "\n"
      "      \"check\" : \"metadata\","                                     "\n"
      "      \"level\" : \"ERROR\","                                        "\n"
      "      \"message\" : \"Missing mandatory metadata: Description\","    "\n"
      "      \"error\" : \"Missing mandatory metadata: Description\""       "\n"
      "    },"                                                              "\n"
      "    {"                                                               "\n"
      "      \"check\" : \"metadata\","                                     "\n"
      "      \"level\" : \"ERROR\","                                        "\n"
      "      \"message\" : \"Missing mandatory metadata: Illustration_48x48@1\"," "\n"
      "      \"error\" : \"Missing mandatory metadata: Illustration_48x48@1\""    "\n"
      "    },"                                                              "\n"
      "    {"                                                               "\n"
      "      \"check\" : \"metadata\","                                     "\n"
      "      \"level\" : \"ERROR\","                                        "\n"
      "      \"message\" : \"Language must contain at least 3 characters\"," "\n"
      "      \"error\" : \"Language must contain at least 3 characters\""    "\n"
      "    },"                                                              "\n"
      "    {"                                                               "\n"
      "      \"check\" : \"metadata\","                                     "\n"
      "      \"level\" : \"ERROR\","                                        "\n"
      "      \"message\" : \"Language doesn't match regex: \\\\w{3}(,\\\\w{3})*\"," "\n"
      "      \"error\" : \"Language doesn't match regex: \\\\w{3}(,\\\\w{3})*\""    "\n"
      "    },"                                                              "\n"
      "    {"                                                               "\n"
      "      \"check\" : \"favicon\","                                      "\n"
      "      \"level\" : \"ERROR\","                                        "\n"
      "      \"message\" : \"Favicon is missing\""                          "\n"
      "    },"                                                              "\n"
      "    {"                                                               "\n"
      "      \"check\" : \"main_page\","                                    "\n"
      "      \"level\" : \"ERROR\","                                        "\n"
      "      \"message\" : \"Main Page Index stored in Archive Header: 4294967295\"," "\n"
      "      \"main_page_index\" : \"4294967295\""                          "\n"
      "    },"                                                              "\n"
      "    {"                                                               "\n"
      "      \"check\" : \"redundant\","                                    "\n"
      "      \"level\" : \"WARNING\","                                      "\n"
      "      \"message\" : \"article1.html and redundant_article.html\","   "\n"
      "      \"path1\" : \"article1.html\","                                "\n"
      "      \"path2\" : \"redundant_article.html\""                        "\n"
      "    },"                                                              "\n"
      "    {"                                                               "\n"
      "      \"check\" : \"url_internal\","                                 "\n"
      "      \"level\" : \"ERROR\","                                        "\n"
      "      \"message\" : \"The following links:\\n- A/non_existent.html\\n(A/non_existent.html) were not found in article dangling_link.html\"," "\n"
      "      \"links\" : ["                                                 "\n"
      "        \"A/non_existent.html\""                                     "\n"
      "      ],"                                                            "\n"
      "      \"normalized_link\" : \"A/non_existent.html\","                "\n"
      "      \"path\" : \"dangling_link.html\""                             "\n"
      "    },"                                                              "\n"
      "    {"                                                               "\n"
      "      \"check\" : \"url_internal\","                                 "\n"
      "      \"level\" : \"ERROR\","                                        "\n"
      "      \"message\" : \"Found 1 empty links in article: empty_link.html\"," "\n"
      "      \"count\" : \"1\","                                            "\n"
      "      \"path\" : \"empty_link.html\""                                "\n"
      "    },"                                                              "\n"
      "    {"                                                               "\n"
      "      \"check\" : \"url_internal\","                                 "\n"
      "      \"level\" : \"ERROR\","                                        "\n"
      "      \"message\" : \"../../oops.html is out of bounds. Article: outofbounds_link.html\"," "\n"
      "      \"link\" : \"../../oops.html\","                               "\n"
      "      \"path\" : \"outofbounds_link.html\""                          "\n"
      "    },"                                                              "\n"
      "    {"                                                               "\n"
      "      \"check\" : \"url_external\","                                 "\n"
      "      \"level\" : \"ERROR\","                                        "\n"
      "      \"message\" : \"http://a.io/pic.png is an external dependence in article external_image_http.html\"," "\n"
      "      \"link\" : \"http://a.io/pic.png\","                           "\n"
      "      \"path\" : \"external_image_http.html\""                             "\n"
      "    },"                                                              "\n"
      "    {"                                                               "\n"
      "      \"check\" : \"url_external\","                                 "\n"
      "      \"level\" : \"ERROR\","                                        "\n"
      "      \"message\" : \"https://a.io/pic.png is an external dependence in article external_image_https.html\"," "\n"
      "      \"link\" : \"https://a.io/pic.png\","                           "\n"
      "      \"path\" : \"external_image_https.html\""                             "\n"
      "    },"                                                              "\n"
      "    {"                                                               "\n"
      "      \"check\" : \"url_external\","                                 "\n"
      "      \"level\" : \"ERROR\","                                        "\n"
      "      \"message\" : \"//a.io/pic.png is an external dependence in article external_image_protocol_relative.html\"," "\n"
      "      \"link\" : \"//a.io/pic.png\","                           "\n"
      "      \"path\" : \"external_image_protocol_relative.html\""                             "\n"
      "    },"                                                              "\n"
      "    {"                                                               "\n"
      "      \"check\" : \"redirect\","                                     "\n"
      "      \"level\" : \"ERROR\","                                        "\n"
      "      \"message\" : \"Redirect loop exists from entry redirect_loop.html\\n\"," "\n"
      "      \"entry_path\" : \"redirect_loop.html\""                       "\n"
      "    },"                                                              "\n"
      "    {"                                                               "\n"
      "      \"check\" : \"redirect\","                                     "\n"
      "      \"level\" : \"ERROR\","                                        "\n"
      "      \"message\" : \"Redirect loop exists from entry redirect_loop2.html\\n\"," "\n"
      "      \"entry_path\" : \"redirect_loop2.html\""                      "\n"
      "    },"                                                              "\n"
      "    {"                                                               "\n"
      "      \"check\" : \"redirect\","                                     "\n"
      "      \"level\" : \"ERROR\","                                        "\n"
      "      \"message\" : \"Redirect loop exists from entry redirect_loop3.html\\n\"," "\n"
      "      \"entry_path\" : \"redirect_loop3.html\""                      "\n"
      "    }"                                                               "\n"
      "  ]"                                                                 "\n"
      "}"                                                                   "\n"
      , std::string(zimcheck_output));
}
