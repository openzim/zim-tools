#include "gtest/gtest.h"
#include "../src/tools.h"
#include "../src/zimwriterfs/tools.h"
#include <magic.h>
#include <unordered_map>

magic_t magic;
bool inflateHtmlFlag = false;
bool isVerbose() { return false; }

TEST(CommonTools, isDirectory)
{
  EXPECT_FALSE(isDirectory("data/minimal-content/favicon.png"));
  EXPECT_TRUE(isDirectory("data/minimal-content"));
}

TEST(CommonTools, fileExists)
{
  EXPECT_TRUE(fileExists("data/minimal-content/favicon.png"));
  EXPECT_FALSE(fileExists("data/minimal-content"));
  EXPECT_FALSE(fileExists("data/minimal-content/non_existent_file.png"));
}

TEST(CommonTools, getMimeTypeForFile)
{
  EXPECT_EQ(getMimeTypeForFile("data/minimal-content", "favicon.png"), "image/png");
  EXPECT_EQ(getMimeTypeForFile("data/minimal-content", "nonexistentnoext"), "application/octet-stream");
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
  EXPECT_EQ("A",  computeRelativePath("A" , "A" ));
  EXPECT_EQ("..", computeRelativePath("A/", "A" ));
  EXPECT_EQ("A/", computeRelativePath("A" , "A/"));
  EXPECT_EQ("./", computeRelativePath("A/", "A/"));

  EXPECT_EQ("B",     computeRelativePath("A" , "B" ));
  EXPECT_EQ("../B",  computeRelativePath("A/", "B" ));
  EXPECT_EQ("B/",    computeRelativePath("A" , "B/"));
  EXPECT_EQ("../B/", computeRelativePath("A/", "B/"));

  EXPECT_EQ("A/B",  computeRelativePath("A" , "A/B" ));
  EXPECT_EQ("B",    computeRelativePath("A/", "A/B" ));
  EXPECT_EQ("A/B/", computeRelativePath("A" , "A/B/"));
  EXPECT_EQ("B/",   computeRelativePath("A/", "A/B/"));

  EXPECT_EQ("..",     computeRelativePath("A/B",  "A" ));
  EXPECT_EQ("../..",  computeRelativePath("A/B/", "A" ));
  EXPECT_EQ("../",    computeRelativePath("A/B",  "A/"));
  EXPECT_EQ("../../", computeRelativePath("A/B/", "A/"));

  EXPECT_EQ("B/CD/EFG", computeRelativePath("A", "B/CD/EFG"));

  EXPECT_EQ("c", computeRelativePath("dir/b", "dir/c"));
  EXPECT_EQ("b", computeRelativePath("dir/b", "dir/b"));
  EXPECT_EQ("b/c", computeRelativePath("dir/b", "dir/b/c"));
  EXPECT_EQ("..", computeRelativePath("dir/b/c", "dir/b"));
  EXPECT_EQ("subdir/", computeRelativePath("dir/b", "dir/subdir/"));
  EXPECT_EQ("../", computeRelativePath("dir/subdir/b", "dir/subdir/"));
  EXPECT_EQ("..", computeRelativePath("dir/subdir/b", "dir/subdir"));
  EXPECT_EQ("../../", computeRelativePath("dir/subdir/b/c", "dir/subdir/"));
  EXPECT_EQ("../..", computeRelativePath("dir/subdir/b/c", "dir/subdir"));
  EXPECT_EQ("../c", computeRelativePath("dir/subdir/", "dir/c"));

  EXPECT_EQ("../c", computeRelativePath("A/B/f", "A/c"));
  EXPECT_EQ("D/c", computeRelativePath("A/f", "A/D/c"));

  EXPECT_EQ("c", computeRelativePath("A/B/f", "A/B/c"));
  EXPECT_EQ("c", computeRelativePath("A/B/c", "A/B/c"));
  EXPECT_EQ("c/d", computeRelativePath("A/B/c", "A/B/c/d"));
  EXPECT_EQ("..", computeRelativePath("A/B/c/d", "A/B/c"));
  EXPECT_EQ("../..", computeRelativePath("A/B/c/d", "A/B"));
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

    EXPECT_EQ(UriKind::PROTOCOL_RELATIVE, uriKind("//example.com"));

    EXPECT_EQ(UriKind::MAILTO, uriKind("mailto:someone@example.com"));
    EXPECT_EQ(UriKind::MAILTO, uriKind("MAILTO:someone@example.com"));

    EXPECT_EQ(UriKind::TEL, uriKind("tel:+0123456789"));
    EXPECT_EQ(UriKind::TEL, uriKind("TEL:+0123456789"));

    EXPECT_EQ(UriKind::SIP, uriKind("sip:1-999-123-4567@voip-provider.example.net"));
    EXPECT_EQ(UriKind::SIP, uriKind("SIP:1-999-123-4567@voip-provider.example.net"));

    EXPECT_EQ(UriKind::GEO, uriKind("geo:12.34,56.78"));
    EXPECT_EQ(UriKind::GEO, uriKind("GEO:12.34,56.78"));

    EXPECT_EQ(UriKind::JAVASCRIPT, uriKind("javascript:console.log('hello!')"));
    EXPECT_EQ(UriKind::JAVASCRIPT, uriKind("JAVAscript:console.log('hello!')"));

    EXPECT_EQ(UriKind::DATA, uriKind("data:text/plain;charset=UTF-8,data"));
    EXPECT_EQ(UriKind::DATA, uriKind("DATA:text/plain;charset=UTF-8,data"));

    EXPECT_EQ(UriKind::GENERIC_URI, uriKind("http:example.com"));
    EXPECT_EQ(UriKind::GENERIC_URI, uriKind("http:/example.com"));
    EXPECT_EQ(UriKind::OTHER, uriKind("git@github.com:openzim/zim-tools.git"));
    EXPECT_EQ(UriKind::OTHER, uriKind("/redirect?url=http://example.com"));
    EXPECT_EQ(UriKind::OTHER, uriKind("redirect?url=http://example.com"));
    EXPECT_EQ(UriKind::OTHER, uriKind("auth.php#returnurl=https://example.com"));
    EXPECT_EQ(UriKind::OTHER, uriKind("/api/v1/http://example.com"));
    EXPECT_EQ(UriKind::OTHER, uriKind("img/file:///etc/passwd"));
    EXPECT_EQ(UriKind::GENERIC_URI, uriKind("ftp:/download.kiwix.org/zim/"));
    EXPECT_EQ(UriKind::GENERIC_URI, uriKind("sendmailto:someone@example.com"));
    EXPECT_EQ(UriKind::GENERIC_URI, uriKind("intel:+0123456789"));
    EXPECT_EQ(UriKind::GENERIC_URI, uriKind("custom-scheme:opaque-value"));
    EXPECT_EQ(UriKind::GENERIC_URI, uriKind("My2+scheme.value:opaque-value"));
    EXPECT_EQ(UriKind::OTHER, uriKind("\xC3\xA9scheme:value"));
    EXPECT_EQ(UriKind::OTHER, uriKind("custom\xC3\xA9:value"));
    EXPECT_EQ(UriKind::OTHER, uriKind("showlocation.cgi?geo:12.34,56.78"));
    EXPECT_EQ(UriKind::OTHER, uriKind("/xyz/javascript:console.log('hello, world!')"));

    EXPECT_EQ(UriKind::OTHER, uriKind("/"));
    EXPECT_EQ(UriKind::OTHER, uriKind("/api/data:text/plain;charset=UTF-8,qwerty"));
    EXPECT_EQ(UriKind::OTHER, uriKind("../img/logo.png"));
    EXPECT_EQ(UriKind::OTHER, uriKind("style.css"));
}

#define EXPECT_INTERNAL_LINK(url) \
    do { \
        EXPECT_FALSE(html_link(html_link::HREF, (url)).isExternalUrl()); \
        EXPECT_FALSE(html_link(html_link::SRC, (url)).isExternalUrl()); \
    } while (false)

#define EXPECT_EXTERNAL_LINK(url) \
    do { \
        EXPECT_TRUE(html_link(html_link::HREF, (url)).isExternalUrl()); \
        EXPECT_TRUE(html_link(html_link::SRC, (url)).isExternalUrl()); \
    } while (false)

TEST(tools, linkClassification)
{
    EXPECT_INTERNAL_LINK("data:text/plain,payload");
    EXPECT_INTERNAL_LINK("../css/dark.css");
    EXPECT_INTERNAL_LINK("/js/light.js");
    EXPECT_INTERNAL_LINK("goodbye");
    EXPECT_INTERNAL_LINK("git@github.com:openzim/zim-tools.git");
    EXPECT_INTERNAL_LINK("/redirect?url=http://example.com");
    EXPECT_INTERNAL_LINK("redirect?url=http://example.com");
    EXPECT_INTERNAL_LINK("?page=2");
    EXPECT_INTERNAL_LINK("#footnote1e100");
    EXPECT_INTERNAL_LINK("auth.php#returnurl=https://example.com");
    EXPECT_INTERNAL_LINK("/api/v1/http://example.com");
    EXPECT_INTERNAL_LINK("img/file:///etc/passwd");

    EXPECT_EXTERNAL_LINK("custom-scheme:opaque-value");
    EXPECT_EXTERNAL_LINK("http://example.com");
    EXPECT_EXTERNAL_LINK("mailto:someone@example.com");
    EXPECT_EXTERNAL_LINK("//example.com/welcome");
}

#undef EXPECT_INTERNAL_LINK
#undef EXPECT_EXTERNAL_LINK

TEST(tools, resolveLinkTarget)
{
    EXPECT_THROW(resolveLinkTarget("/", ""),    AbsolutePathURL);
    EXPECT_THROW(resolveLinkTarget("/a", "/b"), AbsolutePathURL);

    EXPECT_EQ(resolveLinkTarget("",        ""), "");
    EXPECT_EQ(resolveLinkTarget("?q=who",  ""), "");
    EXPECT_EQ(resolveLinkTarget("#intro",  ""), "");
    EXPECT_EQ(resolveLinkTarget(".",       ""), "");
    EXPECT_EQ(resolveLinkTarget(".xyz",    ""), ".xyz");
    EXPECT_EQ(resolveLinkTarget("./",      ""), "");
    EXPECT_EQ(resolveLinkTarget("./xyz",   ""), "xyz");
    EXPECT_EQ(resolveLinkTarget("xyz",     ""), "xyz");
    EXPECT_EQ(resolveLinkTarget("./x/y/z", ""), "x/y/z");
    EXPECT_EQ(resolveLinkTarget("x/y/z",   ""), "x/y/z");
    EXPECT_EQ(resolveLinkTarget(".//",     ""), "/");
    EXPECT_EQ(resolveLinkTarget(".//xy",   ""), "/xy");
    EXPECT_THROW(resolveLinkTarget("./..", ""), OutOfBoundsURL);
    EXPECT_THROW(resolveLinkTarget("..",   ""), OutOfBoundsURL);
    EXPECT_THROW(resolveLinkTarget("../",  ""), OutOfBoundsURL);

    EXPECT_EQ(resolveLinkTarget("",        "/"), "/");
    EXPECT_EQ(resolveLinkTarget("?page=1", "/"), "/");
    EXPECT_EQ(resolveLinkTarget("#note3",  "/"), "/");
    EXPECT_EQ(resolveLinkTarget(".",       "/"), "/");
    EXPECT_EQ(resolveLinkTarget(".xyz",    "/"), "/.xyz");
    EXPECT_EQ(resolveLinkTarget("./",      "/"), "/");
    EXPECT_EQ(resolveLinkTarget("./xyz",   "/"), "/xyz");
    EXPECT_EQ(resolveLinkTarget(".//",     "/"), "//");
    EXPECT_EQ(resolveLinkTarget(".//xy",   "/"), "//xy");
    EXPECT_EQ(resolveLinkTarget("./..",    "/"), "");
    EXPECT_EQ(resolveLinkTarget("./../",   "/"), "");
    EXPECT_EQ(resolveLinkTarget("./..xyz", "/"), "/..xyz");
    EXPECT_EQ(resolveLinkTarget("..",      "/"), "");
    EXPECT_EQ(resolveLinkTarget("..xyz",   "/"), "/..xyz");
    EXPECT_EQ(resolveLinkTarget("../",     "/"), "");
    EXPECT_EQ(resolveLinkTarget("../.",    "/"), "");
    EXPECT_EQ(resolveLinkTarget(".././",   "/"), "");
    EXPECT_EQ(resolveLinkTarget(".././x",  "/"), "x");
    EXPECT_EQ(resolveLinkTarget("../.xyz", "/"), ".xyz");
    EXPECT_EQ(resolveLinkTarget("../xyz",  "/"), "xyz");
    EXPECT_EQ(resolveLinkTarget("..//",    "/"), "/");
    EXPECT_EQ(resolveLinkTarget("..//x",   "/"), "/x");
    EXPECT_THROW(resolveLinkTarget("../..", "/"), OutOfBoundsURL);

    EXPECT_EQ(resolveLinkTarget("",        "b/c/def"), "b/c/def");
    EXPECT_EQ(resolveLinkTarget("?login",  "b/c/def"), "b/c/def");
    EXPECT_EQ(resolveLinkTarget("#a/b/c",  "b/c/def"), "b/c/def");
    EXPECT_EQ(resolveLinkTarget(".",       "b/c/def"), "b/c/");
    EXPECT_EQ(resolveLinkTarget(".xyz",    "b/c/def"), "b/c/.xyz");
    EXPECT_EQ(resolveLinkTarget("./",      "b/c/def"), "b/c/");
    EXPECT_EQ(resolveLinkTarget("./xyz",   "b/c/def"), "b/c/xyz");
    EXPECT_EQ(resolveLinkTarget(".//",     "b/c/def"), "b/c//");
    EXPECT_EQ(resolveLinkTarget(".//xy",   "b/c/def"), "b/c//xy");
    EXPECT_EQ(resolveLinkTarget("./..",    "b/c/def"), "b/");
    EXPECT_EQ(resolveLinkTarget("./../",   "b/c/def"), "b/");
    EXPECT_EQ(resolveLinkTarget("./..xyz", "b/c/def"), "b/c/..xyz");
    EXPECT_EQ(resolveLinkTarget("..",      "b/c/def"), "b/");
    EXPECT_EQ(resolveLinkTarget("..xyz",   "b/c/def"), "b/c/..xyz");
    EXPECT_EQ(resolveLinkTarget("../",     "b/c/def"), "b/");
    EXPECT_EQ(resolveLinkTarget("../.",    "b/c/def"), "b/");
    EXPECT_EQ(resolveLinkTarget(".././",   "b/c/def"), "b/");
    EXPECT_EQ(resolveLinkTarget(".././x",  "b/c/def"), "b/x");
    EXPECT_EQ(resolveLinkTarget("../.xyz", "b/c/def"), "b/.xyz");
    EXPECT_EQ(resolveLinkTarget("../xyz",  "b/c/def"), "b/xyz");
    EXPECT_EQ(resolveLinkTarget("..//",    "b/c/def"), "b//");
    EXPECT_EQ(resolveLinkTarget("..//x",   "b/c/def"), "b//x");
    EXPECT_EQ(resolveLinkTarget("../..",   "b/c/def"), "");
    EXPECT_EQ(resolveLinkTarget("../../",  "b/c/def"), "");
    EXPECT_EQ(resolveLinkTarget("../..//", "b/c/def"), "/");
    EXPECT_THROW(resolveLinkTarget("../../..", "b/c/def"), OutOfBoundsURL);

    EXPECT_EQ(resolveLinkTarget("",        "b/c/"), "b/c/");
    EXPECT_EQ(resolveLinkTarget("?x=9&y=1","b/c/"), "b/c/");
    EXPECT_EQ(resolveLinkTarget("#sec5.2", "b/c/"), "b/c/");
    EXPECT_EQ(resolveLinkTarget(".",       "b/c/"), "b/c/");
    EXPECT_EQ(resolveLinkTarget(".xyz",    "b/c/"), "b/c/.xyz");
    EXPECT_EQ(resolveLinkTarget("./",      "b/c/"), "b/c/");
    EXPECT_EQ(resolveLinkTarget("./xyz",   "b/c/"), "b/c/xyz");
    EXPECT_EQ(resolveLinkTarget(".//",     "b/c/"), "b/c//");
    EXPECT_EQ(resolveLinkTarget(".//xy",   "b/c/"), "b/c//xy");
    EXPECT_EQ(resolveLinkTarget("./..",    "b/c/"), "b/");
    EXPECT_EQ(resolveLinkTarget("./../",   "b/c/"), "b/");
    EXPECT_EQ(resolveLinkTarget("./..xyz", "b/c/"), "b/c/..xyz");
    EXPECT_EQ(resolveLinkTarget("..",      "b/c/"), "b/");
    EXPECT_EQ(resolveLinkTarget("..xyz",   "b/c/"), "b/c/..xyz");
    EXPECT_EQ(resolveLinkTarget("../",     "b/c/"), "b/");
    EXPECT_EQ(resolveLinkTarget("../.",    "b/c/"), "b/");
    EXPECT_EQ(resolveLinkTarget(".././",   "b/c/"), "b/");
    EXPECT_EQ(resolveLinkTarget(".././x",  "b/c/"), "b/x");
    EXPECT_EQ(resolveLinkTarget("../.xyz", "b/c/"), "b/.xyz");
    EXPECT_EQ(resolveLinkTarget("../xyz",  "b/c/"), "b/xyz");
    EXPECT_EQ(resolveLinkTarget("..//",    "b/c/"), "b//");
    EXPECT_EQ(resolveLinkTarget("..//x",   "b/c/"), "b//x");
    EXPECT_EQ(resolveLinkTarget("../..",   "b/c/"), "");
    EXPECT_EQ(resolveLinkTarget("../../",  "b/c/"), "");
    EXPECT_EQ(resolveLinkTarget("../..//", "b/c/"), "/");
    EXPECT_THROW(resolveLinkTarget("../../..", "b/c/"), OutOfBoundsURL);

    EXPECT_EQ(resolveLinkTarget("",        "/b/c/uiop"), "/b/c/uiop");
    EXPECT_EQ(resolveLinkTarget("?lang=en","/b/c/uiop"), "/b/c/uiop");
    EXPECT_EQ(resolveLinkTarget("#home",   "/b/c/uiop"), "/b/c/uiop");
    EXPECT_EQ(resolveLinkTarget(".",       "/b/c/uiop"), "/b/c/");
    EXPECT_EQ(resolveLinkTarget(".xyz",    "/b/c/uiop"), "/b/c/.xyz");
    EXPECT_EQ(resolveLinkTarget("./",      "/b/c/uiop"), "/b/c/");
    EXPECT_EQ(resolveLinkTarget("./xyz",   "/b/c/uiop"), "/b/c/xyz");
    EXPECT_EQ(resolveLinkTarget(".//",     "/b/c/uiop"), "/b/c//");
    EXPECT_EQ(resolveLinkTarget(".//xy",   "/b/c/uiop"), "/b/c//xy");
    EXPECT_EQ(resolveLinkTarget("./..",    "/b/c/uiop"), "/b/");
    EXPECT_EQ(resolveLinkTarget("./../",   "/b/c/uiop"), "/b/");
    EXPECT_EQ(resolveLinkTarget("./..xyz", "/b/c/uiop"), "/b/c/..xyz");
    EXPECT_EQ(resolveLinkTarget("..",      "/b/c/uiop"), "/b/");
    EXPECT_EQ(resolveLinkTarget("..xyz",   "/b/c/uiop"), "/b/c/..xyz");
    EXPECT_EQ(resolveLinkTarget("../",     "/b/c/uiop"), "/b/");
    EXPECT_EQ(resolveLinkTarget("../.",    "/b/c/uiop"), "/b/");
    EXPECT_EQ(resolveLinkTarget(".././",   "/b/c/uiop"), "/b/");
    EXPECT_EQ(resolveLinkTarget(".././x",  "/b/c/uiop"), "/b/x");
    EXPECT_EQ(resolveLinkTarget("../.xyz", "/b/c/uiop"), "/b/.xyz");
    EXPECT_EQ(resolveLinkTarget("../xyz",  "/b/c/uiop"), "/b/xyz");
    EXPECT_EQ(resolveLinkTarget("..//",    "/b/c/uiop"), "/b//");
    EXPECT_EQ(resolveLinkTarget("..//x",   "/b/c/uiop"), "/b//x");
    EXPECT_EQ(resolveLinkTarget("../..",   "/b/c/uiop"), "/");
    EXPECT_EQ(resolveLinkTarget("../../",  "/b/c/uiop"), "/");
    EXPECT_EQ(resolveLinkTarget("../..//", "/b/c/uiop"), "//");
    EXPECT_EQ(resolveLinkTarget("../../..", "/b/c/uiop"), "");

    EXPECT_EQ(resolveLinkTarget("../a/b/aa#localanchor", "/b/c"), "/a/b/aa");
    EXPECT_EQ(resolveLinkTarget("../a/b/aa?localanchor", "/b/c"), "/a/b/aa");

    // Link resolution gets rid of ./ and/or ../ but not // in the URL
    EXPECT_EQ(resolveLinkTarget("../one/./two//three", "/zero/uno"), "/one/two//three");

    // Link resolution is not confused by spurious occurrences of ./ and ../
    EXPECT_EQ(resolveLinkTarget("abc./xyz", ""), "abc./xyz");
    EXPECT_EQ(resolveLinkTarget("abc../xyz", ""), "abc../xyz");
    EXPECT_EQ(resolveLinkTarget("ab.cd./xyz", "/QW/E/RT"), "/QW/E/ab.cd./xyz");
    EXPECT_EQ(resolveLinkTarget("a..b../xyz", "/QW/E/RT"), "/QW/E/a..b../xyz");
    EXPECT_EQ(resolveLinkTarget("x/y",  "/AS/DF/GHJ" ), "/AS/DF/x/y");
    EXPECT_EQ(resolveLinkTarget("x/y",  "AS.DF./qwerty"), "AS.DF./x/y");
    EXPECT_EQ(resolveLinkTarget("x/y",  "/A.S../qwerty"), "/A.S../x/y");

    // A series of adjacent slashes is considered as a chain of empty-named
    // "directories" in the path
    EXPECT_EQ(resolveLinkTarget("../leaf", "/top///bottom"), "/top//leaf");
    EXPECT_EQ(resolveLinkTarget("../../leaf", "/top///bottom"), "/top/leaf");
    EXPECT_EQ(resolveLinkTarget("../../../leaf", "/top///bottom"), "/leaf");
    EXPECT_EQ(resolveLinkTarget("../../../../leaf", "/top///bottom"), "leaf");
    EXPECT_EQ(resolveLinkTarget("lib//../python", "/usr"), "/lib/python");

    // URI-decoding is performed on the first but not second argument
    EXPECT_EQ(resolveLinkTarget("./%64%65%66", "/%41%62c/"), "/%41%62c/def");

    // '%2e" is URI-encoded '.'; check that path resolution is performed
    // on the URI-decoded version of the URL
    EXPECT_EQ(resolveLinkTarget("%2e%2e/a", "/b/c/d"), "/b/a");

    // '%2f" is URI-encoded '/'; check that absolute URL detection is performed
    // before URI-decoding of the URL
    EXPECT_EQ(resolveLinkTarget("%2fa", "/b/"), "/b//a");

    EXPECT_EQ(resolveLinkTarget("%", ""),  "%");
    EXPECT_EQ(resolveLinkTarget("%1", ""), "%1");

    EXPECT_EQ(resolveLinkTarget("%26", ""), "&");
    EXPECT_EQ(resolveLinkTarget("%27", ""), "\'");

    ///////////////////////////////////////////////////////////////////////
    // Testing of detection of out-of-bounds URLS
    ///////////////////////////////////////////////////////////////////////

    EXPECT_THROW(resolveLinkTarget("../../..", ""), OutOfBoundsURL);
    EXPECT_THROW(resolveLinkTarget("../", ""), OutOfBoundsURL);
    EXPECT_EQ(resolveLinkTarget("../", "/a/b/"), "/a/");
    EXPECT_EQ(resolveLinkTarget("..", "a/"), "");
    EXPECT_THROW(resolveLinkTarget("../..", "a/"), OutOfBoundsURL);
    EXPECT_EQ(resolveLinkTarget("../", "/a/"), "/");
    EXPECT_EQ(resolveLinkTarget("../..", "/a/"), "");
    EXPECT_THROW(resolveLinkTarget("../../..", "/a/"), OutOfBoundsURL);
    EXPECT_THROW(resolveLinkTarget("../../../-/s/css_modules/ext.cite.ux-enhancements.css", "A/Blood_/"), OutOfBoundsURL);

    EXPECT_EQ(resolveLinkTarget("css/..", ""), "");
    EXPECT_EQ(resolveLinkTarget("css/../js", ""), "js");
    EXPECT_THROW(resolveLinkTarget("css/../..", ""), OutOfBoundsURL);
    EXPECT_THROW(resolveLinkTarget("../..", "css/"), OutOfBoundsURL);
    EXPECT_THROW(resolveLinkTarget("css/../../js", ""), OutOfBoundsURL);
    EXPECT_THROW(resolveLinkTarget("../../js", "css/"), OutOfBoundsURL);
    EXPECT_THROW(resolveLinkTarget("css/../js/../..", ""), OutOfBoundsURL);
    EXPECT_THROW(resolveLinkTarget("../js/../..", "css/"), OutOfBoundsURL);
    EXPECT_THROW(resolveLinkTarget("css/../../..", ""), OutOfBoundsURL);

    // Make sure that URLs that are both absolute-path and out-of-bounds
    // result in an AbsolutePathURL exception
    EXPECT_THROW(resolveLinkTarget("/../../b",   "1/23/456/"), AbsolutePathURL);
    EXPECT_THROW(resolveLinkTarget("/a/../../b", "1/23/456/"), AbsolutePathURL);
    EXPECT_THROW(resolveLinkTarget("/../../b",   "1/23/"),     AbsolutePathURL);
    EXPECT_THROW(resolveLinkTarget("/a/../../b", "1/23/"),     AbsolutePathURL);

    // Out-of-bounds URL detection should not be confused by
    // URLs with fragment and/or search components
    EXPECT_EQ(resolveLinkTarget("faq#q=../../../../xyz", "/en/"), "/en/faq");
    EXPECT_EQ(resolveLinkTarget("faq?q=../../../../xyz", "/en/"), "/en/faq");

    // ZIM format allows arbitrary strings to be used as entry paths.
    // Check that resolveLinkTarget() treats any '..' and '.' appearing as
    // complete path segments in its second argument as regular path segments
    // (exempt from the dot-segment removal transformation).
    EXPECT_EQ(resolveLinkTarget("smoke", "home/../chimney/"), "home/../chimney/smoke");
    EXPECT_EQ(resolveLinkTarget("crow", "/home/../chimney/"), "/home/../chimney/crow");
    EXPECT_EQ(resolveLinkTarget("a/s/m", "/dot/./org/"), "/dot/./org/a/s/m");
    EXPECT_EQ(resolveLinkTarget("asm", "./org/./.././o/r/g/"), "./org/./.././o/r/g/asm");
    EXPECT_EQ(resolveLinkTarget("oops", "/a/../../../b/"), "/a/../../../b/oops");
    EXPECT_EQ(resolveLinkTarget("oops", "a/../../b/"),  "a/../../b/oops");
}

TEST(tools, addler32)
{
    EXPECT_EQ(adler32("sdfkhewruhwe8"), 640746832);
    EXPECT_EQ(adler32("sdifjsdf"), 251593550);
    EXPECT_EQ(adler32("q"), 7471218);
    EXPECT_EQ(adler32(""), 1);
}

TEST(tools, decodeHtmlEntities)
{
    EXPECT_EQ(decodeHtmlEntities(""),   "");

    // Supported HTML character references
    EXPECT_EQ(decodeHtmlEntities("&amp;"),  "&");
    EXPECT_EQ(decodeHtmlEntities("&apos;"), "'");
    EXPECT_EQ(decodeHtmlEntities("&quot;"), "\"");
    EXPECT_EQ(decodeHtmlEntities("&lt;"),   "<");
    EXPECT_EQ(decodeHtmlEntities("&gt;"),   ">");

    // All other HTML character references
    // (https://html.spec.whatwg.org/multipage/syntax.html#character-references)
    // are NOT currently supported
    EXPECT_EQ(decodeHtmlEntities("&nbsp;"), "&nbsp;");

    // Capitalized versions of supported ones do NOT work
    EXPECT_EQ(decodeHtmlEntities("&AMP;"), "&AMP;");
    EXPECT_EQ(decodeHtmlEntities("&aMP;"), "&aMP;");

    // HTML entities of the form &#dd...; and/or &#xhh...; are NOT decoded
    EXPECT_EQ(decodeHtmlEntities("&#65;"),  "&#65;" ); // should be "A"
    EXPECT_EQ(decodeHtmlEntities("&#x41;"), "&#x41;"); // should be "A"

    // Handling of "incomplete" entity
    EXPECT_EQ(decodeHtmlEntities("&amp"), "&amp");

    // No double decoding
    EXPECT_EQ(decodeHtmlEntities("&amp;lt;"), "&lt;");

    EXPECT_EQ(decodeHtmlEntities("&lt;&gt;"), "<>");

    EXPECT_EQ(decodeHtmlEntities("1&lt;2"),   "1<2");

    EXPECT_EQ(decodeHtmlEntities("3&5&gt;3/5"), "3&5>3/5");

    EXPECT_EQ(
        decodeHtmlEntities("Q&amp;A stands for &quot;Questions and answers&quot;"),
        "Q&A stands for \"Questions and answers\""
    );
}

std::string links2Str(const std::vector<html_link>& links)
{
    std::ostringstream oss;
    const char* sep = "";
    for ( const auto& l : links ) {
        const char* attr = l.attribute == html_link::SRC ? "src" : "href";
        oss << sep << "{ " << attr << ", " << l.link << " }";
        sep = "\n";
    }
    return oss.str();
}

#define EXPECT_LINKS(html, expectedStr) \
        EXPECT_EQ(links2Str(generic_getLinks(html)), expectedStr)

TEST(tools, getLinks)
{
    EXPECT_LINKS(
      "",
      ""
    );

    EXPECT_LINKS(
      R"(<link href="https://fonts.io/css?family=OpenSans" rel="stylesheet">)",
      "{ href, https://fonts.io/css?family=OpenSans }"
    );

    EXPECT_LINKS(
      R"(<link href='https://fonts.io/css?family=OpenSans' rel="stylesheet">)",
      "{ href, https://fonts.io/css?family=OpenSans }"
    );

    EXPECT_LINKS(
      R"(<link src="https://fonts.io/css?family=OpenSans" rel="stylesheet">)",
      "{ src, https://fonts.io/css?family=OpenSans }"
    );

    // URI-decoding is NOT performed on extracted links
    // (that's resolveLinkTarget()'s job)
    EXPECT_LINKS(
      "<audio controls src ='/music/It&apos;s%20only%20love.ogg'></audio>",
      "{ src, /music/It's%20only%20love.ogg }"
    );

    EXPECT_LINKS(
      R"(<a href="/R&amp;D">Research and development</a>
         blablabla
         <a href="../syntax/&lt;script&gt;">&lt;script&gt;</a>
         ...
         <a href="/Presidents/Dwight_&quot;Ike&quot;_Eisenhower">#34</a>
         <img src="https://example.com/getlogo?w=640&amp;h=480">
      )",
      "{ href, /R&D }"                                    "\n"
      "{ href, ../syntax/<script> }"                      "\n"
      "{ href, /Presidents/Dwight_\"Ike\"_Eisenhower }"   "\n"
      "{ src, https://example.com/getlogo?w=640&h=480 }"
    );

    EXPECT_LINKS(
      R"(
<html>
  <head>
    <link src = "/css/stylesheet.css" rel="stylesheet">
    <link rel="icon" href   =    '/favicon.ico'>
    <script defer crossorigin="anonymous" src="/js/analytics.js"></script>
    <!-- <script src="/js/tracker.js"></script> -->
  </head>
  <body>
    <img src="../img/welcome.png">
    <!--
      <a href="commented_out_link.htm"></a>
      <img src="commented_out_image.png">
    -->
    <pre>
      &lt;a href="not_a_link_in_example_code_block.htm"&gt;&lt;/a&gt;
      &lt;img src="not_a_link_in_example_code_block.png"&gt;
    </pre>
    Powered by <a target="_blank" href="https://kiwix.org">Kiwix</a>.
    <script src='/js/footer.js'></script>
  </body>
</html>
)",
      // links
      "{ src, /css/stylesheet.css }"                      "\n"
      "{ href, /favicon.ico }"                            "\n"
      "{ src, /js/analytics.js }"                         "\n"
      "{ src, ../img/welcome.png }"                       "\n"
      "{ href, https://kiwix.org }"                       "\n"
      "{ src, /js/footer.js }"
    );

    EXPECT_LINKS(
      R"(<div>
          <!--
            < a href="pseudolink_in_a_comment_with_an_unmatched_lt_char.htm"
          -->
          <a href="real_link.html"></a>
      </div>)",

      // links
      "{ href, real_link.html }"
    );

    EXPECT_LINKS(
      R"(<div>
          <!--
            > a href="pseudolink_in_a_comment_with_an_unmatched_gt_char.htm"
          -->
          <a href="real_link.html"></a>
      </div>)",

      // links
      "{ href, real_link.html }"
    );

    EXPECT_LINKS(
      R"(<div>
          <!--
            > <a href="pseudolink_in_a_comment_with_gt_before_lt.htm"
          -->
          <a href="real_link.html"></a>
      </div>)",

      // links
      "{ href, real_link.html }"
    );

    EXPECT_LINKS(
      R"(<script>
          console.log("<a href='pseudolink_inside_a_script_tag1'>");
         </script>
         <script id="whatever">
          console.log("<a href='pseudolink_inside_a_script_tag2'>");
         </script>
         <a href="real_link.html"></a>
      )",

      // links
      "{ href, real_link.html }"
    );

    EXPECT_LINKS(
      R"(<script>
          if ( 1 > 0 ) console.log("unmatched > inside a script tag");
         </script>
         <script id="whatever">
          if ( 1 > 0 ) console.log("unmatched > inside a script tag");
         </script>
         <a href="real_link.html"></a>
      )",

      // links
      "{ href, real_link.html }"
    );

    // Despite HTML not being properly parsed, not every href or src followed
    // by an equality sign (with optional whitespace in between) results in a
    // link
    EXPECT_LINKS(
      "abcd href = qwerty src={123} xyz",
      ""
    );
}
#undef EXPECT_LINKS

TEST(tools, httpRedirectHtml)
{
    EXPECT_EQ(
      httpRedirectHtml("http://example.com"),
      "<!DOCTYPE html>"
      "<html>"
        "<head>"
          "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />"
          "<meta http-equiv=\"refresh\" content=\"0;url=http%3A//example.com\" />"
        "</head>"
        "<body></body>"
      "</html>"
    );

    EXPECT_EQ(
      httpRedirectHtml(u8"A/Κίουι"),
      "<!DOCTYPE html>"
      "<html>"
        "<head>"
          "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />"
          "<meta http-equiv=\"refresh\" content=\"0;url=A/%CE%9A%CE%AF%CE%BF%CF%85%CE%B9\" />"
        "</head>"
        "<body></body>"
      "</html>"
    );
}

TEST(tools, guessFrontArticle)
{
  EXPECT_TRUE(guess_is_front_article("text/html"));
  EXPECT_TRUE(guess_is_front_article("text/html;charset=utf8"));
  EXPECT_FALSE(guess_is_front_article("plain/text"));
  EXPECT_FALSE(guess_is_front_article("some-text/html"));
  EXPECT_FALSE(guess_is_front_article("text/html;raw=true"));
}

TEST(CommonTools, GetFileExtension) {
    EXPECT_EQ(getFileExtension("index.html"), "html");
    EXPECT_EQ(getFileExtension("archive.tar.gz"), "gz");
    EXPECT_EQ(getFileExtension("extensionCaseShouldBePreserved.JS"), "JS");
    EXPECT_EQ(getFileExtension("empty_extension."), "");
    EXPECT_EQ(getFileExtension("no_extension"),     "");
    EXPECT_EQ(getFileExtension("./filename"),       "");
    EXPECT_EQ(getFileExtension("repo.git/README"),  "");
    EXPECT_EQ(getFileExtension(".\\filename"),       "");
    EXPECT_EQ(getFileExtension("repo.git\\README"),  "");
}

TEST(CommonTools, AsciiToLower) {
    EXPECT_EQ(asciitolower(""), "");
    EXPECT_EQ(asciitolower("Hello World!"), "hello world!");
    EXPECT_EQ(asciitolower("AbC123!"), "abc123!");
    EXPECT_EQ(asciitolower("ÄBÇ123!"), "ÄbÇ123!");
    EXPECT_EQ(asciitolower("lower"), "lower");
}

TEST(CommonTools, getTextLength)
{
  // Basic ASCII
  EXPECT_EQ(getTextLength(""), 0u);
  EXPECT_EQ(getTextLength("hello"), 5u);

  // Multi-byte UTF-8 (1 codepoint each)
  EXPECT_EQ(getTextLength("café"), 4u);
  EXPECT_EQ(getTextLength("日本語"), 3u);

  // BUG TEST: Hindi "में" = 1 grapheme, 3 codepoints
  EXPECT_EQ(getTextLength("में"), 1u);

  // ZWJ emoji: 👨‍👩‍👧 = 1 grapheme, 5 codepoints
  EXPECT_EQ(getTextLength("\U0001F468\u200D\U0001F469\u200D\U0001F467"), 1u);

  // Emoji + skin tone: 👋🏽 = 1 grapheme, 2 codepoints
  EXPECT_EQ(getTextLength("\U0001F44B\U0001F3FD"), 1u);

  // Mixed: "café में" = 6 graphemes
  EXPECT_EQ(getTextLength("café में"), 6u);
}
