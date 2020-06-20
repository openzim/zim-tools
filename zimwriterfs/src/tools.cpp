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
#include <magic.h>
#include <string.h>
#include <sys/stat.h>
#include <zlib.h>
#include <cerrno>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <memory>

#include <unicode/translit.h>
#include <unicode/ucnv.h>

#ifdef _WIN32
#define SEPARATOR "\\"
#else
#define SEPARATOR "/"
#endif

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
  extMimeTypes["otf"] = "application/vnd.ms-opentype";
  extMimeTypes["OTF"] = "application/vnd.ms-opentype";
  extMimeTypes["eot"] = "application/vnd.ms-fontobject";
  extMimeTypes["EOT"] = "application/vnd.ms-fontobject";
  extMimeTypes["ttf"] = "application/font-ttf";
  extMimeTypes["TTF"] = "application/font-ttf";
  extMimeTypes["woff"] = "application/font-woff";
  extMimeTypes["WOFF"] = "application/font-woff";
  extMimeTypes["woff2"] = "application/font-woff2";
  extMimeTypes["WOFF2"] = "application/font-woff2";
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

extern std::string directoryPath;
extern bool inflateHtmlFlag;
extern bool uniqueNamespace;
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

unsigned int getFileSize(const std::string& path)
{
  struct stat filestatus;
  stat(path.c_str(), &filestatus);
  return filestatus.st_size;
}

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
    url.replace(pos, 3, 1, charFromHex(url.substr(pos + 1, 2)));
    ++pos;
  }
  return url;
}

std::string removeLastPathElement(const std::string& path,
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

/* Split string in a token array */
std::vector<std::string> split(const std::string& str,
                               const std::string& delims = " *-")
{
  std::string::size_type lastPos = str.find_first_not_of(delims, 0);
  std::string::size_type pos = str.find_first_of(delims, lastPos);
  std::vector<std::string> tokens;

  while (std::string::npos != pos || std::string::npos != lastPos) {
    tokens.push_back(str.substr(lastPos, pos - lastPos));
    lastPos = str.find_first_not_of(delims, pos);
    pos = str.find_first_of(delims, lastPos);
  }

  return tokens;
}

std::vector<std::string> split(const char* lhs, const char* rhs)
{
  const std::string m1(lhs), m2(rhs);
  return split(m1, m2);
}

std::vector<std::string> split(const char* lhs, const std::string& rhs)
{
  return split(lhs, rhs.c_str());
}

std::vector<std::string> split(const std::string& lhs, const char* rhs)
{
  return split(lhs.c_str(), rhs);
}

/* Warning: the relative path must be with slashes */
std::string computeAbsolutePath(const std::string& path,
                                const std::string& relativePath)
{
  /* Remove leaf part of the path if not already a directory */
  std::string absolutePath = path[path.length() - 1] == '/'
                                 ? path
                                 : removeLastPathElement(path, false, false);

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

/* Warning: the relative path must be with slashes */
std::string computeRelativePath(const std::string path,
                                const std::string absolutePath)
{
  std::vector<std::string> pathParts = split(path, "/");
  std::vector<std::string> absolutePathParts = split(absolutePath, "/");

  unsigned int commonCount = 0;
  while (commonCount < pathParts.size()
         && commonCount < absolutePathParts.size()
         && pathParts[commonCount] == absolutePathParts[commonCount]) {
    if (!pathParts[commonCount].empty()) {
      commonCount++;
    }
  }

  std::string relativePath;
  for (unsigned int i = commonCount; i < pathParts.size() - 1; i++) {
    relativePath += "../";
  }

  for (unsigned int i = commonCount; i < absolutePathParts.size(); i++) {
    relativePath += absolutePathParts[i];
    relativePath += i + 1 < absolutePathParts.size() ? "/" : "";
  }

  return relativePath;
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

std::string getMimeTypeForFile(const std::string& filename)
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

std::string getNamespaceForMimeType(const std::string& mimeType)
{
  if (uniqueNamespace || mimeType.find("text") == 0 || mimeType.empty()) {
    if (uniqueNamespace || mimeType.find("text/html") == 0
        || mimeType.empty()) {
      return "A";
    } else {
      return "-";
    }
  } else {
    if (mimeType == "application/font-ttf"
        || mimeType == "application/font-woff"
        || mimeType == "application/font-woff2"
        || mimeType == "application/vnd.ms-opentype"
        || mimeType == "application/vnd.ms-fontobject"
        || mimeType == "application/javascript"
        || mimeType == "application/json") {
      return "-";
    } else {
      return "I";
    }
  }
}

inline std::string removeLocalTagAndParameters(const std::string& url)
{
  std::string retVal = url;
  std::size_t found;

  /* Remove URL arguments */
  found = retVal.find("?");
  if (found != std::string::npos) {
    retVal = retVal.substr(0, found);
  }

  /* Remove local tag */
  found = retVal.find("#");
  if (found != std::string::npos) {
    retVal = retVal.substr(0, found);
  }

  return retVal;
}

std::string computeNewUrl(const std::string& aid, const std::string& baseUrl, const std::string& targetUrl)
{
  std::string filename = computeAbsolutePath(aid, targetUrl);
  std::string targetMimeType
      = getMimeTypeForFile(decodeUrl(removeLocalTagAndParameters(filename)));
  std::string newUrl
      = "/" + getNamespaceForMimeType(targetMimeType) + "/" + filename;
  return computeRelativePath(baseUrl, newUrl);
}

std::string removeAccents(const std::string& text)
{
  ucnv_setDefaultName("UTF-8");
  static UErrorCode status = U_ZERO_ERROR;
  static std::unique_ptr<icu::Transliterator> removeAccentsTrans(icu::Transliterator::createInstance(
      "Lower; NFD; [:M:] remove; NFC", UTRANS_FORWARD, status));
  icu::UnicodeString ustring(text.c_str());
  removeAccentsTrans->transliterate(ustring);
  std::string unaccentedText;
  ustring.toUTF8String(unaccentedText);
  return unaccentedText;
}

void remove_all(const std::string& path)
{
  DIR* dir;

  /* It's a directory, remove all its entries first */
  if ((dir = opendir(path.c_str())) != NULL) {
    struct dirent* ent;
    while ((ent = readdir(dir)) != NULL) {
      if (strcmp(ent->d_name, ".") and strcmp(ent->d_name, "..")) {
        std::string childPath = path + SEPARATOR + ent->d_name;
        remove_all(childPath);
      }
    }
    closedir(dir);
    rmdir(path.c_str());
  }

  /* It's a file */
  else {
    remove(path.c_str());
  }
}
