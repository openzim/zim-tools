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

/* Includes */
#include <getopt.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctime>

#include <magic.h>
#include <cstdio>
#include <queue>

#include "article.h"
#include "zimcreatorfs.h"
#include "mimetypecounter.h"
#include "queue.h"
#include "tools.h"
#include "config.h"

/* Check for version number */
#ifndef VERSION
  #define VERSION "UNKNOWN"
#endif

/* Global access strings */
std::string language;
std::string creator;
std::string publisher;
std::string title;
std::string tags;
std::string name;
std::string description;
std::string welcome;
std::string favicon;
std::string directoryPath;
std::string redirectsPath;
std::string zimPath;

bool verboseFlag = false;
pthread_mutex_t verboseMutex;
bool inflateHtmlFlag = false;
bool uniqueNamespace = false;
bool withFullTextIndex = false;

magic_t magic;

bool isVerbose()
{
  pthread_mutex_lock(&verboseMutex);
  bool retVal = verboseFlag;
  pthread_mutex_unlock(&verboseMutex);
  return retVal;
}

/* Print current version defined in the macro */
void version()
{
  // Access the version number through meson macro define
  std::cout << "Version:  " << "v" << VERSION << std::endl;
  std::cout << "\tSee -h or --help argument flags for further argument options" << std::endl;
  std::cout << "\tCopyright 2013-2016 Emmanuel Engelhart <kelson@kiwix.org>" << std::endl;

}

/* Print correct console usage options */
void usage()
{
  std::cout << "Usage: zimwriterfs [mandatory arguments] [optional arguments] "
               "HTML_DIRECTORY ZIM_FILE"
            << std::endl;
  std::cout << std::endl;

  std::cout << "Purpose:" << std::endl;
  std::cout << "\tPacking all files (HTML/JS/CSS/JPEG/WEBM/...) belonging to a "
               "directory in a ZIM file."
            << std::endl;
  std::cout << std::endl;

  std::cout << "Mandatory arguments:" << std::endl;
  std::cout << "\t-w, --welcome\t\tpath of default/main HTML page. The path "
               "must be relative to HTML_DIRECTORY."
            << std::endl;
  std::cout << "\t-f, --favicon\t\tpath of ZIM file favicon. The path must be "
               "relative to HTML_DIRECTORY and the image a 48x48 PNG."
            << std::endl;
  std::cout << "\t-l, --language\t\tlanguage code of the content in ISO639-3"
            << std::endl;
  std::cout << "\t-t, --title\t\ttitle of the ZIM file" << std::endl;
  std::cout << "\t-d, --description\tshort description of the content"
            << std::endl;
  std::cout << "\t-c, --creator\t\tcreator(s) of the content" << std::endl;
  std::cout << "\t-p, --publisher\t\tcreator of the ZIM file itself"
            << std::endl;
  std::cout << std::endl;
  std::cout << "\tHTML_DIRECTORY\t\tis the path of the directory containing "
               "the HTML pages you want to put in the ZIM file,"
            << std::endl;
  std::cout << "\tZIM_FILE\t\tis the path of the ZIM file you want to obtain."
            << std::endl;
  std::cout << std::endl;

  std::cout << "Optional arguments:" << std::endl;
  std::cout << "\t-v, --verbose\t\tprint processing details on STDOUT"
            << std::endl;
  std::cout << "\t-h, --help\t\tprint this help" << std::endl;
  std::cout << "\t-V, --version\t\tprint the version number" << std::endl;
  std::cout
      << "\t-m, --minChunkSize\tnumber of bytes per ZIM cluster (defaul: 2048)"
      << std::endl;
  std::cout << "\t-x, --inflateHtml\ttry to inflate HTML files before packing "
               "(*.html, *.htm, ...)"
            << std::endl;
  std::cout << "\t-u, --uniqueNamespace\tput everything in the same namespace "
               "'A'. Might be necessary to avoid problems with "
               "dynamic/javascript data loading."
            << std::endl;
  std::cout << "\t-r, --redirects\t\tpath to the TSV file with the list of "
               "redirects (namespace, url, title, target_url tab separated)."
            << std::endl;
  std::cout
      << "\t-i, --withFullTextIndex\tindex the content and add it to the ZIM."
      << std::endl;
  std::cout << "\t-a, --tags\t\ttags - semicolon separated" << std::endl;
  std::cout << "\t-n, --name\t\tcustom (version independent) identifier for "
               "the content"
            << std::endl;
  std::cout << std::endl;

  std::cout << "Example:" << std::endl;
  std::cout
      << "\tzimwriterfs --welcome=index.html --favicon=m/favicon.png --language=fra --title=foobar --description=mydescription \\\n\t\t\
--creator=Wikipedia --publisher=Kiwix ./my_project_html_directory my_project.zim"
      << std::endl;
  std::cout << std::endl;

  std::cout << "Documentation:" << std::endl;
  std::cout << "\tzimwriterfs source code: http://www.openzim.org/wiki/Git"
            << std::endl;
  std::cout << "\tZIM format: http://www.openzim.org/" << std::endl;
  std::cout << std::endl;
}

/* Main program entry point */
int main(int argc, char** argv)
{
  int minChunkSize = 2048;

  /* Argument parsing */
  static struct option long_options[]
      = {{"help", no_argument, 0, 'h'},
         {"verbose", no_argument, 0, 'v'},
         {"version", no_argument, 0, 'V'},
         {"welcome", required_argument, 0, 'w'},
         {"minchunksize", required_argument, 0, 'm'},
         {"name", required_argument, 0, 'n'},
         {"redirects", required_argument, 0, 'r'},
         {"inflateHtml", no_argument, 0, 'x'},
         {"uniqueNamespace", no_argument, 0, 'u'},
         {"favicon", required_argument, 0, 'f'},
         {"language", required_argument, 0, 'l'},
         {"title", required_argument, 0, 't'},
         {"tags", required_argument, 0, 'a'},
         {"description", required_argument, 0, 'd'},
         {"creator", required_argument, 0, 'c'},
         {"publisher", required_argument, 0, 'p'},
         {"withFullTextIndex", no_argument, 0, 'i'},
         {0, 0, 0, 0}};
  int option_index = 0;
  int c;

  do {
    c = getopt_long(
        argc, argv, "hVvixuw:m:f:t:d:c:l:p:r:", long_options, &option_index);

    if (c != -1) {
      switch (c) {
        case 'a':
          tags = optarg;
          break;
        case 'V':
          version();
          exit(0);
          break;
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
        case 'i':
          withFullTextIndex = true;
          break;
        case 'l':
          language = optarg;
          break;
        case 'm':
          minChunkSize = atoi(optarg);
          break;
        case 'n':
          name = optarg;
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

  if (directoryPath.empty() || zimPath.empty() || creator.empty()
      || publisher.empty()
      || description.empty()
      || language.empty()
      || welcome.empty()
      || favicon.empty()) {
    if (argc > 1)
      std::cerr << "zimwriterfs: too few arguments!" << std::endl;
    usage();
    exit(1);
  }

  /* Check arguments */
  if (directoryPath[directoryPath.length() - 1] == '/') {
    directoryPath = directoryPath.substr(0, directoryPath.length() - 1);
  }

  /* Check metadata */
  if (!fileExists(directoryPath + "/" + welcome)) {
    std::cerr << "zimwriterfs: unable to find welcome page at '"
              << directoryPath << "/" << welcome
              << "'. --welcome path/value must be relative to HTML_DIRECTORY."
              << std::endl;
    exit(1);
  }

  if (!fileExists(directoryPath + "/" + favicon)) {
    std::cerr << "zimwriterfs: unable to find favicon at " << directoryPath
              << "/" << favicon
              << "'. --favicon path/value must be relative to HTML_DIRECTORY."
              << std::endl;
    exit(1);
  }



  /* System tags */
  if (withFullTextIndex) {
    tags += tags.empty() ? "" : ";";
    tags += "_ftindex";

  }

  setenv("ZIM_LZMA_LEVEL", "9e", 1);
  ZimCreatorFS zimCreator(welcome, isVerbose());
  zimCreator.setMinChunkSize(minChunkSize);
  zimCreator.setIndexing(withFullTextIndex, language);
  zimCreator.startZimCreation(zimPath);

  zimCreator.addArticle(SimpleMetadataArticle("Language", language));
  zimCreator.addArticle(SimpleMetadataArticle("Publisher", publisher));
  zimCreator.addArticle(SimpleMetadataArticle("Creator", creator));
  zimCreator.addArticle(SimpleMetadataArticle("Title", title));
  zimCreator.addArticle(SimpleMetadataArticle("Description", description));
  zimCreator.addArticle(SimpleMetadataArticle("Name", name));
  zimCreator.addArticle(SimpleMetadataArticle("Tags", tags));
  zimCreator.addArticle(MetadataDateArticle());
  zimCreator.addArticle(MetadataFaviconArticle(favicon));

  /* Init */
  magic = magic_open(MAGIC_MIME);
  magic_load(magic, NULL);
  pthread_mutex_init(&verboseMutex, NULL);

  /* Directory visitor */
  MimetypeCounter mimetypeCounter;
  zimCreator.add_customHandler(&mimetypeCounter);
  zimCreator.visitDirectory(directoryPath);

  /* Check redirects file and read it if necessary*/
  if (!redirectsPath.empty() && !fileExists(redirectsPath)) {
    std::cerr << "zimwriterfs: unable to find redirects TSV file at '"
              << redirectsPath << "'. Verify --redirects path/value."
              << std::endl;
    exit(1);
  } else {
    if (isVerbose())
      std::cout << "Reading redirects TSV file " << redirectsPath << "..."
                << std::endl;

    zimCreator.add_redirectArticles_from_file(redirectsPath);
  }

  zimCreator.finishZimCreation();

  magic_close(magic);
  /* Destroy mutex */
  pthread_mutex_destroy(&verboseMutex);
}
