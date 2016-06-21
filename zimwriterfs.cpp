/*
 * Copyright 2013-2016 Emmanuel Engelhart <kelson@kiwix.org>
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

#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>
#include <ctime>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <pthread.h>

#include <iomanip>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <queue>
#include <map>
#include <cstdio>
#include <cerrno>
#include <stdexcept>

#include <magic.h>

#include <zlib.h>

#include <zim/writer/zimcreator.h>
#include <zim/blob.h>

#include <gumbo.h>

#define MAX_QUEUE_SIZE 100

#ifdef _WIN32
#define SEPARATOR "\\"
#else
#define SEPARATOR "/"
#endif

std::string language;
std::string creator;
std::string publisher;
std::string title;
std::string description;
std::string welcome;
std::string favicon; 
std::string directoryPath;
std::string redirectsPath;
std::string zimPath;
zim::writer::ZimCreator zimCreator;
pthread_t directoryVisitor;
pthread_mutex_t filenameQueueMutex;
std::queue<std::string> filenameQueue;
std::queue<std::string> metadataQueue;
std::queue<std::string> redirectsQueue;

bool isDirectoryVisitorRunningFlag = false;
pthread_mutex_t directoryVisitorRunningMutex;
bool verboseFlag = false;
pthread_mutex_t verboseMutex;
bool inflateHtmlFlag = false;
bool uniqueNamespace = false;

magic_t magic;
std::map<std::string, unsigned int> counters;
std::map<std::string, std::string> fileMimeTypes;
std::map<std::string, std::string> extMimeTypes;
char *data = NULL;
unsigned int dataSize = 0;

/* Decompress an STL string using zlib and return the original data. */
inline std::string inflateString(const std::string& str) {
  z_stream zs; // z_stream is zlib's control structure
  memset(&zs, 0, sizeof(zs));

  if (inflateInit(&zs) != Z_OK)
    throw(std::runtime_error("inflateInit failed while decompressing."));

  zs.next_in = (Bytef*)str.data();
  zs.avail_in = str.size();

  int ret;
  char outbuffer[32768];
  std::string outstring;

  // get the decompressed bytes blockwise using repeated calls to inflate
  do {
    zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
    zs.avail_out = sizeof(outbuffer);

    ret = inflate(&zs, 0);

    if (outstring.size() < zs.total_out) {
      outstring.append(outbuffer,
		       zs.total_out - outstring.size());
    }

  } while (ret == Z_OK);

  inflateEnd(&zs);

  if (ret != Z_STREAM_END) { // an error occurred that was not EOF
    std::ostringstream oss;
    oss << "Exception during zlib decompression: (" << ret << ") "
	<< zs.msg;
    throw(std::runtime_error(oss.str()));
  }

  return outstring;
}

inline bool seemsToBeHtml(const std::string &path) {
  if (path.find_last_of(".") != std::string::npos) {
    std::string mimeType = path.substr(path.find_last_of(".")+1);
    if (extMimeTypes.find(mimeType) != extMimeTypes.end()) {
      return "text/html" == extMimeTypes[mimeType];
    }
  }

  return false;
}

inline std::string getFileContent(const std::string &path) {
  std::ifstream in(path.c_str(), ::std::ios::binary);
  if (in) {
    std::string contents;
    in.seekg(0, std::ios::end);
    contents.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(&contents[0], contents.size());
    in.close();

    /* Inflate if necessary */
    if (inflateHtmlFlag && seemsToBeHtml(path)) {
      try {
	contents = inflateString(contents);
      } catch(...) {
	std::cerr << "Can not initialize inflate stream for: " << path << std::endl;
      }
    }
    return(contents);
 }
  std::cerr << "zimwriterfs: unable to open file at path: " << path << std::endl;
  throw(errno);
}

inline unsigned int getFileSize(const std::string &path) {
  struct stat filestatus;
  stat(path.c_str(), &filestatus);
  return filestatus.st_size;
}    

inline bool fileExists(const std::string &path) {
  bool flag = false;
  std::fstream fin;
  fin.open(path.c_str(), std::ios::in);
  if (fin.is_open()) {
    flag = true;
  }
  fin.close();
  return flag;
}

/* base64 */
static const std::string base64_chars = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
  "0123456789+/";

std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len) {
  std::string ret;
  int i = 0;
  int j = 0;
  unsigned char char_array_3[3];
  unsigned char char_array_4[4];

  while (in_len--) {
    char_array_3[i++] = *(bytes_to_encode++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for(i = 0; (i <4) ; i++)
        ret += base64_chars[char_array_4[i]];
      i = 0;
    }
  }

  if (i)
    {
      for(j = i; j < 3; j++)
	char_array_3[j] = '\0';

      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for (j = 0; (j < i + 1); j++)
	ret += base64_chars[char_array_4[j]];

      while((i++ < 3))
	ret += '=';

    }

  return ret;

}

inline std::string decodeUrl(const std::string &encodedUrl) {
  std::string decodedUrl = encodedUrl;
  std::string::size_type pos = 0;
  char ch;

  while ((pos = decodedUrl.find('%', pos)) != std::string::npos &&
	 pos + 2 < decodedUrl.length()) {
    sscanf(decodedUrl.substr(pos + 1, 2).c_str(), "%x", (unsigned int*)&ch);
    decodedUrl.replace(pos, 3, 1, ch);
    ++pos;
  }

  return decodedUrl;
}

inline std::string removeLastPathElement(const std::string path, const bool removePreSeparator, const bool removePostSeparator) {
  std::string newPath = path;
  size_t offset = newPath.find_last_of(SEPARATOR);
 
  if (removePreSeparator && offset == newPath.length()-1) {
    newPath = newPath.substr(0, offset);
    offset = newPath.find_last_of(SEPARATOR);
  }
  newPath = removePostSeparator ? newPath.substr(0, offset) : newPath.substr(0, offset+1);

  return newPath;
}

/* Split string in a token array */
std::vector<std::string> split(const std::string & str,
                                      const std::string & delims=" *-")
{
  std::string::size_type lastPos = str.find_first_not_of(delims, 0);
  std::string::size_type pos = str.find_first_of(delims, lastPos);
  std::vector<std::string> tokens;

  while (std::string::npos != pos || std::string::npos != lastPos)
    {
      tokens.push_back(str.substr(lastPos, pos - lastPos));
      lastPos = str.find_first_not_of(delims, pos);
      pos     = str.find_first_of(delims, lastPos);
    }

  return tokens;
}

std::vector<std::string> split(const char* lhs, const char* rhs){
  const std::string m1 (lhs), m2 (rhs);
  return split(m1, m2);
}

std::vector<std::string> split(const char* lhs, const std::string& rhs){
  return split(lhs, rhs.c_str());
}

std::vector<std::string> split(const std::string& lhs, const char* rhs){
  return split(lhs.c_str(), rhs);
}

/* Warning: the relative path must be with slashes */
inline std::string computeAbsolutePath(const std::string path, const std::string relativePath) {

  /* Add a trailing / to the path if necessary */
  std::string absolutePath = path[path.length()-1] == '/' ? path : removeLastPathElement(path, false, false);

  /* Go through relative path */
  std::vector<std::string> relativePathElements;
  std::stringstream relativePathStream(relativePath);
  std::string relativePathItem;
  while (std::getline(relativePathStream, relativePathItem, '/')) {
    if (relativePathItem == "..") {
      absolutePath = removeLastPathElement(absolutePath, true, false); 
    } else if (!relativePathItem.empty() && relativePathItem != ".") {
      absolutePath += relativePathItem;
      absolutePath += "/";
    }
  }
  
  /* Remove wront trailing / */
  return absolutePath.substr(0, absolutePath.length()-1);
}

/* Warning: the relative path must be with slashes */
std::string computeRelativePath(const std::string path, const std::string absolutePath) {
  std::vector<std::string> pathParts = split(path, "/");
  std::vector<std::string> absolutePathParts = split(absolutePath, "/");

  unsigned int commonCount = 0;
  while (commonCount < pathParts.size() && 
	 commonCount < absolutePathParts.size() && 
	 pathParts[commonCount] == absolutePathParts[commonCount]) {
    if (!pathParts[commonCount].empty()) {
      commonCount++;
    }
  }
    
  std::string relativePath;
  for (unsigned int i = commonCount ; i < pathParts.size()-1 ; i++) {
    relativePath += "../";
  }

  for (unsigned int i = commonCount ; i < absolutePathParts.size() ; i++) {
    relativePath += absolutePathParts[i];
    relativePath += i + 1 < absolutePathParts.size() ? "/" : "";
  }

  return relativePath;
}

void directoryVisitorRunning(bool value) {
  pthread_mutex_lock(&directoryVisitorRunningMutex);
  isDirectoryVisitorRunningFlag = value;
  pthread_mutex_unlock(&directoryVisitorRunningMutex); 
}

bool isDirectoryVisitorRunning() {
  pthread_mutex_lock(&directoryVisitorRunningMutex);
  bool retVal = isDirectoryVisitorRunningFlag;
  pthread_mutex_unlock(&directoryVisitorRunningMutex); 
  return retVal;
}

bool isVerbose() {
  pthread_mutex_lock(&verboseMutex);
  bool retVal = verboseFlag;
  pthread_mutex_unlock(&verboseMutex); 
  return retVal;
}

bool isFilenameQueueEmpty() {
  pthread_mutex_lock(&filenameQueueMutex);
  bool retVal = filenameQueue.empty();
  pthread_mutex_unlock(&filenameQueueMutex);
  return retVal;
}

void pushToFilenameQueue(const std::string &filename) {
  unsigned int wait = 0;
  unsigned int queueSize = 0;

  do {
    usleep(wait);
    pthread_mutex_lock(&filenameQueueMutex);
    unsigned queueSize = filenameQueue.size();
    pthread_mutex_unlock(&filenameQueueMutex);
    wait += 10;
  } while (queueSize > MAX_QUEUE_SIZE);

  pthread_mutex_lock(&filenameQueueMutex);
  filenameQueue.push(filename);
  pthread_mutex_unlock(&filenameQueueMutex); 
}

bool popFromFilenameQueue(std::string &filename) {
  bool retVal = false;
  unsigned int wait = 0;

  do {
    usleep(wait);
    if (!isFilenameQueueEmpty()) {
      pthread_mutex_lock(&filenameQueueMutex);
      filename = filenameQueue.front();
      filenameQueue.pop();
      pthread_mutex_unlock(&filenameQueueMutex);
      retVal = true;
      break;
    } else {
      wait += 10;
    }
  } while (isDirectoryVisitorRunning() || !isFilenameQueueEmpty());

  return retVal;
}

/* Article class */
class Article : public zim::writer::Article {
  protected:
    char ns;
    bool invalid;
    std::string aid;
    std::string url;
    std::string title;
    std::string mimeType;
    std::string redirectAid;
    std::string data;

  public:
    Article() {
      invalid = false;
    }
    explicit Article(const std::string& id, const bool detectRedirects);
    virtual std::string getAid() const;
    virtual char getNamespace() const;
    virtual std::string getUrl() const;
    virtual bool isInvalid() const;
    virtual std::string getTitle() const;
    virtual bool isRedirect() const;
    virtual std::string getMimeType() const;
    virtual std::string getRedirectAid() const;
    virtual bool shouldCompress() const;
};

class MetadataArticle : public Article {
  public:
  MetadataArticle(std::string &id) {
    if (id == "Favicon") {
      aid = "/-/" + id;
      mimeType="image/png";
      redirectAid = favicon;
      ns = '-';
      url = "favicon";
    } else {
      aid = "/M/" + id;
      mimeType="text/plain";
      ns = 'M';
      url = id;
    }
  }
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
};

static bool isLocalUrl(const std::string url) {
  if (url.find(":") != std::string::npos) {
    return (!(
	      url.find("://") != std::string::npos ||
	      url.find("//") == 0 ||
	      url.find("tel:") == 0 ||
	      url.find("geo:") == 0
	      ));
  }
  return true;
}

static std::string extractRedirectUrlFromHtml(const GumboVector* head_children) {
  std::string url;
  
  for (int i = 0; i < head_children->length; ++i) {
    GumboNode* child = (GumboNode*)(head_children->data[i]);
    if (child->type == GUMBO_NODE_ELEMENT &&
	child->v.element.tag == GUMBO_TAG_META) {
      GumboAttribute* attribute;
      if (attribute = gumbo_get_attribute(&child->v.element.attributes, "http-equiv")) {
	if (!strcmp(attribute->value, "refresh")) {
	  if (attribute = gumbo_get_attribute(&child->v.element.attributes, "content")) {
	    std::string targetUrl = attribute->value;
	    std::size_t found = targetUrl.find("URL=") != std::string::npos ? targetUrl.find("URL=") : targetUrl.find("url=");
	    if (found!=std::string::npos) {
	      url = targetUrl.substr(found+4);
	    } else {
	      throw std::string("Unable to find the redirect/refresh target url from the HTML DOM");
	    }
	  }
	}
      }
    }
  }
  
  return url;
}

static void getLinks(GumboNode* node, std::map<std::string, bool> &links) {
  if (node->type != GUMBO_NODE_ELEMENT) {
    return;
  }

  GumboAttribute* attribute = NULL;
  attribute = gumbo_get_attribute(&node->v.element.attributes, "href");
  if (attribute == NULL) {
    attribute = gumbo_get_attribute(&node->v.element.attributes, "src");
  }

  if (attribute != NULL && isLocalUrl(attribute->value)) {
    links[attribute->value] = true;
  }

  GumboVector* children = &node->v.element.children;
  for (int i = 0; i < children->length; ++i) {
    getLinks(static_cast<GumboNode*>(children->data[i]), links);
  }
}

static void replaceStringInPlaceOnce(std::string& subject, const std::string& search,
				 const std::string& replace) {
  size_t pos = 0;
  while ((pos = subject.find(search, pos)) != std::string::npos) {
    subject.replace(pos, search.length(), replace);
    pos += replace.length();
    return; /* Do it once */
  }
}

static void replaceStringInPlace(std::string& subject, const std::string& search,
				 const std::string& replace) {
  size_t pos = 0;
  while ((pos = subject.find(search, pos)) != std::string::npos) {
    subject.replace(pos, search.length(), replace);
    pos += replace.length();
  }

  return;
}

static std::string getMimeTypeForFile(const std::string& filename) {
  std::string mimeType;

  /* Try to get the mimeType from the file extension */
  if (filename.find_last_of(".") != std::string::npos) {
    mimeType = filename.substr(filename.find_last_of(".")+1);
    if (extMimeTypes.find(mimeType) != extMimeTypes.end()) {
      return extMimeTypes[mimeType];
    }
  }

  /* Try to get the mimeType from the cache */
  if (fileMimeTypes.find(filename) != fileMimeTypes.end()) {
    return fileMimeTypes[filename];
  }

  /* Try to get the mimeType with libmagic */
  try {
    std::string path = directoryPath + "/" + filename;
    mimeType = std::string(magic_file(magic, path.c_str()));
    if (mimeType.find(";") != std::string::npos) {
      mimeType = mimeType.substr(0, mimeType.find(";"));
    }
    fileMimeTypes[filename] = mimeType;
    return mimeType;
  } catch (...) {
    return "";
  }
}

inline std::string getNamespaceForMimeType(const std::string& mimeType) {
  if (uniqueNamespace || mimeType.find("text") == 0 || mimeType.empty()) {
    if (uniqueNamespace || mimeType.find("text/html") == 0 || mimeType.empty()) {
      return "A";
    } else {
      return "-";
    }
  } else {
    if (mimeType == "application/font-ttf" || 
	mimeType == "application/font-woff" ||
	mimeType == "application/vnd.ms-opentype" ||
	mimeType == "application/vnd.ms-fontobject" ||
	mimeType == "application/javascript" ||
	mimeType == "application/json"
	) {
      return "-";
    } else {
      return "I";
    }
  }
}

inline std::string removeLocalTagAndParameters(const std::string &url) {
  std::string retVal = url;
  std::size_t found;

  /* Remove URL arguments */
  found = retVal.find("?");
  if (found != std::string::npos) {
    retVal = retVal.substr(0, found);
  }

  /* Remove local tag */
  found = retVal.find("#");
  if (found != std::string::npos) {
    retVal = retVal.substr(0, found);
  }

  return retVal;
}

inline std::string computeNewUrl(const std::string &aid, const std::string &url) {
  std::string filename = computeAbsolutePath(aid, url);
  std::string targetMimeType = getMimeTypeForFile(removeLocalTagAndParameters(decodeUrl(filename)));
  std::string originMimeType = getMimeTypeForFile(aid);
  std::string newUrl = "/" + getNamespaceForMimeType(targetMimeType) + "/" + filename;
  std::string baseUrl = "/" + getNamespaceForMimeType(originMimeType) + "/" + aid;
  return computeRelativePath(baseUrl, newUrl);
}

Article::Article(const std::string& path, const bool detectRedirects = true) {
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

/* ArticleSource class */
class ArticleSource : public zim::writer::ArticleSource {
  public:
    explicit ArticleSource();
    virtual const zim::writer::Article* getNextArticle();
    virtual zim::Blob getData(const std::string& aid);
    virtual std::string getMainPage();
};

ArticleSource::ArticleSource() {
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

/* Non ZIM related code */
void usage() {
  std::cout << "Usage: zimwriterfs [mandatory arguments] [optional arguments] HTML_DIRECTORY ZIM_FILE" << std::endl;
  std::cout << std::endl;

  std::cout << "Purpose:" << std::endl;
  std::cout << "\tPacking all files (HTML/JS/CSS/JPEG/WEBM/...) belonging to a directory in a ZIM file." << std::endl;
  std::cout << std::endl;

  std::cout << "Mandatory arguments:" << std::endl;
  std::cout << "\t-w, --welcome\t\tpath of default/main HTML page. The path must be relative to HTML_DIRECTORY." << std::endl;
  std::cout << "\t-f, --favicon\t\tpath of ZIM file favicon. The path must be relative to HTML_DIRECTORY and the image a 48x48 PNG." << std::endl;
  std::cout << "\t-l, --language\t\tlanguage code of the content in ISO639-3" << std::endl;
  std::cout << "\t-t, --title\t\ttitle of the ZIM file" << std::endl;
  std::cout << "\t-d, --description\tshort description of the content" << std::endl;
  std::cout << "\t-c, --creator\t\tcreator(s) of the content" << std::endl;
  std::cout << "\t-p, --publisher\t\tcreator of the ZIM file itself" << std::endl;
  std::cout << std::endl;
  std::cout << "\tHTML_DIRECTORY\t\tis the path of the directory containing the HTML pages you want to put in the ZIM file," << std::endl;
  std::cout << "\tZIM_FILE\t\tis the path of the ZIM file you want to obtain." << std::endl;
  std::cout << std::endl;

  std::cout << "Optional arguments:" << std::endl;
  std::cout << "\t-v, --verbose\t\tprint processing details on STDOUT" << std::endl;
  std::cout << "\t-h, --help\t\tprint this help" << std::endl;
  std::cout << "\t-m, --minChunkSize\tnumber of bytes per ZIM cluster (defaul: 2048)" << std::endl;
  std::cout << "\t-x, --inflateHtml\ttry to inflate HTML files before packing (*.html, *.htm, ...)" << std::endl;
  std::cout << "\t-u, --uniqueNamespace\tput everything in the same namespace 'A'. Might be necessary to avoid problems with dynamic/javascript data loading." << std::endl;
  std::cout << "\t-r, --redirects\t\tpath to the CSV file with the list of redirects (url, title, target_url tab separated)." << std::endl;
  std::cout << std::endl;
 
   std::cout << "Example:" << std::endl;
  std::cout << "\tzimwriterfs --welcome=index.html --favicon=m/favicon.png --language=fra --title=foobar --description=mydescription \\\n\t\t\
--creator=Wikipedia --publisher=Kiwix ./my_project_html_directory my_project.zim" << std::endl;
  std::cout << std::endl;

  std::cout << "Documentation:" << std::endl;
  std::cout << "\tzimwriterfs source code: http://www.openzim.org/wiki/Git" << std::endl;
  std::cout << "\tZIM format: http://www.openzim.org/" << std::endl;
  std::cout << std::endl;
}

void *visitDirectory(const std::string &path) {

  if (isVerbose())
    std::cout << "Visiting directory " << path << std::endl;

  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
  DIR *directory;

  /* Open directory */
  directory = opendir(path.c_str());
  if (directory == NULL) {
    std::cerr << "zimwriterfs: unable to open directory " << path << std::endl;
    exit(1);
  }

  /* Read directory content */
  struct dirent *entry;
  while (entry = readdir(directory)) {
    std::string entryName = entry->d_name;

    /* Ignore this system navigation virtual directories */
    if (entryName != "." && entryName != "..") {
      std::string fullEntryName = path + '/' + entryName;

      switch (entry->d_type) {
      case DT_REG:
	pushToFilenameQueue(fullEntryName);
	break;
      case DT_DIR:
	visitDirectory(fullEntryName);
	break;
      case DT_BLK:
	std::cerr << "Unable to deal with " << fullEntryName << " (this is a block device)" << std::endl;
	break;
      case DT_CHR:
	std::cerr << "Unable to deal with " << fullEntryName << " (this is a character device)" << std::endl;
	break;
      case DT_FIFO:
	std::cerr << "Unable to deal with " << fullEntryName << " (this is a named pipe)" << std::endl;
	break;
      case DT_LNK:
	pushToFilenameQueue(fullEntryName);
	break;
      case DT_SOCK:
	std::cerr << "Unable to deal with " << fullEntryName << " (this is a UNIX domain socket)" << std::endl;
	break;
      case DT_UNKNOWN:
	struct stat s;
	if (stat(fullEntryName.c_str(), &s) == 0) {
	  if (S_ISREG(s.st_mode)) {
	    pushToFilenameQueue(fullEntryName);
	  } else if (S_ISDIR(s.st_mode)) {
	    visitDirectory(fullEntryName);
          } else {
	    std::cerr << "Unable to deal with " << fullEntryName << " (no clue what kind of file it is - from stat())" << std::endl;
	  }
	} else {
	  std::cerr << "Unable to stat " << fullEntryName << std::endl;
	}
	break;
      default:
	std::cerr << "Unable to deal with " << fullEntryName << " (no clue what kind of file it is)" << std::endl;
	break;
      }
    }
  }
  
  closedir(directory);
}

void *visitDirectoryPath(void *path) {
  visitDirectory(directoryPath);

  if (isVerbose())
    std::cout << "Quitting visitor" << std::endl;

  directoryVisitorRunning(false); 
  pthread_exit(NULL);
}

int main(int argc, char** argv) {
  ArticleSource source;
  int minChunkSize = 2048;

  /* Init file extensions hash */
  extMimeTypes["HTML"] = "text/html";
  extMimeTypes["html"] = "text/html";
  extMimeTypes["HTM"] = "text/html";
  extMimeTypes["htm"] = "text/html";
  extMimeTypes["PNG"] = "image/png";
  extMimeTypes["png"] = "image/png";
  extMimeTypes["TIFF"] = "image/tiff";
  extMimeTypes["tiff"] = "image/tiff";
  extMimeTypes["TIF"] = "image/tiff";
  extMimeTypes["tif"] = "image/tiff";
  extMimeTypes["JPEG"] = "image/jpeg";
  extMimeTypes["jpeg"] = "image/jpeg";
  extMimeTypes["JPG"] = "image/jpeg";
  extMimeTypes["jpg"] = "image/jpeg";
  extMimeTypes["GIF"] = "image/gif";
  extMimeTypes["gif"] = "image/gif";
  extMimeTypes["SVG"] = "image/svg+xml";
  extMimeTypes["svg"] = "image/svg+xml";
  extMimeTypes["TXT"] = "text/plain";
  extMimeTypes["txt"] = "text/plain";
  extMimeTypes["XML"] = "text/xml";
  extMimeTypes["xml"] = "text/xml";
  extMimeTypes["EPUB"] = "application/epub+zip";
  extMimeTypes["epub"] = "application/epub+zip";
  extMimeTypes["PDF"] = "application/pdf";
  extMimeTypes["pdf"] = "application/pdf";
  extMimeTypes["OGG"] = "application/ogg";
  extMimeTypes["ogg"] = "application/ogg";
  extMimeTypes["JS"] = "application/javascript";
  extMimeTypes["js"] = "application/javascript";
  extMimeTypes["JSON"] = "application/json";
  extMimeTypes["json"] = "application/json";
  extMimeTypes["CSS"] = "text/css";
  extMimeTypes["css"] = "text/css";
  extMimeTypes["otf"] = "application/vnd.ms-opentype";
  extMimeTypes["OTF"] = "application/vnd.ms-opentype";
  extMimeTypes["eot"] = "application/vnd.ms-fontobject";
  extMimeTypes["EOT"] = "application/vnd.ms-fontobject";
  extMimeTypes["ttf"] = "application/font-ttf";
  extMimeTypes["TTF"] = "application/font-ttf";
  extMimeTypes["woff"] = "application/font-woff";
  extMimeTypes["WOFF"] = "application/font-woff";
  extMimeTypes["vtt"] = "text/vtt";
  extMimeTypes["VTT"] = "text/vtt";

  /* Argument parsing */
  static struct option long_options[] = {
    {"help", no_argument, 0, 'h'},
    {"verbose", no_argument, 0, 'v'},
    {"welcome", required_argument, 0, 'w'},
    {"minchunksize", required_argument, 0, 'm'},
    {"redirects", required_argument, 0, 'r'},
    {"inflateHtml", no_argument, 0, 'x'},
    {"uniqueNamespace", no_argument, 0, 'u'},
    {"favicon", required_argument, 0, 'f'},
    {"language", required_argument, 0, 'l'},
    {"title", required_argument, 0, 't'},
    {"description", required_argument, 0, 'd'},
    {"creator", required_argument, 0, 'c'},
    {"publisher", required_argument, 0, 'p'},
    {0, 0, 0, 0}
  };
  int option_index = 0;
  int c;

  do { 
    c = getopt_long(argc, argv, "hvxuw:m:f:t:d:c:l:p:r:", long_options, &option_index);
    
    if (c != -1) {
      switch (c) {
      case 'h':
	usage();
	exit(0);	
	break;
      case 'v':
	verboseFlag = true;
	break;
      case 'x':
	inflateHtmlFlag = true;
	break;
      case 'c':
	creator = optarg;
	break;
      case 'd':
	description = optarg;
	break;
      case 'f':
	favicon = optarg;
	break;
      case 'l':
	language = optarg;
	break;
      case 'm':
	minChunkSize = atoi(optarg);
	break;
      case 'p':
	publisher = optarg;
	break;
      case 'r':
	redirectsPath = optarg;
	break;
      case 't':
	title = optarg;
	break;
      case 'u':
	uniqueNamespace = true;
	break;
      case 'w':
	welcome = optarg;
	break;
      }
    }
  } while (c != -1);

  while (optind < argc) {
    if (directoryPath.empty()) {
      directoryPath = argv[optind++];
    } else if (zimPath.empty()) {
      zimPath = argv[optind++];
    } else {
      break;
    }
  }
  
  if (directoryPath.empty() || zimPath.empty() || creator.empty() || publisher.empty() || description.empty() || language.empty() || welcome.empty() || favicon.empty()) {
    if (argc > 1)
      std::cerr << "zimwriterfs: too few arguments!" << std::endl;
    usage();
    exit(1);
  }

  /* Check arguments */
  if (directoryPath[directoryPath.length()-1] == '/') {
    directoryPath = directoryPath.substr(0, directoryPath.length()-1);
  }

  /* Prepare metadata */
  metadataQueue.push("Language");
  metadataQueue.push("Publisher");
  metadataQueue.push("Creator");
  metadataQueue.push("Title");
  metadataQueue.push("Description");
  metadataQueue.push("Date");
  metadataQueue.push("Favicon");
  metadataQueue.push("Counter");

  /* Check metadata */
  if (!fileExists(directoryPath + "/" + welcome)) {
    std::cerr << "zimwriterfs: unable to find welcome page at '" << directoryPath << "/" << welcome << "'. --welcome path/value must be relative to HTML_DIRECTORY." << std::endl;
    exit(1);
  }

  if (!fileExists(directoryPath + "/" + favicon)) {
    std::cerr << "zimwriterfs: unable to find favicon at " << directoryPath << "/" << favicon << "'. --favicon path/value must be relative to HTML_DIRECTORY." << std::endl;
    exit(1);
  }

  /* Check redirects file and read it if necessary*/
  if (!redirectsPath.empty() && !fileExists(redirectsPath)) {
    std::cerr << "zimwriterfs: unable to find redirects CSV file at '" << redirectsPath << "'. Verify --redirects path/value." << std::endl;
    exit(1);
  } else {
    if (isVerbose())
      std::cout << "Reading redirects CSV file " << redirectsPath << "..." << std::endl;

    std::ifstream in_stream;
    std::string line;

    in_stream.open(redirectsPath.c_str());
    while (std::getline(in_stream, line)) {
      redirectsQueue.push(line);
    }
    in_stream.close();
  }

  /* Init */
  magic = magic_open(MAGIC_MIME);
  magic_load(magic, NULL);
  pthread_mutex_init(&filenameQueueMutex, NULL);
  pthread_mutex_init(&directoryVisitorRunningMutex, NULL);
  pthread_mutex_init(&verboseMutex, NULL);

  /* Directory visitor */
  directoryVisitorRunning(true);
  pthread_create(&(directoryVisitor), NULL, visitDirectoryPath, (void*)NULL);
  pthread_detach(directoryVisitor);

  /* ZIM creation */
  setenv("ZIM_LZMA_LEVEL", "9e", 1);
  try {
    zimCreator.setMinChunkSize(minChunkSize);
    zimCreator.create(zimPath, source);
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }

  /* Destroy mutex */
  pthread_mutex_destroy(&directoryVisitorRunningMutex);
  pthread_mutex_destroy(&verboseMutex);
  pthread_mutex_destroy(&filenameQueueMutex);
}
