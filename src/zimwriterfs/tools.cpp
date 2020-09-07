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
#include "../tools.h"

#include <memory>
#include <string.h>
#include <sstream>
#include <fstream>
#include <iostream>

#include <zlib.h>
#include <magic.h>

/* Init file extensions hash */
static std::map<std::string, std::string> _create_extMimeTypes()
{
  std::map<std::string, std::string> extMimeTypes;
  extMimeTypes["HTML"] = "text/html";
  extMimeTypes["html"] = "text/html";
  extMimeTypes["HTM"] = "text/html";
  extMimeTypes["htm"] = "text/html";
  extMimeTypes["PNG"] = "image/png";
  extMimeTypes["png"] = "image/png";
  extMimeTypes["TIFF"] = "image/tiff";
  extMimeTypes["tiff"] = "image/tiff";
  extMimeTypes["TIF"] = "image/tiff";
  extMimeTypes["tif"] = "image/tiff";
  extMimeTypes["JPEG"] = "image/jpeg";
  extMimeTypes["jpeg"] = "image/jpeg";
  extMimeTypes["JPG"] = "image/jpeg";
  extMimeTypes["jpg"] = "image/jpeg";
  extMimeTypes["GIF"] = "image/gif";
  extMimeTypes["gif"] = "image/gif";
  extMimeTypes["SVG"] = "image/svg+xml";
  extMimeTypes["svg"] = "image/svg+xml";
  extMimeTypes["TXT"] = "text/plain";
  extMimeTypes["txt"] = "text/plain";
  extMimeTypes["XML"] = "text/xml";
  extMimeTypes["xml"] = "text/xml";
  extMimeTypes["EPUB"] = "application/epub+zip";
  extMimeTypes["epub"] = "application/epub+zip";
  extMimeTypes["PDF"] = "application/pdf";
  extMimeTypes["pdf"] = "application/pdf";
  extMimeTypes["OGG"] = "audio/ogg";
  extMimeTypes["ogg"] = "audio/ogg";
  extMimeTypes["OGV"] = "video/ogg";
  extMimeTypes["ogv"] = "video/ogg";
  extMimeTypes["JS"] = "application/javascript";
  extMimeTypes["js"] = "application/javascript";
  extMimeTypes["JSON"] = "application/json";
  extMimeTypes["json"] = "application/json";
  extMimeTypes["CSS"] = "text/css";
  extMimeTypes["css"] = "text/css";
  extMimeTypes["otf"] = "font/otf";
  extMimeTypes["OTF"] = "font/otf";
  extMimeTypes["sfnt"] = "font/sfnt";
  extMimeTypes["SFNT"] = "font/sfnt";
  extMimeTypes["eot"] = "application/vnd.ms-fontobject";
  extMimeTypes["EOT"] = "application/vnd.ms-fontobject";
  extMimeTypes["ttf"] = "font/ttf";
  extMimeTypes["TTF"] = "font/ttf";
  extMimeTypes["collection"] = "font/collection";
  extMimeTypes["COLLECTION"] = "font/collection";
  extMimeTypes["woff"] = "font/woff";
  extMimeTypes["WOFF"] = "font/woff";
  extMimeTypes["woff2"] = "font/woff2";
  extMimeTypes["WOFF2"] = "font/woff2";
  extMimeTypes["vtt"] = "text/vtt";
  extMimeTypes["VTT"] = "text/vtt";
  extMimeTypes["webm"] = "video/webm";
  extMimeTypes["WEBM"] = "video/webm";
  extMimeTypes["webp"] = "image/webp";
  extMimeTypes["WEBP"] = "image/webp";
  extMimeTypes["mp4"] = "video/mp4";
  extMimeTypes["MP4"] = "video/mp4";
  extMimeTypes["doc"] = "application/msword";
  extMimeTypes["DOC"] = "application/msword";
  extMimeTypes["docx"] = "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
  extMimeTypes["DOCX"] = "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
  extMimeTypes["ppt"] = "application/vnd.ms-powerpoint";
  extMimeTypes["PPT"] = "application/vnd.ms-powerpoint";
  extMimeTypes["odt"] = "application/vnd.oasis.opendocument.text";
  extMimeTypes["ODT"] = "application/vnd.oasis.opendocument.text";
  extMimeTypes["odp"] = "application/vnd.oasis.opendocument.text";
  extMimeTypes["ODP"] = "application/vnd.oasis.opendocument.text";
  extMimeTypes["zip"] = "application/zip";
  extMimeTypes["ZIP"] = "application/zip";
  extMimeTypes["wasm"] = "application/wasm";
  extMimeTypes["WASM"] = "application/wasm";

  return extMimeTypes;
}

static std::map<std::string, std::string> extMimeTypes = _create_extMimeTypes();

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
      return "text/html" == extMimeTypes[mimeType];
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

static bool isLocalUrl(const std::string url)
{
  if (url.find(":") != std::string::npos) {
    return (!(url.find("://") != std::string::npos || url.find("//") == 0
              || url.find("tel:") == 0
              || url.find("geo:") == 0
              || url.find("javascript:") == 0
              || url.find("mailto:") == 0));
  }
  return true;
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

void getLinks(GumboNode* node, std::map<std::string, bool>& links)
{
  if (node->type != GUMBO_NODE_ELEMENT) {
    return;
  }

  GumboAttribute* attribute = NULL;
  attribute = gumbo_get_attribute(&node->v.element.attributes, "href");
  if (attribute == NULL) {
    attribute = gumbo_get_attribute(&node->v.element.attributes, "src");
  }
  if (attribute == NULL) {
    attribute = gumbo_get_attribute(&node->v.element.attributes, "poster");
  }

  if (attribute != NULL && isLocalUrl(attribute->value)) {
    links[attribute->value] = true;
  }

  GumboVector* children = &node->v.element.children;
  for (unsigned int i = 0; i < children->length; ++i) {
    getLinks(static_cast<GumboNode*>(children->data[i]), links);
  }
}

std::string getMimeTypeForFile(const std::string &directoryPath, const std::string& filename)
{
  std::string mimeType;

  /* Try to get the mimeType from the file extension */
  auto index_of_last_dot = filename.find_last_of(".");
  if (index_of_last_dot != std::string::npos) {
    mimeType = filename.substr(index_of_last_dot + 1);
    try {
      return extMimeTypes.at(mimeType);
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
