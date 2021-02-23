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

#include <dirent.h>
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
#include <unistd.h>
#include <algorithm>
#include <regex>
#include <array>

#ifdef _WIN32
#define SEPARATOR "\\"
#else
#define SEPARATOR "/"
#endif


bool fileExists(const std::string& path)
{
  bool flag = false;
  std::fstream fin;
  fin.open(path.c_str(), std::ios::in);
  if (fin.is_open()) {
    flag = true;
  }
  fin.close();
  return flag;
}

bool isDirectory(const std::string &path)
{
  struct stat filestatus;
  stat(path.c_str(), &filestatus);
  return (filestatus.st_mode & S_IFMT) == S_IFDIR;
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
  size_t offset = newPath.find_last_of(SEPARATOR);

  if (removePreSeparator && offset == newPath.length() - 1) {
    newPath = newPath.substr(0, offset);
    offset = newPath.find_last_of(SEPARATOR);
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

} // unnamed namespace

std::string
computeRelativePath(const std::string& path1, const std::string& path2)
{
    const auto a = split(path1, '/');
    const auto b = split(path2, '/');
    const auto l = std::min(a.size(), b.size());

    const auto x = mismatch(a.begin(), a.begin() + l, b.begin());

    const size_t ups = (a.end() - x.first) - 1;
    std::string r;
    for ( size_t i = 0; i < ups; ++i ) {
      r += "../";
    }

    for ( auto it = x.second; it != b.end(); ++it ) {
        if ( it != x.second )
          r += "/";
        r += *it;
    }
    return r;
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
                              const std::string& search,
                              const std::string& replace)
{
  size_t pos = subject.find(search, 0);
  if (pos != std::string::npos) {
    subject.replace(pos, search.length(), replace);
  }
}

void replaceStringInPlace(std::string& subject,
                          const std::string& search,
                          const std::string& replace)
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

std::vector<html_link> generic_getLinks(const std::string& page)
{
    const char* p = page.c_str();
    const char* linkStart;
    std::vector<html_link> links;
    std::string attr;

    while (*p) {
        if (strncmp(p, " href", 5) == 0) {
            attr = "href";
            p += 5;
        } else if (strncmp(p, " src", 4) == 0) {
            attr = "src";
            p += 4;
        } else {
            p += 1;
            continue;
        }

        while (*p == ' ')
            p += 1 ;
        if (*(p++) != '=')
            continue;
        while (*p == ' ')
            p += 1;
        char delimiter = *p++;
        if (delimiter != '\'' && delimiter != '"')
            continue;

        linkStart = p;
        // [TODO] Handle escape char
        while(*p != delimiter)
            p++;
        const std::string link(linkStart, p);
        links.push_back(html_link(attr, link));
        p += 1;
    }
    return links;
}

bool isOutofBounds(const std::string& input, std::string base)
{
    if (input.empty()) return false;

    if (!base.length() || base.back() != '/')
        base.push_back('/');

    int nr = 0;
    if (base.front() != '/')
        nr++;

    //count nr of substrings ../
    int nrsteps = 0;
    std::string::size_type pos = 0;
    while((pos = input.find("../", pos)) != std::string::npos) {
        nrsteps++;
        pos += 3;
    }

    return nrsteps >= (nr + std::count(base.cbegin(), base.cend(), '/'));
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

std::string normalize_link(const std::string& input, const std::string& baseUrl)
{
    std::string output;
    output.reserve(baseUrl.size() + input.size() + 1);

    bool in_query = false;
    bool check_rel = false;
    const char* p = input.c_str();
    if ( *(p) == '/') {
      // This is an absolute url.
      p++;
    } else {
      //This is a relative url, use base url
      output = baseUrl;
      if (output.back() != '/')
          output += '/';
      check_rel = true;
    }

    //URL Decoding.
    while (*p)
    {
        if ( !in_query && check_rel ) {
            if (strncmp(p, "../", 3) == 0) {
                // We must go "up"
                // Remove the '/' at the end of output.
                output.resize(output.size()-1);
                // Remove the last part.
                auto pos = output.find_last_of('/');
                output.resize(pos==output.npos?0:pos);
                // Move after the "..".
                p += 2;
                check_rel = false;
                continue;
            }
            if (strncmp(p, "./", 2) == 0) {
                // We must simply skip this part
                // Simply move after the ".".
                p += 2;
                check_rel = false;
                continue;
            }
        }
        if ( *p == '#' || *p == '?')
            // This is a beginning of the #anchor inside a page. No need to decode more
            break;
        if ( *p == '%')
        {
            char ch;
            sscanf(p+1, "%2hhx", &ch);
            output += ch;
            p += 3;
            continue;
        }
        if ( *p == '?' ) {
            // We are in the query, so don't try to interprete '/' as path separator
            in_query = true;
        }
        if ( *p == '/') {
            check_rel = true;
            if (output.empty()) {
                // Do not add '/' at beginning of output
                p++;
                continue;
            }
        }
        output += *(p++);
    }
    return output;
}

namespace
{

UriKind specialUriSchemeKind(const std::string& s)
{
    static const std::map<std::string, UriKind> uriSchemes = {
        { "javascript", UriKind::JAVASCRIPT },
        { "mailto",     UriKind::MAILTO     },
        { "tel",        UriKind::TEL        },
        { "geo",        UriKind::GEO        },
        { "data",       UriKind::DATA       },
        { "xmpp",       UriKind::XMPP       },
        { "news",       UriKind::NEWS       },
        { "urn",        UriKind::URN        }
    };

    const auto it = uriSchemes.find(s);
    return it != uriSchemes.end() ? it->second : UriKind::OTHER;
}

void asciitolower(std::string& s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){
        return ('A' <= c && c <= 'Z') ? c - ('Z' - 'z') : c;
    });
}

} // unnamed namespace

UriKind html_link::detectUriKind(const std::string& input_string)
{
    const auto k = input_string.find_first_of(":/?#");
    if ( k == std::string::npos || input_string[k] != ':' )
        return UriKind::OTHER;

    if ( k + 2 < input_string.size()
         && input_string[k+1] == '/'
         && input_string[k+2] == '/' )
        return UriKind::GENERIC_URI;

    std::string scheme = input_string.substr(0, k);
    asciitolower(scheme);
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
  if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
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
