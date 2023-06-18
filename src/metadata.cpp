/*
 * Copyright 2023 Veloman Yunkan <veloman.yunkan@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU  General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include "metadata.h"

#include <sstream>
#include <regex>
#include <unicode/unistr.h>

#include <cctype>
#include <iomanip>


namespace zim
{

namespace
{

const bool MANDATORY = true;
const bool OPTIONAL  = false;

const std::string LANGS_REGEXP = "\\w{3}(,\\w{3})*";
const std::string DATE_REGEXP = R"(\d\d\d\d-\d\d-\d\d)";

bool matchRegex(const std::string& regexStr, const std::string& text)
{
  const std::regex regex(regexStr);
  return std::regex_match(text.begin(), text.end(), regex);
}

size_t getTextLength(const std::string& utf8EncodedString)
{
  return icu::UnicodeString::fromUTF8(utf8EncodedString).length();
}

class MetadataComplexCheckBase
{
public:
  const std::string description;
  const MetadataComplexCheckBase* const prev;

public: // functions
  explicit MetadataComplexCheckBase(const std::string& desc);

  MetadataComplexCheckBase(const MetadataComplexCheckBase&) = delete;
  MetadataComplexCheckBase(MetadataComplexCheckBase&&) = delete;
  void operator=(const MetadataComplexCheckBase&) = delete;
  void operator=(MetadataComplexCheckBase&&) = delete;

  virtual ~MetadataComplexCheckBase();

  virtual bool checkMetadata(const Metadata& m) const = 0;

  static const MetadataComplexCheckBase* getLastCheck() { return last; }

private: // functions
  static const MetadataComplexCheckBase* last;
};

const MetadataComplexCheckBase* MetadataComplexCheckBase::last = nullptr;

MetadataComplexCheckBase::MetadataComplexCheckBase(const std::string& desc)
  : description(desc)
  , prev(last)
{
  last = this;
}

MetadataComplexCheckBase::~MetadataComplexCheckBase()
{
  // Ideally, we should de-register this object from the list of live objects.
  // However, in the current implementation MetadataComplexCheckBase objects
  // are only constructed in static storage and the list of active objects
  // isn't supposed to be accessed after any MetadataComplexCheckBase object
  // has been destroyed as part of program termination clean-up actions.
}

#define ADD_METADATA_COMPLEX_CHECK(DESC, CLSNAME)              \
class CLSNAME : public MetadataComplexCheckBase                \
{                                                              \
public:                                                        \
  CLSNAME() : MetadataComplexCheckBase(DESC) {}                \
  bool checkMetadata(const Metadata& data) const override;     \
};                                                             \
                                                               \
const CLSNAME CONCAT(obj, CLSNAME);                            \
                                                               \
bool CLSNAME::checkMetadata(const Metadata& data) const        \
/* should be followed by the check body */



#define CONCAT(X, Y) X##Y
#define GENCLSNAME(UUID) CONCAT(MetadataComplexCheck, UUID)

#define METADATA_ASSERT(DESC) ADD_METADATA_COMPLEX_CHECK(DESC, GENCLSNAME(__LINE__))


#include "metadata_constraints.cpp"

// This function is intended for pretty printing of regexps with non-printable
// characters.
// In a general purpose/rigorous version we should escape the escape symbol
// (backslash) too, but that doesn't play well with the purpose stated above.
std::string escapeNonPrintableChars(const std::string& s)
{
  std::ostringstream os;
  os << std::hex;
  for (const char c : s) {
    if (std::isprint(c)) {
      os << c;
    } else {
      const unsigned int charVal = static_cast<unsigned char>(c);
      os << "\\x" << std::setw(2) << std::setfill('0') << charVal;
    }
  }
  return os.str();
}

Metadata::Errors concat(Metadata::Errors e1, const Metadata::Errors& e2)
{
  e1.insert(e1.end(), e2.begin(), e2.end());
  return e1;
}

} // unnamed namespace

const Metadata::ReservedMetadataTable& Metadata::reservedMetadataInfo = reservedMetadataInfoTable;

const Metadata::ReservedMetadataRecord&
Metadata::getReservedMetadataRecord(const std::string& name)
{
  for ( const auto& x : reservedMetadataInfo ) {
    if ( x.name == name )
      return x;
  }

  throw std::out_of_range(name + " is not a reserved metadata name");
}

bool Metadata::has(const std::string& name) const
{
  return data.find(name) != data.end();
}

const std::string& Metadata::operator[](const std::string& name) const
{
  return data.at(name);
}

void Metadata::set(const std::string& name, const std::string& value)
{
  data[name] = value;
}

bool Metadata::valid() const
{
  return check().empty();
}

Metadata::Errors Metadata::checkMandatoryMetadata() const
{
  Errors errors;
  for ( const auto& rmr : reservedMetadataInfo ) {
    if ( rmr.isMandatory && data.find(rmr.name) == data.end() ) {
      errors.push_back("Missing mandatory metadata: " + rmr.name );
    }
  }

  return errors;
}

Metadata::Errors Metadata::checkSimpleConstraints() const
{
  Errors errors;
  for ( const auto& nv : data ) {
    const auto& name = nv.first;
    const auto& value = nv.second;
    try {
      const auto& rmr = getReservedMetadataRecord(name);
      if ( rmr.minLength != 0 && getTextLength(value) < rmr.minLength ) {
        std::ostringstream oss;
        oss << name << " must contain at least " << rmr.minLength << " characters";
        errors.push_back(oss.str());
      }
      if ( rmr.maxLength != 0 && getTextLength(value) > rmr.maxLength ) {
        std::ostringstream oss;
        oss << name << " must contain at most " << rmr.maxLength << " characters";
        errors.push_back(oss.str());
      }
      if ( !rmr.regex.empty() && !matchRegex(rmr.regex, value) ) {
        const std::string regex = escapeNonPrintableChars(rmr.regex);
        errors.push_back(name + " doesn't match regex: " + regex);
      }
    } catch ( const std::out_of_range& ) {
      // ignore non-reserved metadata
    }
  }
  return errors;
}

Metadata::Errors Metadata::checkComplexConstraints() const
{
  Errors errors;
  const MetadataComplexCheckBase* c = MetadataComplexCheckBase::getLastCheck();
  for ( ; c != nullptr ; c = c->prev ) {
    if ( ! c->checkMetadata(*this) ) {
      errors.push_back(c->description);
    }
  }
  return errors;
}

Metadata::Errors Metadata::check() const
{
  const Errors e1 = checkMandatoryMetadata();
  const Errors e2 = checkSimpleConstraints();
  if ( !e1.empty() || !e2.empty() )
    return concat(e1, e2);

  return checkComplexConstraints();
}

} // namespace zim
