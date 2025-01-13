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

    EXPECT_EQ(UriKind::OTHER, uriKind("/"));
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
    ASSERT_EQ(normalize_link("", ""), "");
    ASSERT_EQ(normalize_link("/", ""), "");
    ASSERT_EQ(normalize_link("", "/"), "/");

    ASSERT_EQ(normalize_link("/a", "/b"), "a");

    // not absolute
    ASSERT_EQ(normalize_link("a", "/b"), "/b/a");
    ASSERT_EQ(normalize_link("../a", "/b/c"), "/b/a");
    ASSERT_EQ(normalize_link(".././a", "/b/c"), "/b/a");
    ASSERT_EQ(normalize_link("../a/b/aa#localanchor", "/b/c"), "/b/a/b/aa");
    ASSERT_EQ(normalize_link("../a/b/aa?localanchor", "/b/c"), "/b/a/b/aa");

    ASSERT_EQ(normalize_link("a", ""), "a");
    ASSERT_EQ(normalize_link("./a", ""), "a");

    // URI-decoding is performed
    ASSERT_EQ(normalize_link("/%41%62c", "/"), "Abc");

    // #439: normalized link reading off end of buffer
    // small-string-opt sizes, so sanitizers and valgrind don't pick this up
    EXPECT_EQ(normalize_link("%", "/"), "/");
    EXPECT_EQ(normalize_link("%1", ""), "");

    // ../test/tools-test.cpp:260: Failure
    // Expected equality of these values:
    //   normalize_link("%", "/")
    //     Which is: "/\01bc"
    //   "/"
    //
    // ../test/tools-test.cpp:261: Failure
    // Expected equality of these values:
    //   normalize_link("%1", "")
    //     Which is: "\x1" "1bc"
    //   ""

    // test outside of small-string-opt
    // valgrind will pick up on the error in this one
    EXPECT_EQ(normalize_link("qrstuvwxyz%", "/abcdefghijklmnop"), "/abcdefghijklmnop/qrstuvwxyz");
    EXPECT_EQ(normalize_link("qrstuvwxyz%1", "/abcdefghijklmnop"), "/abcdefghijklmnop/qrstuvwxyz");
}

TEST(tools, addler32)
{
    ASSERT_EQ(adler32("sdfkhewruhwe8"), 640746832);
    ASSERT_EQ(adler32("sdifjsdf"), 251593550);
    ASSERT_EQ(adler32("q"), 7471218);
    ASSERT_EQ(adler32(""), 1);
}

TEST(tools, decodeHtmlEntities)
{
    ASSERT_EQ(decodeHtmlEntities(""),   "");

    // Supported HTML character references
    ASSERT_EQ(decodeHtmlEntities("&amp;"),  "&");
    ASSERT_EQ(decodeHtmlEntities("&apos;"), "'");
    ASSERT_EQ(decodeHtmlEntities("&quot;"), "\"");
    ASSERT_EQ(decodeHtmlEntities("&lt;"),   "<");
    ASSERT_EQ(decodeHtmlEntities("&gt;"),   ">");

    // All other HTML character references
    // (https://html.spec.whatwg.org/multipage/syntax.html#character-references)
    // are NOT currently supported
    ASSERT_EQ(decodeHtmlEntities("&nbsp;"), "&nbsp;");

    // Capitalized versions of supported ones do NOT work
    ASSERT_EQ(decodeHtmlEntities("&AMP;"), "&AMP;");
    ASSERT_EQ(decodeHtmlEntities("&aMP;"), "&aMP;");

    // HTML entities of the form &#dd...; and/or &#xhh...; are NOT decoded
    ASSERT_EQ(decodeHtmlEntities("&#65;"),  "&#65;" ); // should be "A"
    ASSERT_EQ(decodeHtmlEntities("&#x41;"), "&#x41;"); // should be "A"

    // Handling of "incomplete" entity
    ASSERT_EQ(decodeHtmlEntities("&amp"), "&amp");

    // No double decoding
    ASSERT_EQ(decodeHtmlEntities("&amp;lt;"), "&lt;");

    ASSERT_EQ(decodeHtmlEntities("&lt;&gt;"), "<>");

    ASSERT_EQ(decodeHtmlEntities("1&lt;2"),   "1<2");

    ASSERT_EQ(decodeHtmlEntities("3&5&gt;3/5"), "3&5>3/5");

    ASSERT_EQ(
        decodeHtmlEntities("Q&amp;A stands for &quot;Questions and answers&quot;"),
        "Q&A stands for \"Questions and answers\""
    );
}

std::string links2Str(const std::vector<html_link>& links)
{
    std::ostringstream oss;
    const char* sep = "";
    for ( const auto& l : links ) {
        oss << sep << "{ " << l.attribute << ", " << l.link << " }";
        sep = "\n";
    }
    return oss.str();
}

#define EXPECT_LINKS(html, expectedStr) \
        ASSERT_EQ(links2Str(generic_getLinks(html)), expectedStr)

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
    // (that's normalize_link()'s job)
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

    // Known issue - HTML is not parsed and therefore false links
    //               may be returned
    EXPECT_LINKS(
      R"(
<html>
  <head>
    <link src = "/css/stylesheet.css" rel="stylesheet">
    <link rel="icon" href   =    '/favicon.ico'>
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
  </body>
</html>
)",
      // links
      "{ src, /css/stylesheet.css }"                      "\n"
      "{ href, /favicon.ico }"                            "\n"
      "{ src, ../img/welcome.png }"                       "\n"
      "{ href, commented_out_link.htm }"                  "\n"
      "{ src, commented_out_image.png }"                  "\n"
      "{ href, not_a_link_in_example_code_block.htm }"    "\n"
      "{ src, not_a_link_in_example_code_block.png }"     "\n"
      "{ href, https://kiwix.org }"
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
  ASSERT_TRUE(guess_is_front_article("text/html"));
  ASSERT_TRUE(guess_is_front_article("text/html;charset=utf8"));
  ASSERT_FALSE(guess_is_front_article("plain/text"));
  ASSERT_FALSE(guess_is_front_article("some-text/html"));
  ASSERT_FALSE(guess_is_front_article("text/html;raw=true"));
}
