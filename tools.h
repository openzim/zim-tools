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


#ifndef OPENZIM_ZIMWRITERFS_TOOLS_H
#define OPENZIM_ZIMWRITERFS_TOOLS_H

#include <string>
#include <map>
#include <gumbo.h>

std::string getMimeTypeForFile(const std::string& filename);
std::string getNamespaceForMimeType(const std::string& mimeType);
std::string getFileContent(const std::string &path);
unsigned int getFileSize(const std::string &path);
std::string decodeUrl(const std::string &encodedUrl);
std::string computeAbsolutePath(const std::string& path, const std::string& relativePath);
bool fileExists(const std::string &path);
std::string removeLastPathElement(const std::string& path, const bool removePreSeparator, const bool removePostSeparator);
std::string computeNewUrl(const std::string &aid, const std::string &url);

std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len);

void replaceStringInPlaceOnce(std::string& subject, const std::string& search, const std::string& replace);
void replaceStringInPlace(std::string& subject, const std::string& search, const std::string& replace);
void stripTitleInvalidChars(std::string & str);

std::string extractRedirectUrlFromHtml(const GumboVector* head_children);
void getLinks(GumboNode* node, std::map<std::string, bool> &links);

#endif // OPENZIM_ZIMWRITERFS_TOOLS_H
