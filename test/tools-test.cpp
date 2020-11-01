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

TEST(tools, isDataUrl)
{
    EXPECT_TRUE (isDataUrl("data:text/plain;charset=UTF-8,the%20data"));
    EXPECT_TRUE (isDataUrl("DATA:text/plain;charset=UTF-8,the%20data"));

    EXPECT_FALSE(isDataUrl("/api/data:text/plain;charset=UTF-8,the%20data"));
}

TEST(tools, isExternalUrl)
{
    EXPECT_TRUE (isExternalUrl("http://example.com"));
    EXPECT_TRUE (isExternalUrl("https://example.com"));
    EXPECT_TRUE (isExternalUrl("HttP://example.com"));
    EXPECT_TRUE (isExternalUrl("HtTps://example.com"));
    EXPECT_TRUE (isExternalUrl("file:///etc/passwd"));
    EXPECT_TRUE (isExternalUrl("ftp://download.kiwix.org/zim/"));
    EXPECT_TRUE (isExternalUrl("mailto:someone@example.com"));
    EXPECT_TRUE (isExternalUrl("MAILTO:someone@example.com"));
    EXPECT_TRUE (isExternalUrl("tel:+0123456789"));
    EXPECT_TRUE (isExternalUrl("TEL:+0123456789"));
    EXPECT_TRUE (isExternalUrl("geo:12.34,56.78"));
    EXPECT_TRUE (isExternalUrl("GEO:12.34,56.78"));
    EXPECT_TRUE (isExternalUrl("javascript:console.log('hello, world!')"));
    EXPECT_TRUE (isExternalUrl("JAVAscript:console.log('hello, world!')"));

    EXPECT_FALSE(isExternalUrl("http:example.com"));
    EXPECT_FALSE(isExternalUrl("http:/example.com"));
    EXPECT_FALSE(isExternalUrl("git@github.com:openzim/zim-tools.git"));
    EXPECT_FALSE(isExternalUrl("/redirect?url=http://example.com"));
    EXPECT_FALSE(isExternalUrl("redirect?url=http://example.com"));
    EXPECT_FALSE(isExternalUrl("auth.php#returnurl=https://example.com"));
    EXPECT_FALSE(isExternalUrl("/api/v1/http://example.com"));
    EXPECT_FALSE(isExternalUrl("img/file:///etc/passwd"));
    EXPECT_FALSE(isExternalUrl("ftp:/download.kiwix.org/zim/"));
    EXPECT_FALSE(isExternalUrl("sendmailto:someone@example.com"));
    EXPECT_FALSE(isExternalUrl("intel:+0123456789"));
    EXPECT_FALSE(isExternalUrl("showlocation.cgi?geo:12.34,56.78"));
    EXPECT_FALSE(isExternalUrl("/xyz/javascript:console.log('hello, world!')"));

    EXPECT_FALSE(isExternalUrl("data:text/plain;charset=UTF-8,the%20data"));
    EXPECT_FALSE(isExternalUrl("DATA:text/plain;charset=UTF-8,the%20data"));
    EXPECT_FALSE(isExternalUrl("/api/data:text/plain;charset=UTF-8,qwerty"));
    EXPECT_FALSE(isExternalUrl("../img/logo.png"));
    EXPECT_FALSE(isExternalUrl("style.css"));
}

TEST(tools, isInternalUrl)
{
    EXPECT_FALSE(isInternalUrl("http://example.com"));
    EXPECT_FALSE(isInternalUrl("https://example.com"));
    EXPECT_FALSE(isInternalUrl("HttP://example.com"));
    EXPECT_FALSE(isInternalUrl("HtTps://example.com"));
    EXPECT_FALSE(isInternalUrl("file:///etc/passwd"));
    EXPECT_FALSE(isInternalUrl("ftp://download.kiwix.org/zim/"));
    EXPECT_FALSE(isInternalUrl("mailto:someone@example.com"));
    EXPECT_FALSE(isInternalUrl("MAILTO:someone@example.com"));
    EXPECT_FALSE(isInternalUrl("tel:+0123456789"));
    EXPECT_FALSE(isInternalUrl("TEL:+0123456789"));
    EXPECT_FALSE(isInternalUrl("geo:12.34,56.78"));
    EXPECT_FALSE(isInternalUrl("GEO:12.34,56.78"));
    EXPECT_FALSE(isInternalUrl("javascript:console.log('hello, world!')"));
    EXPECT_FALSE(isInternalUrl("JAVAscript:console.log('hello, world!')"));

    EXPECT_FALSE(isInternalUrl("data:text/plain;charset=UTF-8,the%20data"));
    EXPECT_FALSE(isInternalUrl("DATA:text/plain;charset=UTF-8,the%20data"));

    EXPECT_TRUE (isInternalUrl("http:example.com"));
    EXPECT_TRUE (isInternalUrl("http:/example.com"));
    EXPECT_TRUE (isInternalUrl("git@github.com:openzim/zim-tools.git"));
    EXPECT_TRUE (isInternalUrl("/redirect?url=http://example.com"));
    EXPECT_TRUE (isInternalUrl("redirect?url=http://example.com"));
    EXPECT_TRUE (isInternalUrl("auth.php#returnurl=https://example.com"));
    EXPECT_TRUE (isInternalUrl("/api/v1/http://example.com"));
    EXPECT_TRUE (isInternalUrl("img/file:///etc/passwd"));
    EXPECT_TRUE (isInternalUrl("ftp:/download.kiwix.org/zim/"));
    EXPECT_TRUE (isInternalUrl("sendmailto:someone@example.com"));
    EXPECT_TRUE (isInternalUrl("intel:+0123456789"));
    EXPECT_TRUE (isInternalUrl("showlocation.cgi?geo:12.34,56.78"));
    EXPECT_TRUE (isInternalUrl("/xyz/javascript:console.log('hello, world!')"));

    EXPECT_TRUE (isInternalUrl("/api/data:text/plain;charset=UTF-8,qwerty"));
    EXPECT_TRUE (isInternalUrl("../img/logo.png"));
    EXPECT_TRUE (isInternalUrl("style.css"));
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
