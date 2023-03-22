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
    67, // this is the lower limit on a syntactically valid PNG file
        // (according to https://github.com/mathiasbynens/small)
    10000, // this is roughly the size of the raw (i.e. without any compression)
           // RGBA pixel data of a 48x48 image
           // Question: how much PNG metadata shall we allow?
    PNG_REGEXP
  },
};
