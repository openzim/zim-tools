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
