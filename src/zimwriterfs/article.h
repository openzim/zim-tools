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

#ifndef OPENZIM_ZIMWRITERFS_ARTICLE_H
#define OPENZIM_ZIMWRITERFS_ARTICLE_H

#include <zim/blob.h>
#include <zim/writer/creator.h>
#include <string>

class ZimCreatorFS;

class Article : public zim::writer::Article
{
 protected:
  char ns;
  std::string url;
  std::string title;
  std::string mimeType;
  zim::writer::Url redirectUrl;

 public:
  virtual zim::writer::Url getUrl() const;
  virtual std::string getTitle() const;
  virtual bool isRedirect() const;
  virtual std::string getMimeType() const;
  virtual zim::writer::Url getRedirectUrl() const;
  virtual bool shouldIndex() const;
  virtual bool shouldCompress() const;
  virtual ~Article(){};
};

class FileArticle : public Article
{
 private:
  const ZimCreatorFS *creator;
  mutable std::string data;
  mutable bool dataRead;
  bool invalid;
  std::string _getFilename() const;
  void readData() const;

 public:
  //! Must be initialized with full file path
  explicit FileArticle(const ZimCreatorFS *creator,
                       const std::string& full_path,
                       const bool detect_html_redirects = true);
  virtual zim::Blob getData() const;
  virtual bool isLinktarget() const { return false; }
  virtual bool isDeleted() const { return false; }
  virtual zim::size_type getSize() const;

  //! Returns full filename; or empty string if content already read from the file
  virtual std::string getFilename() const;

  virtual bool isInvalid() const;
};

/// Redirect entry from user-supplied file
class RedirectArticle : public Article
{
 public:
  explicit RedirectArticle(const ZimCreatorFS *creator,
                           char ns,
                           const std::string& url,
                           const std::string& title,
                           const zim::writer::Url& redirectUrl);
  virtual zim::Blob getData() const { return zim::Blob(); }
  virtual bool isRedirect() const { return true; }
  virtual bool isLinktarget() const { return false; }
  virtual bool isDeleted() const { return false; }
  virtual zim::size_type getSize() const { return 0; }
  virtual std::string getFilename() const  { return ""; }
private:
  const ZimCreatorFS *creator;
};

#endif  // OPENZIM_ZIMWRITERFS_ARTICLE_H
