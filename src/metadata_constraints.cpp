const Metadata::ReservedMetadataTable reservedMetadataInfoTable = {
  // name              isMandatory  length      regex         mime-type
  //                               min   max
  { "Name",            MANDATORY,  1,    0,     "",             TEXT_PLAIN },
  { "Title",           MANDATORY,  1,    30,    "",             TEXT_PLAIN },
  { "Language",        MANDATORY,  3,    0,     LANGS_REGEXP,   TEXT_PLAIN },
  { "Creator",         MANDATORY,  1,    0,     "",             TEXT_PLAIN },
  { "Publisher",       MANDATORY,  1,    0,     "",             TEXT_PLAIN },
  { "Date",            MANDATORY,  10,   10,    DATE_REGEXP,    TEXT_PLAIN },
  { "Description",     MANDATORY,  1,    80,    "",             TEXT_PLAIN },
  { "LongDescription", OPTIONAL,   0,    4000,  "",             TEXT_PLAIN },
  { "License",         OPTIONAL,   0,    0,     "",             TEXT_PLAIN },
  { "Tags",            OPTIONAL,   0,    0,     "",             TEXT_PLAIN },
  { "Relation",        OPTIONAL,   0,    0,     "",             TEXT_PLAIN },
  { "Flavour",         OPTIONAL,   0,    0,     "",             TEXT_PLAIN },
  { "Source",          OPTIONAL,   0,    0,     "",             TEXT_PLAIN },
  { "Counter",         OPTIONAL,   0,    0,     COUNTER_REGEXP, TEXT_PLAIN },
  { "Scraper",         OPTIONAL,   0,    0,     "",             TEXT_PLAIN },

  {
    "Illustration_48x48@1",
    MANDATORY,
    0, // There are no constraints on the illustration metadata size
    0, // in order to avoid decoding it as UTF-8 encoded text
    PNG_REGEXP,
    Metadata::MimeType::PNG
  },
};

METADATA_ASSERT("LongDescription shouldn't be shorter than Description")
{
  return !data.has("LongDescription") ||
    data["LongDescription"].value.size() == 0 ||
    data["LongDescription"].value.size() >= data["Description"].value.size();
}
