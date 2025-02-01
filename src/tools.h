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
#include <vector>
#include <stdexcept>
#include <sstream>

#include <zim/writer/contentProvider.h>
#include <zim/writer/item.h>
#include <zim/item.h>

/* Formatter for std::exception what() message:
 * throw std::runtime_error(
 *   Formatter() << "zimwriterfs: Unable to read" << filename << ": " << strerror(errno));
 */
class Formatter
{
public:
    Formatter() {}
    ~Formatter() {}

    template <typename Type>
    Formatter & operator << (const Type & value)
    {
        stream_ << value;
        return *this;
    }

    // std::string str() const         { return stream_.str(); }
    operator std::string () const   { return stream_.str(); }

private:
    Formatter(const Formatter &);
    Formatter & operator = (Formatter &);

    std::stringstream stream_;
};

enum class UriKind : int
{
    // Special URIs without authority that are valid in HTML
    JAVASCRIPT,     // javascript:...
    MAILTO,         // mailto:user@example.com
    TEL,            // tel:+0123456789
    SIP,            // sip:1-999-123-4567@voip-provider.example.net
    GEO,            // geo:12.34,56.78
    DATA,           // data:image/png;base64,...
    XMPP,           // xmpp:kelson@kiwix.org
    NEWS,           // news:comp.os.linux.announce
    URN,            // urn:nbn:de:bsz:24-digibib-bsz3530416370

    GENERIC_URI,    // Generic URI with scheme and authority: <scheme>://.....
    PROTOCOL_RELATIVE, // Protocol-relative URL: //<host>/<path>/<to>/<resource>

    OTHER           // not a valid URI (though it can be a valid relative
                    // or absolute URL)
};

class html_link
{
public:
    const std::string attribute;
    const std::string link;
    const UriKind     uriKind;

    html_link(const std::string& _attr, const std::string& _link)
        : attribute(_attr)
        , link(_link)
        , uriKind(detectUriKind(_link))
    {}

    bool isExternalUrl() const
    {
        return uriKind != UriKind::OTHER && uriKind != UriKind::DATA;
    }

    bool isInternalUrl() const
    {
        return uriKind == UriKind::OTHER;
    }

    static UriKind detectUriKind(const std::string& input_string);
};

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

// Guess if the item is a front article.
// This is not a exact science, we use the mimetype to infer it.
bool guess_is_front_article(const std::string& mimetype);


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

    zim::writer::Hints getHints() const {
      return { { zim::writer::HintKeys::FRONT_ARTICLE, guess_is_front_article(item.getMimetype()) } };
    }
};

std::string decodeUrl(const std::string& encodedUrl);

// Assuming that basePath and targetPath are relative to the same location
// returns the relative path of targetPath from basePath
std::string computeRelativePath(const std::string& basePath,
                                const std::string& targetPath);

std::string computeAbsolutePath(const std::string& path,
                                const std::string& relativePath);
bool fileExists(const std::string& path);
bool isDirectory(const std::string &path);

std::string base64_encode(unsigned char const* bytes_to_encode,
                          unsigned int in_len);

void replaceStringInPlaceOnce(std::string& subject,
                              const std::string& search,
                              const std::string& replace);
void replaceStringInPlace(std::string& subject,
                          const std::string& search,
                          const std::string& replace);
void stripTitleInvalidChars(std::string& str);

//Returns a vector of the links in a particular page. includes links under 'href' and 'src'
std::vector<html_link> generic_getLinks(const std::string& page);

// checks if a relative path is out of bounds (relative to base)
bool isOutofBounds(const std::string& input, std::string base);

//Adler32 Hash Function. Used to hash the BLOB data obtained from each article, for redundancy checks.
//Please note that the adler32 hash function has a high number of collisions, and that the hash match is not taken as final.
int adler32(const std::string& buf);

std::string decodeHtmlEntities(const std::string& str);

//Removes extra spaces from URLs. Usually done by the browser, so web authors sometimes tend to ignore it.
//Converts the %20 to space.Essential for comparing URLs.
std::string normalize_link(const std::string& input, const std::string& baseUrl);

std::string httpRedirectHtml(const std::string& redirectUrl);
#endif  // OPENZIM_TOOLS_H
