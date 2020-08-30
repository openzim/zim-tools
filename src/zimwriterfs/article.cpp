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

#include <iomanip>
#include <sstream>
#include <fstream>

#include "article.h"
#include "tools.h"
#include "../tools.h"
#include "zimcreatorfs.h"


zim::writer::Url Article::getUrl() const
{
  return zim::writer::Url(ns, url);
}

std::string Article::getTitle() const
{
  return title;
}

bool Article::isRedirect() const
{
  return !redirectUrl.empty();
}

std::string Article::getMimeType() const
{
  return mimeType;
}

zim::writer::Url Article::getRedirectUrl() const
{
  return redirectUrl;
}

bool Article::shouldCompress() const
{
  return getMimeType().find("text") == 0
      || getMimeType() == "application/javascript"
      || getMimeType() == "application/json"
      || getMimeType() == "image/svg+xml";
}

bool Article::shouldIndex() const
{
  return getMimeType().find("text/html") == 0;
}

void FileArticle::readData() const
{
  data = getFileContent(_getFilename());
  dataRead = true;
}

FileArticle::FileArticle(const ZimCreatorFS *_creator, const std::string& full_path, const bool detect_html_redirects)
    : creator(_creator)
      , dataRead(false)
{
  invalid = false;

  url = full_path.substr(creator->basedir().size() + 1);

  /* mime-type */
  mimeType = getMimeTypeForFile(creator->basedir(), url);

  /* namespace */
  ns = getNamespaceForMimeType(mimeType, creator->uniqNamespace())[0];

  /* HTML specific code */
  if ( mimeType.find("text/html") != std::string::npos
    || mimeType.find("text/css") != std::string::npos ) {
    readData();
  }

  if ( mimeType.find("text/html") != std::string::npos ) {
    parseAndAdaptHtml(detect_html_redirects);
  } else if (mimeType.find("text/css") != std::string::npos) {
    adaptCss();
  }
}

bool FileArticle::isInvalid() const
{
  return invalid;
}

void FileArticle::parseAndAdaptHtml(bool detectRedirects)
{
  GumboOutput* output = gumbo_parse(data.c_str());
  GumboNode* root = output->root;

  /* Search the content of the <title> tag in the HTML */
  if (root->type == GUMBO_NODE_ELEMENT
      && root->v.element.children.length >= 2) {
    const GumboVector* root_children = &root->v.element.children;
    GumboNode* head = NULL;
    for (unsigned int i = 0; i < root_children->length; ++i) {
      GumboNode* child = (GumboNode*)(root_children->data[i]);
      if (child->type == GUMBO_NODE_ELEMENT
          && child->v.element.tag == GUMBO_TAG_HEAD) {
        head = child;
        break;
      }
    }

    if (head != NULL) {
      GumboVector* head_children = &head->v.element.children;
      for (unsigned int i = 0; i < head_children->length; ++i) {
        GumboNode* child = (GumboNode*)(head_children->data[i]);
        if (child->type == GUMBO_NODE_ELEMENT
            && child->v.element.tag == GUMBO_TAG_TITLE) {
          if (child->v.element.children.length == 1) {
            GumboNode* title_text
                = (GumboNode*)(child->v.element.children.data[0]);
            if (title_text->type == GUMBO_NODE_TEXT) {
              title = title_text->v.text.text;
              stripTitleInvalidChars(title);
            }
          }
        }
      }

      /* Detect if this is a redirection (if no redirects TSV file specified)
       */
      std::string targetUrl;
      try {
        targetUrl = detectRedirects
                        ? extractRedirectUrlFromHtml(head_children)
                        : "";
      } catch (std::string& error) {
        std::cerr << error << std::endl;
      }
      if (!targetUrl.empty()) {
        auto redirectUrl = computeAbsolutePath(url, decodeUrl(targetUrl));
        if (!fileExists(creator->basedir() + "/" + redirectUrl)) {
          redirectUrl.clear();
          invalid = true;
        } else {
          this->redirectUrl = zim::writer::Url(redirectUrl);
        }
      }

      /* If no title, then compute one from the filename */
      if (title.empty()) {
        auto path = _getFilename();
        auto found = path.rfind("/");
        if (found != std::string::npos) {
          title = path.substr(found + 1);
          found = title.rfind(".");
          if (found != std::string::npos) {
            title = title.substr(0, found);
          }
        } else {
          title = path;
        }
        std::replace(title.begin(), title.end(), '_', ' ');
      }
    }
  }

  /* Update links in the html to let them still be valid */
  std::map<std::string, bool> links;
  getLinks(root, links);
  std::string longUrl = std::string("/") + ns + "/" + url;

  /* If a link appearch to be duplicated in the HTML, it will
     occurs only one time in the links variable */
  for (auto& linkPair: links) {
    auto target = linkPair.first;
    if (!target.empty() && target[0] != '#' && target[0] != '?'
        && target.substr(0, 5) != "data:") {
      replaceStringInPlace(data,
                           "\"" + target + "\"",
                           "\"" + creator->computeNewUrl(url, longUrl, target) + "\"");
    }
  }
  gumbo_destroy_output(&kGumboDefaultOptions, output);
}

void FileArticle::adaptCss() {
  /* Rewrite url() values in the CSS */
  size_t startPos = 0;
  size_t endPos = 0;
  std::string url;
  std::string longUrl = std::string("/") + ns + "/" + this->url;

  while ((startPos = data.find("url(", endPos))
         && startPos != std::string::npos) {

    /* URL delimiters */
    endPos = data.find(")", startPos);
    startPos = startPos + (data[startPos + 4] == '\''
                                   || data[startPos + 4] == '"'
                               ? 5
                               : 4);
    endPos = endPos - (data[endPos - 1] == '\''
                               || data[endPos - 1] == '"'
                           ? 1
                           : 0);
    url = data.substr(startPos, endPos - startPos);
    std::string startDelimiter = data.substr(startPos - 1, 1);
    std::string endDelimiter = data.substr(endPos, 1);

    if (url.substr(0, 5) != "data:") {

      /* Deal with URL with arguments (using '? ') */
      std::string path = url;
      size_t markPos = url.find("?");
      if (markPos != std::string::npos) {
        path = url.substr(0, markPos);
      }

      /* Embeded fonts need to be inline because Kiwix is
         otherwise not able to load same because of the
         same-origin security */
      std::string mimeType = getMimeTypeForFile(creator->basedir(), path);
      if (mimeType == "application/font-ttf"
          || mimeType == "application/font-woff"
          || mimeType == "application/font-woff2"
          || mimeType == "application/vnd.ms-opentype"
          || mimeType == "application/vnd.ms-fontobject") {
        try {
          std::string fontContent = getFileContent(
              creator->basedir() + "/" + computeAbsolutePath(this->url, path));
          replaceStringInPlaceOnce(
              data,
              startDelimiter + url + endDelimiter,
              startDelimiter + "data:" + mimeType + ";base64,"
                  + base64_encode(reinterpret_cast<const unsigned char*>(
                                      fontContent.c_str()),
                                  fontContent.length())
                  + endDelimiter);
        } catch (...) {
        }
      } else {
        /* Deal with URL with arguments (using '? ') */
        if (markPos != std::string::npos) {
          endDelimiter = url.substr(markPos, 1);
        }

        replaceStringInPlaceOnce(
            data,
            startDelimiter + url + endDelimiter,
            startDelimiter + creator->computeNewUrl(this->url, longUrl, path) + endDelimiter);
      }
    }
  }
}

zim::Blob FileArticle::getData() const
{
  if (!dataRead)
    readData();

  return zim::Blob(data.data(), data.size());
}

std::string FileArticle::_getFilename() const
{
  return creator->basedir() + "/" + url;
}

std::string FileArticle::getFilename() const
{
  // If data is read (because we changed the content), use the content
  // not the content of the filename.
  if (dataRead)
    return "";
  return _getFilename();
}

zim::size_type FileArticle::getSize() const
{
  if (dataRead) {
    return data.size();
  }

  std::ifstream in(_getFilename(), std::ios::binary|std::ios::ate);
  return in.tellg();
}

RedirectArticle::RedirectArticle(const ZimCreatorFS *_creator,
                                 char ns,
                                 const std::string& url,
                                 const std::string& title,
                                 const zim::writer::Url& redirectUrl)
  : creator(_creator)
{
  this->ns = ns;
  this->url = url;
  this->title = title;
  this->redirectUrl = redirectUrl;
  mimeType = getMimeTypeForFile(creator->basedir(), redirectUrl.getUrl());
}

SimpleMetadataArticle::SimpleMetadataArticle(const std::string& id,
                                             const std::string& value)
    : MetadataArticle(id), value(value)
{
}

MetadataDateArticle::MetadataDateArticle() : MetadataArticle("Date")
{
}

void MetadataDateArticle::genDate() const
{
  time_t t = time(0);
  struct tm* now = localtime(&t);
  std::stringstream stream;
  stream << (now->tm_year + 1900) << '-' << std::setw(2) << std::setfill('0')
         << (now->tm_mon + 1) << '-' << std::setw(2) << std::setfill('0')
         << now->tm_mday;
  data = stream.str();
}

zim::Blob MetadataDateArticle::getData() const
{
  if (data.empty()) {
    genDate();
  }
  return zim::Blob(data.data(), data.size());
}

zim::size_type MetadataDateArticle::getSize() const
{
  if(data.empty()) {
    genDate();
  }
  return data.size();
}

