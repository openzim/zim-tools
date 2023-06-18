const Metadata::ReservedMetadataTable reservedMetadataInfoTable = {
  // name              isMandatory minLength   maxLength   regex
  { "Name",            MANDATORY,  1,          0,          ""                },
  { "Title",           MANDATORY,  1,          30,         ""                },
  { "Language",        MANDATORY,  3,          0,          LANGS_REGEXP      },
  { "Creator",         MANDATORY,  1,          0,          ""                },
  { "Publisher",       MANDATORY,  1,          0,          ""                },
  { "Date",            MANDATORY,  10,         10,         DATE_REGEXP       },
  { "Description",     MANDATORY,  1,          80,         ""                },
  { "LongDescription", OPTIONAL,   0,          4000,       ""                },
  { "License",         OPTIONAL,   0,          0,          ""                },
  { "Tags",            OPTIONAL,   0,          0,          ""                },
  { "Relation",        OPTIONAL,   0,          0,          ""                },
  { "Flavour",         OPTIONAL,   0,          0,          ""                },
  { "Source",          OPTIONAL,   0,          0,          ""                },
  { "Counter",         OPTIONAL,   0,          0,          ""                },
  { "Scraper",         OPTIONAL,   0,          0,          ""                },

  {
    "Illustration_48x48@1",
    MANDATORY,
    0, // There are no constraints on the illustration metadata size
    0, // in order to avoid decoding it as UTF-8 encoded text
    ""
  },
};

METADATA_ASSERT("LongDescription shouldn't be shorter than Description")
{
  return !data.has("LongDescription")
      || data["LongDescription"].size() >= data["Description"].size();
}
