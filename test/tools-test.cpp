#include "gtest/gtest.h"

#include "../src/tools.h"
#include <magic.h>
#include <unordered_map>

magic_t magic;
bool inflateHtmlFlag = false;
bool isVerbose() { return false; }

TEST(CommonTools, isDirectory)
{
  ASSERT_FALSE(isDirectory("data/minimal-content/favicon.png"));
  ASSERT_TRUE(isDirectory("data/minimal-content"));
}

TEST(CommonTools, base64_encode)
{
  unsigned char data[] = { 0xff, 0x00, 0x7a };
  std::string txt = base64_encode(data, sizeof(data));
  EXPECT_EQ(txt, "/wB6");
}

TEST(CommonTools, decodeUrl)
{
  std::string src = "%00";
  std::string res = decodeUrl(src);
  EXPECT_EQ(res.size(), 1u);
  EXPECT_EQ(res[0], '\0');

  src = "%ff";
  res = decodeUrl(src);
  EXPECT_EQ(res.size(), 1u);
  EXPECT_EQ(res[0], '\xff');

  std::unordered_map<const char *, const std::string> expectationsMap = {
    // test normal use
    { "https://www.example.com/cgi-bin/search.cgi?q=example%20search",
      "https://www.example.com/cgi-bin/search.cgi?q=example search" },
    { "%2a", "*" },
    // test corner cases
    { "%", "%" },
    { "%2", "%2" },
    { "%%","%%" },
    { "%%%", "%%%" },
    { "%at", "%at" },
    { "%%ft", "%%ft" },
    { "%%53", "%S"},
    { "%%5t", "%%5t"}
  };

  for (auto p : expectationsMap) {
    std::string res = decodeUrl(p.first);
    EXPECT_EQ(res, p.second);
  }
}

TEST(CommonTools, computeRelativePath)
{
  EXPECT_EQ("B", computeRelativePath("A", "B"));
  EXPECT_EQ("B/CD/EFG", computeRelativePath("A", "B/CD/EFG"));

  EXPECT_EQ("c", computeRelativePath("dir/b", "dir/c"));
  EXPECT_EQ("subdir/", computeRelativePath("dir/b", "dir/subdir/"));
  EXPECT_EQ("../c", computeRelativePath("dir/subdir/", "dir/c"));

  EXPECT_EQ("../c", computeRelativePath("A/B/f", "A/c"));
  EXPECT_EQ("D/c", computeRelativePath("A/f", "A/D/c"));

  EXPECT_EQ("c", computeRelativePath("A/B/f", "A/B/c"));
}

TEST(CommonTools, computeAbsolutePath)
{
  std::string str;

  str = computeAbsolutePath("", "");
  EXPECT_EQ(str, "");

  str = computeAbsolutePath("/home/alex/oz/zim-tools/test/data/", "minimal-content/hello.html");
  EXPECT_EQ(str, "/home/alex/oz/zim-tools/test/data/minimal-content/hello.html");

  str = computeAbsolutePath("../test/data", "minimal-content/hello.html");
  EXPECT_EQ(str, "../test/minimal-content/hello.html");

  // without trailing /  'data' component will be stripped from path:
  str = computeAbsolutePath("/home/alex/oz/zim-tools/test/data", "minimal-content/hello.html");
  EXPECT_EQ(str, "/home/alex/oz/zim-tools/test/minimal-content/hello.html");
}

TEST(CommonTools, replaceStringInPlaceOnce)
{
  std::string str;

  str = "";
  replaceStringInPlaceOnce(str, "", "");
  EXPECT_EQ(str, "");

  str = "abcd";
  replaceStringInPlace(str, "a", "");
  EXPECT_EQ(str, "bcd");

  str = "abcd";
  replaceStringInPlaceOnce(str, "a", "b");
  EXPECT_EQ(str, "bbcd");

  str = "aabcd";
  replaceStringInPlaceOnce(str, "a", "b");
  EXPECT_EQ(str, "babcd");
}

TEST(CommonTools, replaceStringInPlace)
{
  std::string str;

  str = "";
  replaceStringInPlace(str, "", "");
  EXPECT_EQ(str, "");

  str = "abcd";
  replaceStringInPlace(str, "a", "b");
  EXPECT_EQ(str, "bbcd");

  str = "abcd";
  replaceStringInPlace(str, "a", "");
  EXPECT_EQ(str, "bcd");

  str = "aabcd";
  replaceStringInPlace(str, "a", "b");
  EXPECT_EQ(str, "bbbcd");
}

TEST(CommonTools, stripTitleInvalidChars)
{
  std::string str;

  str = "\u202Aheader\u202A";
  stripTitleInvalidChars(str);
  EXPECT_EQ(str, "header");
}

UriKind uriKind(const std::string& s)
{
    return html_link::detectUriKind(s);
}

TEST(tools, uriKind)
{
    EXPECT_EQ(UriKind::GENERIC_URI, uriKind("http://example.com"));
    EXPECT_EQ(UriKind::GENERIC_URI, uriKind("https://example.com"));
    EXPECT_EQ(UriKind::GENERIC_URI, uriKind("HttP://example.com"));
    EXPECT_EQ(UriKind::GENERIC_URI, uriKind("HtTps://example.com"));
    EXPECT_EQ(UriKind::GENERIC_URI, uriKind("file:///etc/passwd"));
    EXPECT_EQ(UriKind::GENERIC_URI, uriKind("ftp://download.kiwix.org/zim/"));

    EXPECT_EQ(UriKind::MAILTO, uriKind("mailto:someone@example.com"));
    EXPECT_EQ(UriKind::MAILTO, uriKind("MAILTO:someone@example.com"));

    EXPECT_EQ(UriKind::TEL, uriKind("tel:+0123456789"));
    EXPECT_EQ(UriKind::TEL, uriKind("TEL:+0123456789"));

    EXPECT_EQ(UriKind::GEO, uriKind("geo:12.34,56.78"));
    EXPECT_EQ(UriKind::GEO, uriKind("GEO:12.34,56.78"));

    EXPECT_EQ(UriKind::JAVASCRIPT, uriKind("javascript:console.log('hello!')"));
    EXPECT_EQ(UriKind::JAVASCRIPT, uriKind("JAVAscript:console.log('hello!')"));

    EXPECT_EQ(UriKind::DATA, uriKind("data:text/plain;charset=UTF-8,data"));
    EXPECT_EQ(UriKind::DATA, uriKind("DATA:text/plain;charset=UTF-8,data"));

    EXPECT_EQ(UriKind::OTHER, uriKind("http:example.com"));
    EXPECT_EQ(UriKind::OTHER, uriKind("http:/example.com"));
    EXPECT_EQ(UriKind::OTHER, uriKind("git@github.com:openzim/zim-tools.git"));
    EXPECT_EQ(UriKind::OTHER, uriKind("/redirect?url=http://example.com"));
    EXPECT_EQ(UriKind::OTHER, uriKind("redirect?url=http://example.com"));
    EXPECT_EQ(UriKind::OTHER, uriKind("auth.php#returnurl=https://example.com"));
    EXPECT_EQ(UriKind::OTHER, uriKind("/api/v1/http://example.com"));
    EXPECT_EQ(UriKind::OTHER, uriKind("img/file:///etc/passwd"));
    EXPECT_EQ(UriKind::OTHER, uriKind("ftp:/download.kiwix.org/zim/"));
    EXPECT_EQ(UriKind::OTHER, uriKind("sendmailto:someone@example.com"));
    EXPECT_EQ(UriKind::OTHER, uriKind("intel:+0123456789"));
    EXPECT_EQ(UriKind::OTHER, uriKind("showlocation.cgi?geo:12.34,56.78"));
    EXPECT_EQ(UriKind::OTHER, uriKind("/xyz/javascript:console.log('hello, world!')"));

    EXPECT_EQ(UriKind::OTHER, uriKind("/api/data:text/plain;charset=UTF-8,qwerty"));
    EXPECT_EQ(UriKind::OTHER, uriKind("../img/logo.png"));
    EXPECT_EQ(UriKind::OTHER, uriKind("style.css"));
}

TEST(tools, isOutofBounds)
{
    ASSERT_FALSE(isOutofBounds("", ""));
    ASSERT_TRUE(isOutofBounds("../../..", ""));
    ASSERT_TRUE(isOutofBounds("../", ""));
    ASSERT_FALSE(isOutofBounds("../", "/a/b"));
    ASSERT_FALSE(isOutofBounds("../", "/a"));
    ASSERT_TRUE(isOutofBounds("../../", "/a"));
    ASSERT_TRUE(isOutofBounds("../../../-/s/css_modules/ext.cite.ux-enhancements.css", "A/Blood_/"));
}

TEST(tools, normalize_link)
{
    ASSERT_EQ(normalize_link("/a", "/b"), "a");

    // not absolute
    ASSERT_EQ(normalize_link("a", "/b"), "/b/a");
    ASSERT_EQ(normalize_link("../a", "/b/c"), "/b/a");
    ASSERT_EQ(normalize_link(".././a", "/b/c"), "/b/a");
    ASSERT_EQ(normalize_link("../a/b/aa#localanchor", "/b/c"), "/b/a/b/aa");
    ASSERT_EQ(normalize_link("../a/b/aa?localanchor", "/b/c"), "/b/a/b/aa");
}

TEST(tools, addler32)
{
    ASSERT_EQ(adler32("sdfkhewruhwe8"), 640746832);
    ASSERT_EQ(adler32("sdifjsdf"), 251593550);
    ASSERT_EQ(adler32("q"), 7471218);
    ASSERT_EQ(adler32(""), 1);
}

TEST(tools, getLinks)
{
    auto v = generic_getLinks("");

    ASSERT_TRUE(v.empty());

    std::string page1 = "<link href=\"https://fonts.goos.com/css?family=OpenSans\" rel=\"stylesheet\">";
    auto v1 = generic_getLinks(page1);

    ASSERT_TRUE(v1.size() == 1);
    ASSERT_EQ(v1[0].attribute, "href");
    ASSERT_EQ(v1[0].link, "https://fonts.goos.com/css?family=OpenSans");

    std::string page2 = "<link href=\"https://fonts.goos.com/css?family=OpenSans\" rel=\"stylesheet\">";
    auto v2 = generic_getLinks(page2);

    ASSERT_TRUE(v2.size() == 1);
    ASSERT_EQ(v1[0].attribute, "href");

    std::string page3 = "<link src=\"https://fonts.goos.com/css?family=OpenSans\" rel=\"stylesheet\">";
    auto v3 = generic_getLinks(page3);

    ASSERT_TRUE(v3.size() == 1);
    ASSERT_EQ(v3[0].attribute, "src");
    ASSERT_EQ(v3[0].link, "https://fonts.goos.com/css?family=OpenSans");
}
