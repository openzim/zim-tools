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
#include <sstream>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <map>
#include <string_view>

#include <zlib.h>
#include <magic.h>

// adapted from cppreference example implementation of lexicographical_compare
struct tolower_str_comparator {
    using UChar_t = unsigned char;

    static constexpr UChar_t toLower(const UChar_t c) noexcept {
        constexpr UChar_t offset {'a' - 'A'};
        return (c <= 'Z' && c >= 'A') ? c + offset : c;
    }

    static constexpr bool charCompare(const UChar_t lhs, const UChar_t rhs) noexcept {
        return toLower(lhs) < toLower(rhs);
    }

    constexpr bool operator() (const std::string_view lhs, const std::string_view rhs) const noexcept {
        auto first1 = lhs.cbegin();
        const auto last1 = lhs.cend();
        auto first2 = rhs.cbegin();
        const auto last2 = rhs.cend();

        for (; (first1 != last1) && (first2 != last2); ++first1, ++first2) {
            if (charCompare(*first1, *first2)) {
                return true;
            }
            if (charCompare(*first2, *first1)) {
                return false;
            }
        }
        return (first1 == last1) && (first2 != last2);
    }
};

using MimeMap_t = std::map<std::string, const std::string, tolower_str_comparator>;

static const MimeMap_t extMimeTypes {
      {"html", "text/html"},
      {"htm", "text/html"},
      {"png", "image/png"},
      {"tiff", "image/tiff"},
      {"tif", "image/tiff"},
      {"jpeg", "image/jpeg"},
      {"jpg", "image/jpeg"},
      {"gif", "image/gif"},
      {"svg", "image/svg+xml"},
      {"txt", "text/plain"},
      {"xml", "text/xml"},
      {"epub", "application/epub+zip"},
      {"pdf", "application/pdf"},
      {"ogg", "audio/ogg"},
      {"ogv", "video/ogg"},
      {"js", "application/javascript"},
      {"json", "application/json"},
      {"css", "text/css"},
      {"otf", "font/otf"},
      {"sfnt", "font/sfnt"},
      {"eot", "application/vnd.ms-fontobject"},
      {"ttf", "font/ttf"},
      {"collection", "font/collection"},
      {"woff", "font/woff"},
      {"woff2", "font/woff2"},
      {"vtt", "text/vtt"},
      {"webm", "video/webm"},
      {"webp", "image/webp"},
      {"mp4", "video/mp4"},
      {"doc", "application/msword"},
      {"docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
      {"ppt", "application/vnd.ms-powerpoint"},
      {"odt", "application/vnd.oasis.opendocument.text"},
      {"odp", "application/vnd.oasis.opendocument.text"},
      {"zip", "application/zip"},
      {"wasm", "application/wasm"},
};

static std::map<std::string, std::string> fileMimeTypes;

extern bool inflateHtmlFlag;

extern magic_t magic;

/* Decompress an STL string using zlib and return the original data. */
inline std::string inflateString(const std::string& str)
{
  z_stream zs;  // z_stream is zlib's control structure
  memset(&zs, 0, sizeof(zs));

  if (inflateInit(&zs) != Z_OK)
    throw(std::runtime_error("inflateInit failed while decompressing."));

  zs.next_in = (Bytef*)str.data();
  zs.avail_in = str.size();

  int ret;
  char outbuffer[32768];
  std::string outstring;

  // get the decompressed bytes blockwise using repeated calls to inflate
  do {
    zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
    zs.avail_out = sizeof(outbuffer);

    ret = inflate(&zs, 0);

    if (outstring.size() < zs.total_out) {
      outstring.append(outbuffer, zs.total_out - outstring.size());
    }
  } while (ret == Z_OK);

  inflateEnd(&zs);

  if (ret != Z_STREAM_END) {  // an error occurred that was not EOF
    std::ostringstream oss;
    oss << "Exception during zlib decompression: (" << ret << ") " << zs.msg;
    throw(std::runtime_error(oss.str()));
  }

  return outstring;
}

inline bool seemsToBeHtml(const std::string& path)
{
  if (path.find_last_of(".") != std::string::npos) {
    std::string mimeType = path.substr(path.find_last_of(".") + 1);
    if (extMimeTypes.find(mimeType) != extMimeTypes.end()) {
      return "text/html" == extMimeTypes.at(mimeType);
    }
  }

  return false;
}

std::string getFileContent(const std::string& path)
{
  std::ifstream in(path, std::ios::binary| std::ios::ate);
  if (in) {
    std::string contents;
    contents.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(&contents[0], contents.size());
    in.close();

    /* Inflate if necessary */
    if (inflateHtmlFlag && seemsToBeHtml(path)) {
      try {
        contents = inflateString(contents);
      } catch (...) {
        std::cerr << "Can not initialize inflate stream for: " << path
                  << std::endl;
      }
    }
    return (contents);
  }
  std::cerr << "zimwriterfs: unable to open file at path: " << path
            << std::endl;
  throw(errno);
}

std::string extractRedirectUrlFromHtml(const GumboVector* head_children)
{
  std::string url;

  for (unsigned int i = 0; i < head_children->length; ++i) {
    GumboNode* child = (GumboNode*)(head_children->data[i]);
    if (child->type == GUMBO_NODE_ELEMENT
        && child->v.element.tag == GUMBO_TAG_META) {
      GumboAttribute* attribute;
      if ((attribute
           = gumbo_get_attribute(&child->v.element.attributes, "http-equiv"))
          != NULL) {
        if (!strcmp(attribute->value, "refresh")) {
          if ((attribute
               = gumbo_get_attribute(&child->v.element.attributes, "content"))
              != NULL) {
            std::string targetUrl = attribute->value;
            std::size_t found = targetUrl.find("URL=") != std::string::npos
                                    ? targetUrl.find("URL=")
                                    : targetUrl.find("url=");
            if (found != std::string::npos) {
              url = targetUrl.substr(found + 4);
            } else {
              throw std::string(
                  "Unable to find the redirect/refresh target url from the "
                  "HTML DOM");
            }
          }
        }
      }
    }
  }

  return url;
}

std::string generateDate()
{
  time_t t = time(0);
  struct tm* now = localtime(&t);
  std::stringstream stream;
  stream << (now->tm_year + 1900) << '-' << std::setw(2) << std::setfill('0')
         << (now->tm_mon + 1) << '-' << std::setw(2) << std::setfill('0')
         << now->tm_mday;
  return stream.str();
}


std::string getMimeTypeForFile(const std::string &directoryPath, const std::string& filename)
{
  std::string mimeType;

  /* Try to get the mimeType from the file extension */
  auto index_of_last_dot = filename.find_last_of(".");
  if (index_of_last_dot != std::string::npos) {
    auto extension = filename.substr(index_of_last_dot + 1);
    try {
      return extMimeTypes.at(extension);
    } catch (std::out_of_range&) {}
  }

  /* Try to get the mimeType from the cache */
  try {
    return fileMimeTypes.at(filename);
  } catch (std::out_of_range&) {}

  /* Try to get the mimeType with libmagic */
  try {
    std::string path = directoryPath + "/" + filename;
    mimeType = std::string(magic_file(magic, path.c_str()));
    if (mimeType.find(";") != std::string::npos) {
      mimeType = mimeType.substr(0, mimeType.find(";"));
    }
    fileMimeTypes[filename] = mimeType;
  } catch (...) { }
  if (mimeType.empty()) {
    return "application/octet-stream";
  } else {
    return mimeType;
  }
}
