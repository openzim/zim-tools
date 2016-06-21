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


extern std::string directoryPath;

Article::Article(const std::string& path, const bool detectRedirects) {
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
      for (int i = 0; i < root_children->length; ++i) {
	GumboNode* child = (GumboNode*)(root_children->data[i]);
	if (child->type == GUMBO_NODE_ELEMENT &&
	    child->v.element.tag == GUMBO_TAG_HEAD) {
	  head = child;
	  break;
	}
      }

      if (head != NULL) {
	GumboVector* head_children = &head->v.element.children;
	for (int i = 0; i < head_children->length; ++i) {
	  GumboNode* child = (GumboNode*)(head_children->data[i]);
	  if (child->type == GUMBO_NODE_ELEMENT &&
	      child->v.element.tag == GUMBO_TAG_TITLE) {
	    if (child->v.element.children.length == 1) {
	      GumboNode* title_text = (GumboNode*)(child->v.element.children.data[0]);
	      if (title_text->type == GUMBO_NODE_TEXT) {
		title = title_text->v.text.text;
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

    gumbo_destroy_output(&kGumboDefaultOptions, output);
  }
}

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
