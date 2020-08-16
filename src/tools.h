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

std::string getMimeTypeForFile(const std::string& filename);
std::string getNamespaceForMimeType(const std::string& mimeType, bool uniqueNamespace);
std::string getFileContent(const std::string& path);
unsigned int getFileSize(const std::string& path);
std::string decodeUrl(const std::string& encodedUrl);
std::string computeAbsolutePath(const std::string& path,
                                const std::string& relativePath);
bool fileExists(const std::string& path);

std::string computeNewUrl(const std::string& aid, const std::string& baseUrl, const std::string& targetUrl, const bool uniqueNs);

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

void remove_all(const std::string& path);

//Returns a vector of the links in a particular page. includes links under 'href' and 'src'
std::vector<std::string> generic_getLinks(const std::string& page, bool withHref = true);

// checks if a relative path is out of bounds (relative to base)
bool isOutofBounds(const std::string& input, const std::string& base);

//Adler32 Hash Function. Used to hash the BLOB data obtained from each article, for redundancy checks.
//Please note that the adler32 hash function has a high number of collisions, and that the hash match is not taken as final.
int adler32(std::string buf);

//Removes extra spaces from URLs. Usually done by the browser, so web authors sometimes tend to ignore it.
//Converts the %20 to space.Essential for comparing URLs.
std::string normalize_link(const std::string& input, const std::string& baseUrl);


#endif  // OPENZIM_TOOLS_H
