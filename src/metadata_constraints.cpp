const Metadata::ReservedMetadataTable reservedMetadataInfoTable = {
  // name              isMandatory minLength   maxLength   regex
  { "Name",            MANDATORY,  1,          0,          ""                },
  { "Title",           MANDATORY,  1,          30,         ""                },
  { "Language",        MANDATORY,  3,          0,          LANGS_REGEXP      },
  { "Creator",         MANDATORY,  1,          0,          ""                },
  { "Publisher",       MANDATORY,  1,          0,          ""                },
  { "Date",            MANDATORY,  10,         10,         DATE_REGEXP       },
  { "Description",     MANDATORY,  1,          80,         ""                },
  { "LongDescription", OPTIONAL,   1,          4000,       ""                },
  { "License",         OPTIONAL,   1,          0,          ""                },
  { "Tags",            OPTIONAL,   1,          0,          ""                },
  { "Relation",        OPTIONAL,   1,          0,          ""                },
  { "Flavour",         OPTIONAL,   1,          0,          ""                },
  { "Source",          OPTIONAL,   1,          0,          ""                },
  { "Counter",         OPTIONAL,   1,          0,          ""                },
  { "Scraper",         OPTIONAL,   1,          0,          ""                },

  {
    "Illustration_48x48@1",
    MANDATORY,
    0, // There are no constraints on the illustration metadata size
    0, // in order to avoid decoding it as UTF-8 encoded text
    PNG_REGEXP
  },
};
