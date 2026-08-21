/*
 * Copyright 2013-2016 Emmanuel Engelhart <kelson@kiwix.org>
 * Copyright 2016 Matthieu Gautier <mgautier@kymeria.fr>
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

#include "tools.h"

#include <string.h>
#include <sys/stat.h>
#include <cerrno>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <memory>
#include <algorithm>
#include <regex>
#include <array>
#include <unicode/brkiter.h>
#include <unicode/utypes.h>
#include <unicode/unistr.h>

#include <filesystem>


std::string asciitolower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return ('A' <= c && c <= 'Z') ? c - ('Z' - 'z') : c;
        });
    return s;
}

bool fileExists(const std::string& path)
{
  std::error_code ec;
  return std::filesystem::is_regular_file(std::filesystem::u8path(path), ec);
}

bool isDirectory(const std::string &path)
{
  std::error_code ec;
  return std::filesystem::is_directory(std::filesystem::u8path(path), ec);
}

std::string getFileExtension(std::string_view path) {
    const auto posOfLastDot = path.find_last_of(".");
    if (posOfLastDot == std::string_view::npos) {
        return "";
    }
    const auto partAfterLastDot = path.substr(posOfLastDot + 1);
    return partAfterLastDot.find_first_of("/\\") == std::string_view::npos
         ? std::string(partAfterLastDot)
         : "";
}

/* base64 */
static const std::string base64_chars
    = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz"
      "0123456789+/";

std::string base64_encode(unsigned char const* bytes_to_encode,
                          unsigned int in_len)
{
  std::string ret;
  int i = 0;
  int j = 0;
  unsigned char char_array_3[3];
  unsigned char char_array_4[4];

  while (in_len--) {
    char_array_3[i++] = *(bytes_to_encode++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1]
          = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2]
          = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for (i = 0; (i < 4); i++)
        ret += base64_chars[char_array_4[i]];
      i = 0;
    }
  }

  if (i) {
    for (j = i; j < 3; j++)
      char_array_3[j] = '\0';

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1]
        = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2]
        = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;

    for (j = 0; (j < i + 1); j++)
      ret += base64_chars[char_array_4[j]];

    while ((i++ < 3))
      ret += '=';
  }

  return ret;
}

static char charFromHex(std::string a)
{
  std::istringstream Blat(a);
  int Z;
  Blat >> std::hex >> Z;
  return char(Z);
}

std::string decodeUrl(const std::string& originalUrl)
{
  std::string url = originalUrl;
  std::string::size_type pos = 0;
  while ((pos = url.find('%', pos)) != std::string::npos
         && pos + 2 < url.length()) {
    if (!isxdigit(url[pos+1]) || !isxdigit(url[pos+2])) {
      ++pos;
      continue;
    }
    url.replace(pos, 3, 1, charFromHex(url.substr(pos + 1, 2)));
    ++pos;
  }
  return url;
}

static std::string removeLastPathElement(const std::string& path,
                                  const bool removePreSeparator,
                                  const bool removePostSeparator)
{
  std::string newPath = path;
  size_t offset = newPath.find_last_of('/');

  if (removePreSeparator && offset == newPath.length() - 1) {
    newPath = newPath.substr(0, offset);
    offset = newPath.find_last_of('/');
  }
  newPath = removePostSeparator ? newPath.substr(0, offset)
                                : newPath.substr(0, offset + 1);

  return newPath;
}

namespace
{

std::vector<std::string> split(const std::string & str, char delim)
{
  std::vector<std::string> result;
  auto start = str.begin();
  while ( true ) {
      const auto end = find(start, str.end(), delim);

      result.push_back(std::string(start, end));
      if ( end == str.end() )
        break;
      start = end + 1;
  }

  return result;
}

std::string
getRelativePath(const std::string& basePath, const std::string& targetPath)
{
    const auto b = split(basePath, '/');
    const auto t = split(targetPath, '/');
    const auto l = std::min(b.size()-1, t.size());

    auto x = mismatch(b.begin(), b.begin() + l, t.begin());

    const size_t ups = (b.end() - x.first) + (x.second == t.end());
    std::string r;
    for ( size_t i = 1; i < ups; ++i ) {
      if ( !r.empty() && r.back() != '/' )
        r += "/";
      r += "..";
    }

    for ( auto it = x.second; it != t.end(); ++it ) {
        if ( !r.empty() && r.back() != '/' )
          r += "/";
        r += *it;
    }
    return r;
}

} // unnamed namespace

std::string
computeRelativePath(const std::string& basePath, const std::string& targetPath)
{
  if ( targetPath.back() == '/' ) {
    if ( basePath == targetPath )
      return "./";

    const std::string strippedPath = targetPath.substr(0, targetPath.size()-1);
    return getRelativePath(basePath, strippedPath) + '/';
  }
  return getRelativePath(basePath, targetPath);
}

/* Warning: the relative path must be with slashes */
std::string computeAbsolutePath(const std::string& path,
                                const std::string& relativePath)
{
  /* Remove leaf part of the path if not already a directory */
  std::string absolutePath = path.length()
                               ? (path[path.length() - 1] == '/'
                                  ? path
                                  : removeLastPathElement(path, false, false))
                               : path;

  /* Go through relative path */
  std::vector<std::string> relativePathElements;
  std::stringstream relativePathStream(relativePath);
  std::string relativePathItem;
  while (std::getline(relativePathStream, relativePathItem, '/')) {
    if (relativePathItem == "..") {
      absolutePath = removeLastPathElement(absolutePath, true, false);
    } else if (!relativePathItem.empty() && relativePathItem != ".") {
      absolutePath += relativePathItem;
      absolutePath += "/";
    }
  }

  /* Remove wront trailing / */
  return absolutePath.substr(0, absolutePath.length() - 1);
}


void replaceStringInPlaceOnce(std::string& subject,
                              std::string_view search,
                              std::string_view replace)
{
  size_t pos = subject.find(search, 0);
  if (pos != std::string::npos) {
    subject.replace(pos, search.length(), replace);
  }
}

void replaceStringInPlace(std::string& subject,
                          std::string_view search,
                          std::string_view replace)
{
  if (search.empty())
    return;

  size_t pos = 0;
  while ((pos = subject.find(search, pos)) != std::string::npos) {
    subject.replace(pos, search.length(), replace);
    pos += replace.length();
  }

  return;
}

void stripTitleInvalidChars(std::string& str)
{
  /* Remove unicode orientation invisible characters */
  replaceStringInPlace(str, "\u202A", "");
  replaceStringInPlace(str, "\u202C", "");
}

namespace
{

const char* getHtmlEntity(const std::string& core)
{
  static const std::map<std::string, const char*> t = {
    { "amp",  "&"  },
    { "apos", "'"  },
    { "quot", "\"" },
    { "lt",   "<"  },
    { "gt",   ">"  },
  };

  const auto it = t.find(core);
  return it != t.end() ? it->second : nullptr;
}

} // unnamed namespace

std::string decodeHtmlEntities(const std::string& str)
{
  const char* p = str.c_str();
  std::string result;
  const char* start = nullptr;
  for ( ; *p ; ++p ) {
    if ( *p == '&' ) {
      if ( start ) {
        result.insert(result.end(), start, p);
      }
      start = p;
    } else if ( !start ) {
      result.push_back(*p);
    } else if ( *p == ';' ) {
      const char* d = getHtmlEntity(std::string(start+1, p));
      if ( d ) {
        result += d;
      } else {
        result.insert(result.end(), start, p+1);
      }
      start = nullptr;
    }
  }
  if ( start ) {
    result.insert(result.end(), start, p);
  }
  return result;
}

namespace
{

const char* strSkipTillRightAfter(const char* p, const char* s)
{
    const int slen = strlen(s);
    for ( ; *p ; ++p) {
        if ( strncmp(p, s, slen) == 0 ) {
            return p + slen;
        }
    }
    return p;
}

inline const char* skipWhitespace(const char* p)
{
    while (*p == ' ')
        ++p;

    return p;
}

std::string getStringBeforeNext(const char* p, char c) {
    const char* const s = p;
    // XXX: don't run beyond end of string
    while(*p != c)
        p++;
    return std::string(s, p);
}

} // unnamed namespace

std::vector<html_link> generic_getLinks(const std::string& page)
{
    const char* p = page.c_str();
    std::vector<html_link> links;

    // The difference of the counts of the '<' and '>' characters preceding
    // the current position. In a valid HTML without comments it should only
    // take values 0 or 1.
    int ltgtBalance = 0;
    bool processingAScriptTag = false;

    while (*p) {
        if ( *p == '<' ) {
          if (strncmp(p, "<!--", 4) == 0) {
            p = strSkipTillRightAfter(p, "-->");
            continue;
          }
          ++ltgtBalance;
          ++p;
          if ( strncmp(p, "script", 6) == 0 && (p[6] == '>' || p[6] == ' ') ) {
            processingAScriptTag = true;
            p += 6;
          }
          continue;
        }
        if ( *p == '>' ) {
          --ltgtBalance;
          if ( processingAScriptTag ) {
            p = strSkipTillRightAfter(p, "</script>");
            processingAScriptTag = false;
          } else {
            ++p;
          }
          continue;
        }

        if ( ltgtBalance != 1 ) {
          ++p;
          continue;
        }

        html_link::AttributeKind attr;
        if (strncmp(p, " href", 5) == 0) {
            attr = html_link::HREF;
            p += 5;
        } else if (strncmp(p, " src", 4) == 0) {
            attr = html_link::SRC;
            p += 4;
        } else {
            p += 1;
            continue;
        }

        p = skipWhitespace(p);
        if (*(p++) != '=')
            continue;
        p = skipWhitespace(p);
        const char delimiter = *p++;
        if (delimiter != '\'' && delimiter != '"')
            continue;

        const auto link = getStringBeforeNext(p, delimiter);
        links.push_back(html_link(attr, decodeHtmlEntities(link)));
        p += link.size() + 1;
    }
    return links;
}

int adler32(const std::string& buf)
{
    unsigned int s1 = 1;
    unsigned int s2 = 0;
    unsigned int sz=buf.size();
    for (size_t n = 0; n <sz; n++)
    {
        s1 = (s1 + buf[n]) % 65521;
        s2 = (s2 + s1) % 65521;
    }
    return (s2 << 16) | s1;
}


namespace
{

// "abc/def/xyz"  -->  "abc/def"
// "abc/def/"     -->  "abc/def"
// "abc"          -->  ""
// "/abc"         -->  "/"
// "/"            -->  "/"
std::string getBasePath(const std::string& zimPath)
{
    const auto pos = zimPath.find_last_of('/');
    return pos == std::string::npos
         ? std::string()
         : zimPath.substr(0, pos == 0 ? 1 : pos);
}

bool dropLastSegmentOfThePath(std::string& path)
{
    if ( path.empty() )
        return false;

    const auto i = path.find_last_of("/");
    if ( i == std::string::npos ) {
        // "abc"  -->  ""
        path.clear();
    } else if ( i == 0 ) {
        // "/abc" -->  "/"
        // "/"    -->  ""
        path.resize(path == "/" ? 0 : 1);
    } else {
        // "abc/def/xyz" --> "abc/def
        // "abc/def/"    --> "abc/def
        // "abc/def"     --> "abc
        path.resize(i);
    }
    return true;
}

// Determine the separator to be used if the path has to be extended
const char* getPathSeparatorFor(const std::string& path)
{
    return path.empty() || path == "/" ? "" : "/";
}

void stripFragmentAndOrSearchComponent(std::string& url)
{
    // strip the fragment and/or search components, if any
    auto endOfPathComponent = url.find_last_of("#?");
    if ( endOfPathComponent != std::string::npos ) {
      url.resize(endOfPathComponent);
    }
}

} // unnamed namespace

InternalLinkResolver::InternalLinkResolver(const std::string& zimPath)
: zimEntryPath(zimPath)
, basePath(getBasePath(zimPath))
{
}

std::string InternalLinkResolver::resolveLinkTarget(std::string url) const
{
    stripFragmentAndOrSearchComponent(url);
    if (url.empty()) {
      return zimEntryPath;
    }

    if ( url.front() == '/' ) {
      throw AbsolutePathURL(url);
    }

    url = decodeUrl(url);

    std::string resolvedPath = basePath;
    const char* pathTerminator = "";
    const char* pathSeparator  = getPathSeparatorFor(resolvedPath);
    const char* const urlEnd = url.data() + url.size();
    for (const char* segStart = url.data(); segStart <= urlEnd; ) {
      const char* const segEnd = std::find(segStart, urlEnd, '/');
      const std::string_view p(segStart, segEnd - segStart);
      segStart = segEnd + 1;
      if ( p == ".") {
        pathSeparator = pathTerminator = getPathSeparatorFor(resolvedPath);
        continue;
      } else if ( p == ".." ) {
        if ( !dropLastSegmentOfThePath(resolvedPath) ) {
          throw OutOfBoundsURL(url);
        }
        pathSeparator = pathTerminator = getPathSeparatorFor(resolvedPath);
      } else {
        resolvedPath += pathSeparator;
        resolvedPath += p;
        pathTerminator = "";
        pathSeparator  = "/";
      }
    }

    return resolvedPath + pathTerminator;
}

std::string resolveLinkTarget(std::string url, const std::string& zimPath) {
  InternalLinkResolver resolver(zimPath);

  return resolver.resolveLinkTarget(url);
}

namespace
{

bool isAsciiAlpha(const char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

bool isAsciiAlphaNumeric(const char c)
{
    return isAsciiAlpha(c) || (c >= '0' && c <= '9');
}

bool isValidUriScheme(const std::string& scheme)
{
    if (scheme.empty() || !isAsciiAlpha(scheme.front())) {
        return false;
    }

    return std::all_of(
        scheme.begin() + 1,
        scheme.end(),
        [](const char c) { return isAsciiAlphaNumeric(c) || c == '+' || c == '-' || c == '.'; });
}

UriKind specialUriSchemeKind(const std::string& s)
{
    static const std::map<std::string, UriKind> uriSchemes = {
        { "javascript", UriKind::JAVASCRIPT },
        { "mailto",     UriKind::MAILTO     },
        { "tel",        UriKind::TEL        },
        { "sip",        UriKind::SIP        },
        { "geo",        UriKind::GEO        },
        { "data",       UriKind::DATA       },
        { "xmpp",       UriKind::XMPP       },
        { "news",       UriKind::NEWS       },
        { "urn",        UriKind::URN        }
    };

    const auto it = uriSchemes.find(s);
    return it != uriSchemes.end() ? it->second : UriKind::GENERIC_URI;
}


} // unnamed namespace

UriKind html_link::detectUriKind(std::string_view input_string)
{
    const auto k = input_string.find_first_of(":/?#");
    if ( k == std::string_view::npos || input_string[k] != ':' ) {
        if ( k == 0 && input_string.substr(0, 2) == "//" )
            return UriKind::PROTOCOL_RELATIVE;
        else
            return UriKind::OTHER;
    }

    const std::string raw_scheme(input_string.substr(0, k));
    if (!isValidUriScheme(raw_scheme)) {
        return UriKind::OTHER;
    }
    if ( k + 2 < input_string.size()
         && input_string[k+1] == '/'
         && input_string[k+2] == '/' )
        return UriKind::GENERIC_URI;

    const std::string scheme = asciitolower(raw_scheme);
    return specialUriSchemeKind(scheme);
}

namespace
{

static bool isReservedUrlChar(const char c)
{
    constexpr std::array<char, 10> reserved = {{';', ',', '?', ':',
                                               '@', '&', '=', '+', '$' }};

    return std::any_of(reserved.begin(), reserved.end(),
                       [&c] (const char &elem) { return elem == c; } );
}

bool needsEscape(const char c, const bool encodeReserved)
{
  if (isAsciiAlphaNumeric(c))
    return false;

  if (isReservedUrlChar(c))
    return encodeReserved;

  constexpr std::array<char, 10> noNeedEscape = {{'-', '_', '.', '!', '~',
                                                '*', '\'', '(', ')', '/' }};

  return not std::any_of(noNeedEscape.begin(), noNeedEscape.end(),
                         [&c] (const char &elem) { return elem == c; } );
}

std::string urlEncode(const std::string& value, bool encodeReserved)
{
  std::ostringstream os;
  os << std::hex << std::uppercase;
  for (std::string::const_iterator it = value.begin();
       it != value.end();
       ++it) {
    if (!needsEscape(*it, encodeReserved)) {
      os << *it;
    } else {
      os << '%' << std::setw(2) << static_cast<unsigned int>(static_cast<unsigned char>(*it));
    }
  }
  return os.str();
}

} // unnamed namespace

std::string httpRedirectHtml(const std::string& redirectUrl)
{
    const auto encodedurl = urlEncode(redirectUrl, true);
    std::ostringstream ss;

    ss << "<!DOCTYPE html>"
          "<html>"
          "<head>"
          "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />"
          "<meta http-equiv=\"refresh\" content=\"0;url=" + encodedurl + "\" />"
          "</head>"
          "<body></body>"
          "</html>";
    return ss.str();
}

bool guess_is_front_article(std::string_view mimetype) {
  return ( mimetype.find("text/html") == 0
        && mimetype.find("raw=true") == std::string_view::npos);
}

size_t getTextLength(std::string_view utf8EncodedString)
{
  // For some unknown reason implicite convertion from std::string to icu::StringPiece
  // is broken on Windows.
  // Constructors are definde in stringpiece.h as
  // ```
  // StringPiece(const std::string& str)
  //  : ptr_(str.data()), length_(static_cast<int32_t>(str.size())) { }
  // StringPiece(const char* offset, int32_t len) : ptr_(offset), length_(len) { }
  // ```
  // However using the first constructor ends with a corrupted StringPiece (wrong ptr)
  // and using second one works. Don't ask me why
  // This is broken
  // icu::StringPiece stringPiece(utf8EncodedString);
  // This is not
  icu::StringPiece stringPiece(utf8EncodedString.data(),
                               static_cast<int32_t>(utf8EncodedString.size()));
  icu::UnicodeString ustr = icu::UnicodeString::fromUTF8(stringPiece);

  UErrorCode status = U_ZERO_ERROR;
  std::unique_ptr<icu::BreakIterator> bi(
      icu::BreakIterator::createCharacterInstance(icu::Locale::getRoot(), status));

  if (!U_SUCCESS(status)) {
    return ustr.length();  // Fallback to codepoint count
  }

  bi->setText(ustr);

  size_t count = 0;
  while (bi->next() != icu::BreakIterator::DONE) {
    ++count;
  }
  return count;
}
