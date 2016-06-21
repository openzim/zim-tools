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

#include "articlesource.h"
#include "article.h"
#include "tools.h"

#include <zim/blob.h>

#include <iomanip>
#include <sstream>
#include <map>

bool popFromFilenameQueue(std::string &filename);
bool isVerbose();

extern std::string welcome;
extern std::string language;
extern std::string creator;
extern std::string publisher;
extern std::string title;
extern std::string description;
extern std::string directoryPath;

std::map<std::string, unsigned int> counters;
char *data = NULL;
unsigned int dataSize = 0;



ArticleSource::ArticleSource() {
  /* Prepare metadata */
  metadataQueue.push("Language");
  metadataQueue.push("Publisher");
  metadataQueue.push("Creator");
  metadataQueue.push("Title");
  metadataQueue.push("Description");
  metadataQueue.push("Date");
  metadataQueue.push("Favicon");
  metadataQueue.push("Counter");
}

void ArticleSource::init_redirectsQueue_from_file(const std::string& path){
    std::ifstream in_stream;
    std::string line;

    in_stream.open(path.c_str());
    while (std::getline(in_stream, line)) {
      redirectsQueue.push(line);
    }
    in_stream.close();
}

std::string ArticleSource::getMainPage() {
  return welcome;
}

Article *article = NULL;
const zim::writer::Article* ArticleSource::getNextArticle() {
  std::string path;

  if (article != NULL) {
    delete(article);
  }

  if (!metadataQueue.empty()) {
    path = metadataQueue.front();
    metadataQueue.pop();
    article = new MetadataArticle(path);
  } else if (!redirectsQueue.empty()) {
    std::string line = redirectsQueue.front();
    redirectsQueue.pop();
    article = new RedirectArticle(line);
  } else if (popFromFilenameQueue(path)) {
    do {
      article = new Article(path);
    } while (article && article->isInvalid() && popFromFilenameQueue(path));
  } else {
    article = NULL;
  }

  /* Count mimetypes */
  if (article != NULL && !article->isRedirect()) {

    if (isVerbose())
      std::cout << "Creating entry for " << article->getAid() << std::endl;

    std::string mimeType = article->getMimeType();
    if (counters.find(mimeType) == counters.end()) {
      counters[mimeType] = 1;
    } else {
      counters[mimeType]++;
    }
  }

  return article;
}

zim::Blob ArticleSource::getData(const std::string& aid) {

  if (isVerbose())
    std::cout << "Packing data for " << aid << std::endl;

  if (data != NULL) {
    delete(data);
    data = NULL;
  }

  if (aid.substr(0, 3) == "/M/") {
    std::string value; 

    if ( aid == "/M/Language") {
      value = language;
    } else if (aid == "/M/Creator") {
      value = creator;
    } else if (aid == "/M/Publisher") {
      value = publisher;
    } else if (aid == "/M/Title") {
      value = title;
    } else if (aid == "/M/Description") {
      value = description;
    } else if ( aid == "/M/Date") {
      time_t t = time(0);
      struct tm * now = localtime( & t );
      std::stringstream stream;
      stream << (now->tm_year + 1900) << '-' 
	     << std::setw(2) << std::setfill('0') << (now->tm_mon + 1) << '-'
	     << std::setw(2) << std::setfill('0') << now->tm_mday;
      value = stream.str();
    } else if ( aid == "/M/Counter") {
      std::stringstream stream;
      for (std::map<std::string, unsigned int>::iterator it = counters.begin(); it != counters.end(); ++it) {
	stream << it->first << "=" << it->second << ";";
      }
      value = stream.str();
    }

    dataSize = value.length();
    data = new char[dataSize];
    memcpy(data, value.c_str(), dataSize);
  } else {
    std::string aidPath = directoryPath + "/" + aid;
    
    if (getMimeTypeForFile(aid).find("text/html") == 0) {
      std::string html = getFileContent(aidPath);
      
      /* Rewrite links (src|href|...) attributes */
      GumboOutput* output = gumbo_parse(html.c_str());
      GumboNode* root = output->root;

      std::map<std::string, bool> links;
      getLinks(root, links);
      std::map<std::string, bool>::iterator it;
      std::string aidDirectory = removeLastPathElement(aid, false, false);
      
      /* If a link appearch to be duplicated in the HTML, it will
	 occurs only one time in the links variable */
      for(it = links.begin(); it != links.end(); it++) {
	if (!it->first.empty() && it->first[0] != '#' && it->first[0] != '?' && it->first.substr(0, 5) != "data:") {
	  replaceStringInPlace(html, "\"" + it->first + "\"", "\"" + computeNewUrl(aid, it->first) + "\"");
	}
      }
      gumbo_destroy_output(&kGumboDefaultOptions, output);

      dataSize = html.length();
      data = new char[dataSize];
      memcpy(data, html.c_str(), dataSize);
    } else if (getMimeTypeForFile(aid).find("text/css") == 0) {
      std::string css = getFileContent(aidPath);

      /* Rewrite url() values in the CSS */
      size_t startPos = 0;
      size_t endPos = 0;
      std::string url;

      while ((startPos = css.find("url(", endPos)) && startPos != std::string::npos) {

	/* URL delimiters */
	endPos = css.find(")", startPos);
	startPos = startPos + (css[startPos+4] == '\'' || css[startPos+4] == '"' ? 5 : 4);
	endPos = endPos - (css[endPos-1] == '\'' || css[endPos-1] == '"' ? 1 : 0);
	url = css.substr(startPos, endPos - startPos);
	std::string startDelimiter = css.substr(startPos-1, 1);
	std::string endDelimiter = css.substr(endPos, 1);

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
	  if (mimeType == "application/font-ttf" || 
	      mimeType == "application/font-woff" || 
	      mimeType == "application/vnd.ms-opentype" ||
	      mimeType == "application/vnd.ms-fontobject") {

	    try {
	      std::string fontContent = getFileContent(directoryPath + "/" + computeAbsolutePath(aid, path));
	      replaceStringInPlaceOnce(css, 
				       startDelimiter + url + endDelimiter, 
				       startDelimiter + "data:" + mimeType + ";base64," + 
				       base64_encode(reinterpret_cast<const unsigned char*>(fontContent.c_str()), fontContent.length()) +
				       endDelimiter
				       );
	    } catch (...) {
	    }
	  } else {

	    /* Deal with URL with arguments (using '? ') */
	    if (markPos != std::string::npos) {
	      endDelimiter = url.substr(markPos, 1);
	    }

	    replaceStringInPlaceOnce(css,
				     startDelimiter + url + endDelimiter,
				     startDelimiter + computeNewUrl(aid, path) + endDelimiter);
	  }
	}
      }

      dataSize = css.length();
      data = new char[dataSize];
      memcpy(data, css.c_str(), dataSize);
    } else {
      dataSize = getFileSize(aidPath);
      data = new char[dataSize];
      memcpy(data, getFileContent(aidPath).c_str(), dataSize);
    }
  }

  return zim::Blob(data, dataSize);
}

