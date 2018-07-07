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
#include <zim/writer/zimcreator.h>
#include <string>

extern std::string favicon;

class Article : public zim::writer::Article
{
 protected:
  char ns;
  std::string aid;
  std::string url;
  std::string title;
  std::string mimeType;
  std::string redirectAid;

 public:
  virtual std::string getAid() const;
  virtual char getNamespace() const;
  virtual std::string getUrl() const;
  virtual std::string getTitle() const;
  virtual bool isRedirect() const;
  virtual std::string getMimeType() const;
  virtual std::string getRedirectAid() const;
  virtual bool shouldCompress() const;
  virtual ~Article(){};
};

class MetadataArticle : public zim::writer::Article
{
 protected:
   std::string id;

 public:
  explicit MetadataArticle(const std::string& id) : id(id) {};
  virtual std::string getAid() const { return "/M/" + id; }
  virtual char getNamespace() const { return 'M'; }
  virtual std::string getUrl() const { return id; }
  virtual bool isInvalid() const { return false; }
  virtual std::string getTitle() const { return ""; }
  virtual bool isRedirect() const { return false; }
  virtual bool isLinktarget() const { return false; }
  virtual bool isDeleted() const { return false; }
  virtual std::string getMimeType() const { return "text/plain"; }
  virtual std::string getRedirectAid() const { return ""; }
  virtual bool shouldIndex() const { return false; }
  virtual bool shouldCompress() const { return true; }
  virtual std::string getFilename() const { return ""; }
};

class SimpleMetadataArticle : public MetadataArticle
{
 private:
  std::string value;

 public:
  explicit SimpleMetadataArticle(const std::string& id,
                                 const std::string& value);
  virtual zim::Blob getData() const
  {
    return zim::Blob(value.c_str(), value.size());
  }
  virtual zim::size_type getSize() const
  {
    return value.size();
  }
};

class MetadataFaviconArticle : public MetadataArticle
{
 private:
   std::string redirectAid;
 public:
  explicit MetadataFaviconArticle(std::string value)
    : MetadataArticle(""), redirectAid(value) {}
  virtual std::string getAid() const { return "/-/Favicon"; }
  virtual char getNamespace() const { return '-'; }
  virtual std::string getUrl() const { return "favicon"; }
  virtual bool isInvalid() const { return false; }
  virtual std::string getTitle() const { return ""; }
  virtual bool isRedirect() const { return true; }
  virtual std::string getMimeType() const { return "image/png"; }
  virtual std::string getRedirectAid() const { return redirectAid; }
  virtual bool shouldIndex() const { return false; }
  virtual bool shouldCompress() const { return false; }
  virtual std::string getFilename() const { return ""; }
  virtual zim::Blob getData() const { return zim::Blob(); }
  virtual zim::size_type getSize() const { return 0; }
};

class MetadataDateArticle : public MetadataArticle
{
 private:
  mutable std::string data;
  void genDate() const;

 public:
  MetadataDateArticle();
  virtual zim::Blob getData() const;
  virtual zim::size_type getSize() const;
};

class FileArticle : public Article
{
 private:
  mutable std::string data;
  mutable bool dataRead;
  bool invalid;
  std::string _getFilename() const;
  void readData() const;
  void parseAndAdaptHtml(bool detectRedirects);
  void adaptCss();

 public:
  explicit FileArticle(const std::string& path,
                       const bool detectRedirects = true);
  virtual zim::Blob getData() const;
  virtual bool isLinktarget() const { return false; }
  virtual bool isDeleted() const { return false; }
  virtual bool shouldIndex() const;
  virtual zim::size_type getSize() const;
  virtual std::string getFilename() const;
  virtual bool isInvalid() const;
};

class RedirectArticle : public Article
{
 public:
  explicit RedirectArticle(char ns,
                           const std::string& url,
                           const std::string& title,
                           const std::string& redirectAid)
  {
    this->ns = ns;
    this->url = url;
    this->title = title;
    this->redirectAid = redirectAid;
    aid = std::string("/") + ns + "/" + url;
    mimeType = "text/plain";
  }
  virtual zim::Blob getData() const { return zim::Blob(); }
  virtual bool isRedirect() const { return true; }
  virtual bool isLinktarget() const { return false; }
  virtual bool isDeleted() const { return false; }
  virtual bool shouldIndex() const { return false; }
  virtual zim::size_type getSize() const { return 0; }
  virtual std::string getFilename() const  { return ""; }
};

#endif  // OPENZIM_ZIMWRITERFS_ARTICLE_H
