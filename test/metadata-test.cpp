#include "../src/metadata.h"

#include "gtest/gtest.h"


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
  m.set("Illustration_48x48@1", "\x89PNG\r\n\x1a\n" + std::string(100, 'x'));

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
  m.set("Illustration_48x48@1", "\x89PNG\r\n\x1a\n" + std::string(100, 'x'));

  return m;
}

TEST(Metadata, nonReservedMetadataIsNotAProblem)
{
  zim::Metadata m = makeValidMetadata();
  m.set("NonReservedMetadata", "");
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
        "Date doesn't match regex: \\d\\d\\d\\d-\\d\\d-\\d\\d"
      })
  );
  m.set("Date", "1234-56-78"); // Yes, such a date is considered valid
                               // by the current simple regex
  ASSERT_TRUE(m.valid());

  m.set("Language", "fre,");
  ASSERT_FALSE(m.valid());
  ASSERT_EQ(m.check(),
      zim::Metadata::Errors({
        "Language doesn't match regex: \\w{3}(,\\w{3})*"
      })
  );

  m.set("Language", "fre,nch");
  ASSERT_TRUE(m.valid());
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
