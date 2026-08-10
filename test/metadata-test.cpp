#include "../src/metadata.h"

#include "gtest/gtest.h"

std::string fakePNG()
{
  return std::string{"\x89PNG\r\n\x1a\n"} + std::string(100, 'x');
}

#define ASSERT_VALID_COUNTER(VAL) \
  do { \
    zim::Metadata m = makeValidMetadata(); \
    m.set("Counter", VAL); \
    ASSERT_TRUE(m.valid()) << "Expected valid counter: " << VAL; \
  } while (0)

#define ASSERT_INVALID_COUNTER(VAL) \
  do { \
    zim::Metadata m = makeValidMetadata(); \
    m.set("Counter", VAL); \
    ASSERT_FALSE(m.valid()) << "Expected invalid counter: " << VAL; \
  } while (0)

TEST(Metadata, isDefaultConstructible)
{
  zim::Metadata m;
  (void)m; // suppress compiler's warning about an unused variable
}


TEST(Metadata, detectsAbsenceOfMandatoryEntries)
{
  zim::Metadata m;

  ASSERT_FALSE(m.valid());
  ASSERT_EQ(m.check(),
      zim::Metadata::Errors({
        "Missing mandatory metadata: Name",
        "Missing mandatory metadata: Title",
        "Missing mandatory metadata: Language",
        "Missing mandatory metadata: Creator",
        "Missing mandatory metadata: Publisher",
        "Missing mandatory metadata: Date",
        "Missing mandatory metadata: Description",
        "Missing mandatory metadata: Illustration_48x48@1",
      })
  );

  m.set("Description", "Any nonsense is better than nothing");
  m.set("Date", "2020-20-20");
  m.set("Creator", "Demiurge");
  m.set("Name", "wikipedia_py_all");

  ASSERT_FALSE(m.valid());
  ASSERT_EQ(m.check(),
      zim::Metadata::Errors({
        "Missing mandatory metadata: Title",
        "Missing mandatory metadata: Language",
        "Missing mandatory metadata: Publisher",
        "Missing mandatory metadata: Illustration_48x48@1",
      })
  );

  m.set("Title", "Chief Executive Officer");
  m.set("Publisher", "Zangak");
  m.set("Language", "py3");
  m.set("Illustration_48x48@1", fakePNG(), "image/png");

  ASSERT_TRUE(m.valid());
  ASSERT_TRUE(m.check().empty());
}

zim::Metadata makeValidMetadata()
{
  zim::Metadata m;

  m.set("Description", "Any nonsense is better than nothing");
  m.set("Date", "2020-20-20");
  m.set("Creator", "Demiurge");
  m.set("Name", "wikipedia_py_all");
  m.set("Title", "Chief Executive Officer");
  m.set("Publisher", "Zangak");
  m.set("Language", "py3");
  m.set("Illustration_48x48@1", fakePNG(), "image/png");

  return m;
}

TEST(Metadata, nonReservedMetadataIsNotAProblem)
{
  zim::Metadata m = makeValidMetadata();
  m.set("NonReservedMetadata", "", "application/javascript");
  ASSERT_TRUE(m.valid());
}

TEST(Metadata, minSizeConstraints)
{
  zim::Metadata m = makeValidMetadata();
  m.set("Title", "");
  ASSERT_FALSE(m.valid());
  ASSERT_EQ(m.check(),
      zim::Metadata::Errors({
        "Title must contain at least 1 characters"
      })
  );
  m.set("Title", "t");
  ASSERT_TRUE(m.valid());
}

TEST(Metadata, maxSizeConstraints)
{
  zim::Metadata m = makeValidMetadata();
  m.set("Title", std::string(31, 'a'));
  ASSERT_FALSE(m.valid());
  ASSERT_EQ(m.check(),
      zim::Metadata::Errors({
        "Title must contain at most 30 characters"
      })
  );
  m.set("Title", std::string(30, 'a'));
  ASSERT_TRUE(m.valid());
}

TEST(Metadata, regexpConstraints)
{
  zim::Metadata m = makeValidMetadata();
  m.set("Date", "YYYY-MM-DD");
  ASSERT_FALSE(m.valid());
  ASSERT_EQ(m.check(),
      zim::Metadata::Errors({
        "Date doesn't match regex: ^\\d\\d\\d\\d-\\d\\d-\\d\\d$"
      })
  );

  m.set("Date", "1234-56-78"); // Yes, such a date is considered valid
                               // by the current simple regex
  ASSERT_TRUE(m.valid());

  m.set("Language", "fre,");
  ASSERT_FALSE(m.valid());
  ASSERT_EQ(m.check(),
      zim::Metadata::Errors({
        "Language doesn't match regex: ^\\w{3}(,\\w{3})*$"
      })
  );

  m.set("Language", "fre,nch");
  ASSERT_TRUE(m.valid());

  m.set("Illustration_48x48@1", "zimdata/favicon.png", "image/png");
  ASSERT_EQ(m.check(),
      zim::Metadata::Errors({
        "Illustration_48x48@1 doesn't match regex: ^\\x89PNG\\x0d\\x0a\\x1a\\x0a"
      })
  );
}

TEST(Metadata, mimeTypeChecking)
{
  zim::Metadata m = makeValidMetadata();
  m.set("License", "Driver's", "text/plain;charset=ASCII");
  ASSERT_EQ(m.check(),
      zim::Metadata::Errors({
        "License has wrong MIME type: text/plain;charset=ASCII"
      })
  );

  m.set("License", "Driver's", "image/png");
  ASSERT_EQ(m.check(),
      zim::Metadata::Errors({
        "License has wrong MIME type: image/png"
      })
  );

  m.set("License", "Driver's", "text/plain");
  ASSERT_TRUE(m.valid());

  m.set("License", "Driver's", "TEXT/PLAIN");
  ASSERT_TRUE(m.valid());

  m.set("License", "Driver's", "text/plain;charset=UTF-8");
  ASSERT_TRUE(m.valid());

  m.set("License", "Driver's", "text/plain;charset=utf-8");
  ASSERT_TRUE(m.valid());

  m.set("Illustration_48x48@1", fakePNG(), "text/plain");
  ASSERT_EQ(m.check(),
      zim::Metadata::Errors({
        "Illustration_48x48@1 has wrong MIME type: text/plain"
      })
  );
}

TEST(Metadata, pngRegexp)
{
  const std::string PNG_HEADER = "\x89PNG\r\n\x1a\n";
  zim::Metadata m = makeValidMetadata();
  {
    m.set("Illustration_48x48@1", PNG_HEADER + 'A', "image/png");
    ASSERT_TRUE(m.valid());
  }
  {
    m.set("Illustration_48x48@1", PNG_HEADER + '\n', "image/png");
    ASSERT_TRUE(m.valid());
  }
  {
    m.set("Illustration_48x48@1", PNG_HEADER + '\0', "image/png");
    ASSERT_TRUE(m.valid());
  }
}


TEST(Metadata, complexConstraints)
{
  zim::Metadata m = makeValidMetadata();
  m.set("Description",     "Short description");
  m.set("LongDescription", "Long description");
  ASSERT_FALSE(m.valid());
  ASSERT_EQ(m.check(),
      zim::Metadata::Errors({
        "LongDescription shouldn't be shorter than Description"
      })
  );
}

TEST(Metadata, mandatoryMetadataAndSimpleChecksAreRunUnconditionally)
{
  zim::Metadata m;

  m.set("Description", "Blablabla");
  m.set("Date", "2020-20-20");
  m.set("Creator", "Demiurge");
  m.set("Name", "wikipedia_js_maxi");
  m.set("Title", "A title that is too long to read for a five year old");
  m.set("Publisher", "Zangak");
  m.set("Language", "js");
  //m.set("Illustration_48x48@1", "");

  ASSERT_FALSE(m.valid());
  ASSERT_EQ(m.check(),
      zim::Metadata::Errors({
        "Missing mandatory metadata: Illustration_48x48@1",
        "Language must contain at least 3 characters",
        "Language doesn't match regex: ^\\w{3}(,\\w{3})*$",
        "Title must contain at most 30 characters"
      })
  );
}

TEST(Metadata, complexChecksAreRunOnlyIfMandatoryMetadataRequirementsAreMet)
{
  zim::Metadata m;

  m.set("Description",     "Blablabla");
  m.set("LongDescription", "Blabla");
  m.set("Date", "2020-20-20");
  m.set("Creator", "TED");
  m.set("Name", "TED_bodylanguage");
  //m.set("Title", "");
  m.set("Publisher", "Kiwix");
  m.set("Language", "bod,yla,ngu,age");
  m.set("Illustration_48x48@1", fakePNG(), "image/png");

  ASSERT_FALSE(m.valid());
  ASSERT_EQ(m.check(),
      zim::Metadata::Errors({
        "Missing mandatory metadata: Title",
      })
  );

  m.set("Title", "Blabluba");

  ASSERT_FALSE(m.valid());
  ASSERT_EQ(m.check(),
      zim::Metadata::Errors({
        "LongDescription shouldn't be shorter than Description"
      })
  );
}

TEST(Metadata, counterRegexp){
  // VALID COUNTER CHECKS

  // Single entry, no trailing semicolon (Valid)
  ASSERT_VALID_COUNTER("text/html=5");

  // Single entry, with trailing semicolon (Valid)
  ASSERT_VALID_COUNTER("text/html=5;");

  // Multiple entries, strictly delimited, no spaces (Valid)
  ASSERT_VALID_COUNTER("text/html=5;image/png=10;application/javascript=2");

  // Multiple entries, strictly delimited, trailing semicolon (Valid)
  ASSERT_VALID_COUNTER("text/html=5;image/png=10;");

  // INVALID COUNTER CHECKS

  // Missing semicolon between entries = invalid
  ASSERT_INVALID_COUNTER("text/html=5image/png=10");

  // Contains a space = invalid
  ASSERT_INVALID_COUNTER("text/html=5; image/png=10;");

  // Value is not a number = invalid
  ASSERT_INVALID_COUNTER("text/html=X;");

  // Invalid MIME type (missing the slash) = invalid
  ASSERT_INVALID_COUNTER("text_html=5;");

  // Missing the number entirely = invalid
  ASSERT_INVALID_COUNTER("text/html=;");
  // additional invalid cases, this time straight from #1000

  // the exact broken string from the issue
  // this MUST fail because it contains spaces, quotes, and singular semicolons
  ASSERT_INVALID_COUNTER("application/javascript=4;application/pdf=3;image/apng=1;image/gif=5166;image/jpeg=280;image/png=124;image/svg+xml=8;image/svg+xml; charset=utf-8; profile=\"https://www.mediawiki.org/wiki/Specs/SVG/1.0.0\"=67381;image/webp=524540;text/css=28;text/html=50000;text/html; charset=iso-8859-1=1;text/javascript=3");

  // simplified test: charset parameter leakage
  // Fails because of the space and the semicolon inside the MIME type
  ASSERT_INVALID_COUNTER("text/html; charset=utf-8=10;");

  // fails because 'charset=utf-8;' doesn't have a valid MIME type format or a '=' count
  ASSERT_INVALID_COUNTER("image/svg+xml=8; charset=utf-8;");

  // simplified test: profile URL leakage
  // fails because quotes and colons are not allowed in the MIME type section
  ASSERT_INVALID_COUNTER("image/svg+xml; profile=\"https://www.mediawiki.org...\"=67381;");

  // the Corrected String ( ideally generated case )
  // strip parameters and sum the duplicates:
  // text/html (50000 + 1 = 50001)
  // image/svg+xml (8 + 67381 = 673889)
  ASSERT_VALID_COUNTER("application/javascript=4;application/pdf=3;image/apng=1;image/gif=5166;image/jpeg=280;image/png=124;image/svg+xml=673889;image/webp=524540;text/css=28;text/html=50001;text/javascript=3");

  // couple more edge cases
  ASSERT_INVALID_COUNTER("4/2=3");
  ASSERT_INVALID_COUNTER("1.00/0.25=9");
  ASSERT_INVALID_COUNTER("+/-=000");
  ASSERT_INVALID_COUNTER("./..=5");

  // Realistic cases: Valid real-world MIME types using [a-zA-Z0-9.\-+]

  // Contains '+'
  ASSERT_VALID_COUNTER("image/svg+xml=8");
  ASSERT_VALID_COUNTER("application/ld+json=10");

  // Contains '-'
  ASSERT_VALID_COUNTER("application/x-www-form-urlencoded=42");
  ASSERT_VALID_COUNTER("text/x-c++src=3"); // Contains both '-' and '+'

  // Contains '.'
  ASSERT_VALID_COUNTER("application/vnd.ms-excel=1");
  ASSERT_VALID_COUNTER("application/vnd.apple.mpegurl=5");

  // Contains Digits (0-9)
  ASSERT_VALID_COUNTER("audio/mp3=100");
  ASSERT_VALID_COUNTER("video/h264=2");

  // Contains Uppercase (A-Z)
  ASSERT_VALID_COUNTER("image/JPEG=5");
  ASSERT_VALID_COUNTER("application/PDF=10");

  // cases that pass validation but unlikely to be real MIME types
  // These test the full extent of the [a-zA-Z0-9.\-+] character class boundaries

  // single special characters
  ASSERT_VALID_COUNTER("a/+++=1");
  ASSERT_VALID_COUNTER("a/...=1");
  ASSERT_VALID_COUNTER("a/---=1");

  // numbers or uppercase only
  ASSERT_VALID_COUNTER("a/0123456789=1");
  ASSERT_VALID_COUNTER("a/ABCDEFGHIJKLMNOPQRSTUVWXYZ=1");

  // Mixed special chars only
  ASSERT_VALID_COUNTER("a/.+-=1");
  ASSERT_VALID_COUNTER("a/0.0-0+0=42");

  // Extreme combinations
  ASSERT_VALID_COUNTER("x/Z+9.8-7.6+5-4.3+2-1.0=1");

  // Single character subtypes using each class
  ASSERT_VALID_COUNTER("a/A=1");
  ASSERT_VALID_COUNTER("a/0=1");
  ASSERT_VALID_COUNTER("a/.=1");
  ASSERT_VALID_COUNTER("a/-=1");
  ASSERT_VALID_COUNTER("a/+=1");
}
