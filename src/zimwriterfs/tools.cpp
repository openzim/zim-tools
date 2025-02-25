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

#include "../mimetypes.h"
#include "tools.h"

#include <cstring>
#include <sstream>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <map>
#include <string>

#include <zlib.h>
#include <magic.h>

// specify transparent comparator for heterogeneous find support (c++14)
static std::map<std::string, const std::string, std::less<>> fileMimeTypes{};

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

inline bool seemsToBeHtml(const std::string_view path)
{
  static constexpr std::string_view dot{"."};
  const std::size_t lastDot = path.find_last_of(dot);

  if (lastDot != std::string_view::npos) {
    const std::string_view mimeType = path.substr(lastDot + 1);
    const auto mimeItr = extMimeTypes().find(mimeType);
    if (mimeItr != extMimeTypes().end()) {
      return mimeTextHtml == mimeItr->second;
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


std::string getMimeTypeForFile(const std::string &directoryPath, std::string_view filename)
{
  static constexpr std::string_view dot{"."};
  static constexpr std::string_view semicolon{";"};

  std::string mimeType;

  // Try to get the mimeType from the file extension
  const std::size_t index_of_last_dot = filename.find_last_of(dot);
  if (index_of_last_dot != std::string_view::npos) {
    auto extension = filename.substr(index_of_last_dot + 1);

    if(auto extMimeItr = extMimeTypes().find(extension);
       extMimeItr != extMimeTypes().cend()) {
        return std::string{extMimeItr->second};
    }
  }

  // Try to get the mimeType from the cache
  if(auto fileMimeItr = fileMimeTypes.find(filename);
     fileMimeItr != fileMimeTypes.cend()) {
    return std::string{fileMimeItr->second};
  }

  // operator+(string, string_view) is c++20, so use += for 17 compliance
  const std::string path = (directoryPath + "/") += filename;

  mimeType = std::string{::magic_file(magic, path.c_str())};

  // truncate if there is a semicolon in the magic_file result
  if (const std::size_t scOffset = mimeType.find(semicolon);
      scOffset != std::string::npos) {
    mimeType.resize(scOffset);
  }

  if(not mimeType.empty()) {
    using pair_type = decltype(fileMimeTypes)::value_type;
    fileMimeTypes.insert(pair_type{filename, mimeType});
    return mimeType;
  }

  return std::string{mimeAppOctetStream};
}
