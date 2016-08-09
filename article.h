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

#include <string>
#include <zim/writer/zimcreator.h>
#include <zim/blob.h>

extern std::string favicon;

class Article : public zim::writer::Article {
  protected:
    char ns;
    bool invalid;
    std::string aid;
    std::string url;
    std::string title;
    std::string mimeType;
    std::string redirectAid;

  public:
    virtual std::string getAid() const;
    virtual char getNamespace() const;
    virtual std::string getUrl() const;
    virtual bool isInvalid() const;
    virtual std::string getTitle() const;
    virtual bool isRedirect() const;
    virtual std::string getMimeType() const;
    virtual std::string getRedirectAid() const;
    virtual bool shouldCompress() const;
    virtual ~Article() {};
};

class MetadataArticle : public Article {
  public:
    explicit MetadataArticle(const std::string &id);
};

class SimpleMetadataArticle : public MetadataArticle {
  private:
    std::string value;
  public:
    explicit SimpleMetadataArticle(const std::string &id, const std::string& value);
    virtual zim::Blob getData() const { return zim::Blob(value.c_str(), value.size()); }
};

class MetadataFaviconArticle : public Article {
  public:
    explicit MetadataFaviconArticle(std::string value) {
      aid = "/-/Favicon";
      mimeType = "image/png";
      redirectAid = value;
      ns = '-';
      url = "favicon";
    }
    virtual zim::Blob getData() const { return zim::Blob(); }
};

class MetadataDateArticle : public MetadataArticle
{
  private:
    mutable std::string data;
  public:
    MetadataDateArticle();
    virtual zim::Blob getData() const;
};


class FileArticle : public Article {
  private:
    mutable std::string data;
    mutable bool        dataRead;

  public:
    explicit FileArticle(const std::string& id, const bool detectRedirects = true);
    virtual zim::Blob getData() const;
};


class RedirectArticle : public Article {
  public:
  RedirectArticle(const std::string &line) {
    size_t start;
    size_t end;
    ns = line[0];
    end = line.find_first_of("\t", 2);
    url = line.substr(2, end - 2);
    start = end + 1;
    end = line.find_first_of("\t", start);
    title = line.substr(start, end - start);
    redirectAid = line.substr(end + 1);
    aid = "/" + line.substr(0, 1) + "/" + url;
    mimeType = "text/plain";
  }
  virtual zim::Blob getData() const { return zim::Blob(); }
};

#endif // OPENZIM_ZIMWRITERFS_ARTICLE_H
