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

#include "article.h"
#include "tools.h"

#include <iomanip>
#include <sstream>


extern std::string directoryPath;

std::string Article::getAid() const
{
  return aid;
}

bool Article::isInvalid() const
{
  return invalid;
}

char Article::getNamespace() const
{
  return ns;
}

std::string Article::getUrl() const
{
  return url;
}

std::string Article::getTitle() const
{
  return title;
}

bool Article::isRedirect() const
{
  return !redirectAid.empty();
}

std::string Article::getMimeType() const
{
  return mimeType;
}

std::string Article::getRedirectAid() const
{
  return redirectAid;
}

bool Article::shouldCompress() const {
  return (getMimeType().find("text") == 0 ||
	  getMimeType() == "application/javascript" ||
	  getMimeType() == "application/json" ||
          getMimeType() == "image/svg+xml" ? true : false);
}

FileArticle::FileArticle(const std::string& path, const bool detectRedirects):
    dataRead(false)
{
  invalid = false;

  /* aid */
  aid = path.substr(directoryPath.size()+1);

  /* url */
  url = aid;

  /* mime-type */
  mimeType = getMimeTypeForFile(aid);
  
  /* namespace */
  ns = getNamespaceForMimeType(mimeType)[0];

  /* HTML specific code */
  if (mimeType.find("text/html") != std::string::npos) {
    std::size_t found;
    std::string html = getFileContent(path);
    GumboOutput* output = gumbo_parse(html.c_str());
    GumboNode* root = output->root;

    /* Search the content of the <title> tag in the HTML */
    if (root->type == GUMBO_NODE_ELEMENT && root->v.element.children.length >= 2) {
      const GumboVector* root_children = &root->v.element.children;
      GumboNode* head = NULL;
      for (unsigned int i = 0; i < root_children->length; ++i) {
	GumboNode* child = (GumboNode*)(root_children->data[i]);
	if (child->type == GUMBO_NODE_ELEMENT &&
	    child->v.element.tag == GUMBO_TAG_HEAD) {
	  head = child;
	  break;
	}
      }

      if (head != NULL) {
	GumboVector* head_children = &head->v.element.children;
	for (unsigned int i = 0; i < head_children->length; ++i) {
	  GumboNode* child = (GumboNode*)(head_children->data[i]);
	  if (child->type == GUMBO_NODE_ELEMENT &&
	      child->v.element.tag == GUMBO_TAG_TITLE) {
	    if (child->v.element.children.length == 1) {
	      GumboNode* title_text = (GumboNode*)(child->v.element.children.data[0]);
	      if (title_text->type == GUMBO_NODE_TEXT) {
		title = title_text->v.text.text;
		stripTitleInvalidChars(title);
	      }
	    }
	  }
	}

	/* Detect if this is a redirection (if no redirects CSV specified) */
	std::string targetUrl;
	try {
	  targetUrl = detectRedirects ? extractRedirectUrlFromHtml(head_children) : "";
	} catch (std::string &error) {
	  std::cerr << error << std::endl;
	}
	if (!targetUrl.empty()) {
	  redirectAid = computeAbsolutePath(aid, decodeUrl(targetUrl));
	  if (!fileExists(directoryPath + "/" + redirectAid)) {
	    redirectAid.clear();
	    invalid = true;
	  }
	}
      }

      /* If no title, then compute one from the filename */
      if (title.empty()) {
	found = path.rfind("/");
	if (found != std::string::npos) {
	  title = path.substr(found+1);
	  found = title.rfind(".");
	  if (found!=std::string::npos) {
	    title = title.substr(0, found);
	  }
	} else {
	  title = path;
	}
	std::replace(title.begin(), title.end(), '_',  ' ');
      }
    }

    /* Update links in the html to let them still be valid */
    std::map<std::string, bool> links;
    getLinks(root, links);
    std::map<std::string, bool>::iterator it;

    /* If a link appearch to be duplicated in the HTML, it will
       occurs only one time in the links variable */
    for(it = links.begin(); it != links.end(); it++) {
      if (!it->first.empty()
        && it->first[0] != '#'
        && it->first[0] != '?'
        && it->first.substr(0, 5) != "data:") {
        replaceStringInPlace(html, "\"" + it->first + "\"", "\"" + computeNewUrl(aid, it->first) + "\"");
      }
    }

    data = html;
    dataRead = true;

    gumbo_destroy_output(&kGumboDefaultOptions, output);
  }
}

zim::Blob FileArticle::getData() const {
    if ( dataRead )
        return zim::Blob(data.data(), data.size());;

    std::string aidPath = directoryPath + "/" + aid;
    std::string fileContent = getFileContent(aidPath);

    if (getMimeType().find("text/css") == 0) {
        /* Rewrite url() values in the CSS */
        size_t startPos = 0;
        size_t endPos = 0;
        std::string url;

        while ((startPos = fileContent.find("url(", endPos)) && startPos != std::string::npos) {
            /* URL delimiters */
            endPos = fileContent.find(")", startPos);
            startPos = startPos + (fileContent[startPos+4] == '\'' || fileContent[startPos+4] == '"' ? 5 : 4);
            endPos = endPos - (fileContent[endPos-1] == '\'' || fileContent[endPos-1] == '"' ? 1 : 0);
            url = fileContent.substr(startPos, endPos - startPos);
            std::string startDelimiter = fileContent.substr(startPos-1, 1);
            std::string endDelimiter = fileContent.substr(endPos, 1);

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
                std::string mimeType = getMimeTypeForFile(path);
                if ( mimeType == "application/font-ttf"
                  || mimeType == "application/font-woff"
                  || mimeType == "application/vnd.ms-opentype"
                  || mimeType == "application/vnd.ms-fontobject") {
                    try {
                        std::string fontContent = getFileContent(directoryPath
                                                               + "/"
                                                               + computeAbsolutePath(aid, path));
                        replaceStringInPlaceOnce(fileContent,
                                                 startDelimiter + url + endDelimiter,
                                                 startDelimiter + "data:" + mimeType + ";base64," +
                                                 base64_encode(reinterpret_cast<const unsigned char*>(fontContent.c_str()),
                                                               fontContent.length()) + endDelimiter);
                    } catch (...) {}
                } else {
                    /* Deal with URL with arguments (using '? ') */
                    if (markPos != std::string::npos) {
                        endDelimiter = url.substr(markPos, 1);
                    }

                        replaceStringInPlaceOnce(fileContent,
                                                 startDelimiter + url + endDelimiter,
                                                 startDelimiter + computeNewUrl(aid, path) + endDelimiter);
               }
           }
        }
    }

    data = fileContent;
    dataRead = true;
    return zim::Blob(data.data(), data.size());
}


MetadataArticle::MetadataArticle(const std::string &id) {
    aid = "/M/"+id;
    mimeType = "text/plain";
    ns = 'M';
    url = id;
}

SimpleMetadataArticle::SimpleMetadataArticle(const std::string &id, const std::string &value):
    MetadataArticle(id),
    value(value)
{}

MetadataDateArticle::MetadataDateArticle():
    MetadataArticle("Date")
{}

zim::Blob MetadataDateArticle::getData() const
{
  if ( data.size() == 0 )
  {
    time_t t = time(0);
    struct tm * now = localtime( & t );
    std::stringstream stream;
    stream << (now->tm_year + 1900) << '-'
           << std::setw(2) << std::setfill('0') << (now->tm_mon + 1) << '-'
           << std::setw(2) << std::setfill('0') << now->tm_mday;
    data = stream.str();
  }
  return zim::Blob(data.data(), data.size());
}

