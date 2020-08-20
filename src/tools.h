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

#ifndef OPENZIM_TOOLS_H
#define OPENZIM_TOOLS_H

#include <map>
#include <string>

#include <zim/writer/contentProvider.h>
#include <zim/writer/item.h>
#include <zim/item.h>

std::string getMimeTypeForFile(const std::string& filename);
std::string getNamespaceForMimeType(const std::string& mimeType);
std::string getFileContent(const std::string& path);
unsigned int getFileSize(const std::string& path);
std::string decodeUrl(const std::string& encodedUrl);
std::string computeAbsolutePath(const std::string& path,
                                const std::string& relativePath);
bool fileExists(const std::string& path);
std::string removeLastPathElement(const std::string& path,
                                  const bool removePreSeparator,
                                  const bool removePostSeparator);
std::string computeNewUrl(const std::string& aid, const std::string& baseUrl, const std::string& targetUrl);

std::string base64_encode(unsigned char const* bytes_to_encode,
                          unsigned int in_len);

void replaceStringInPlaceOnce(std::string& subject,
                              const std::string& search,
                              const std::string& replace);
void replaceStringInPlace(std::string& subject,
                          const std::string& search,
                          const std::string& replace);
void stripTitleInvalidChars(std::string& str);

std::string computeRelativePath(const std::string path,
                                const std::string absolutePath);

std::string removeAccents(const std::string& text);

void remove_all(const std::string& path);

// Few helper class to help copy a item from a archive to another one.
class ItemProvider : public zim::writer::ContentProvider
{
    zim::Item item;
    bool feeded;
  public:
    ItemProvider(zim::Item item)
      : item(item),
        feeded(false)
    {}

    zim::size_type getSize() const {
      return item.getSize();
    }

    zim::Blob feed() {
      if (feeded) {
        return zim::Blob();
      }
      feeded = true;
      return item.getData();
    }
};


class CopyItem : public zim::writer::Item         //Article class that will be passed to the zimwriter. Contains a zim::Article class, so it is easier to add a
{
    //article from an existing ZIM file.
    zim::Item item;

  public:
    explicit CopyItem(const zim::Item item):
      item(item)
    {}

    virtual std::string getPath() const
    {
        return item.getPath();
    }

    virtual std::string getTitle() const
    {
        return item.getTitle();
    }

    virtual std::string getMimeType() const
    {
        return item.getMimetype();
    }

    std::unique_ptr<zim::writer::ContentProvider> getContentProvider() const
    {
       return std::unique_ptr<zim::writer::ContentProvider>(new ItemProvider(item));
    }
};


#endif  // OPENZIM_TOOLS_H
