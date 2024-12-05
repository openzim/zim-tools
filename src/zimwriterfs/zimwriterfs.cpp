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
#include <sys/param.h>
#include <unistd.h>
#include <libgen.h>
#include <limits.h>
#include <ctime>

#include <magic.h>
#include <cstdio>
#include <queue>

#include "zimcreatorfs.h"
#include "../metadata.h"
#include "../tools.h"
#include "../version.h"
#include "tools.h"

namespace {
/* Command line options */
std::string language;
std::string creator;
std::string publisher;
std::string title;
std::string tags;
std::string flavour;
std::string scraper = "zimwriterfs-" VERSION;
std::string name;
std::string source;
std::string description;
std::string longDescription;
std::string welcome;
std::string illustration;
std::string redirectsPath;
std::string zimPath;
std::string directoryPath;


int threads = 4;
zim::size_type clusterSize = 2048*1024;

bool verboseFlag = false;
bool withoutFTIndex = false;
bool noUuid = false;
bool dontCheckArgs = false;
bool continue_without_magic = false;

bool thereAreMissingArguments()
{
  if ( directoryPath.empty() || zimPath.empty() )
    return true;

  if ( dontCheckArgs )
    return false;

  return creator.empty()
      || name.empty()
      || publisher.empty()
      || description.empty()
      || language.empty()
      || welcome.empty()
      || title.empty()
      || illustration.empty();
}

zim::Metadata makeMetadata() {
  zim::Metadata metadata;

  metadata.set("Language",        language);
  metadata.set("Publisher",       publisher);
  metadata.set("Creator",         creator);
  metadata.set("Title",           title);
  metadata.set("Description",     description);
  metadata.set("LongDescription", longDescription);
  metadata.set("Name",            name);
  metadata.set("Source",          source);
  metadata.set("Flavour",         flavour);
  metadata.set("Scraper",         scraper);
  metadata.set("Tags",            tags);
  metadata.set("Date",            generateDate());
  if ( !illustration.empty() )  {
    const auto data = getFileContent(directoryPath + "/" + illustration);
    metadata.set("Illustration_48x48@1", data);
  }

  return metadata;
}


bool checkMetadata(const zim::Metadata& metadata)
{
  const auto errors = metadata.check();

  if ( !errors.empty() ) {
    std::cerr << "Metadata doesn't meet the following requirements:\n";
    for ( const auto& err : errors ) {
      std::cerr << "    " << err << std::endl;
    }
  }

  return errors.empty();
}

void addMetadata(ZimCreatorFS& zimCreator, const zim::Metadata& metadata)
{
  for ( const auto& kv : metadata ) {
    if (kv.first == "Illustration_48x48@1") {
      zimCreator.addIllustration(48, kv.second);
    } else {
      zimCreator.addMetadata(kv.first, kv.second);
    }
  }
}


}

// Global flags
bool inflateHtmlFlag = false;

pthread_mutex_t verboseMutex;

magic_t magic;

bool isVerbose()
{
  pthread_mutex_lock(&verboseMutex);
  bool retVal = verboseFlag;
  pthread_mutex_unlock(&verboseMutex);
  return retVal;
}

void printZimWriterFsVersions(std::ostream& out = std::cout) {
  out << "zim-tools " << VERSION << std::endl;
  out << "+ libmagic " << magic_version() << std::endl;
  out << std::endl;
  zim::printVersions(out);
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
  std::cout << "\t-I, --illustration\tpath of ZIM file illustration. The path must be "
               "relative to HTML_DIRECTORY and the image a 48x48 PNG."
            << std::endl;
  std::cout << "\t-l, --language\t\tlanguage code of the content in ISO639-3"
            << std::endl;
  std::cout << "\t-n, --name\t\tcustom (version independent) identifier for "
               "the content"
            << std::endl;
  std::cout << "\t-t, --title\t\ttitle of the ZIM file" << std::endl;
  std::cout << "\t-d, --description\tshort description of the content"
            << std::endl;
  std::cout << "\t-c, --creator\t\tcreator(s) of the content" << std::endl;
  std::cout << "\t-p, --publisher\t\tcreator of the ZIM file itself"
            << std::endl;
  std::cout << std::endl;
  std::cout << "\tHTML_DIRECTORY\t\tpath of the directory containing "
               "the HTML pages you want to put in the ZIM file."
            << std::endl;
  std::cout << "\tZIM_FILE\t\tpath of the ZIM file you want to obtain."
            << std::endl;
  std::cout << std::endl;

  std::cout << "Optional arguments:" << std::endl;
  std::cout << "\t-v, --verbose\t\tprint processing details on STDOUT"
            << std::endl;
  std::cout << "\t-h, --help\t\tprint this help" << std::endl;
  std::cout << "\t-V, --version\t\tprint the version number" << std::endl;
  std::cout << "\t-L, --longDescription\tlong description of the content"
            << std::endl;
  std::cout
      << "\t-m, --clusterSize\tnumber of bytes per ZIM cluster (default: 2048Kb)"
      << std::endl;
  std::cout << "\t-J, --threads\t\tcount of threads to utilize (default: 4)"
      << std::endl;
  std::cout << "\t-x, --inflateHtml\ttry to inflate HTML files before packing "
               "(*.html, *.htm, ...)"
            << std::endl;
  std::cout << "\t-r, --redirects\t\tpath to a TSV file containing a list of "
               "redirects (url title target_url)."
            << std::endl;
  std::cout
      << "\t-j, --withoutFTIndex\tdon't create and add a fulltext index of the content to the ZIM."
      << std::endl;
  std::cout << "\t-a, --tags\t\ttags - semicolon separated" << std::endl;
  std::cout << "\t-e, --source\t\tcontent source URL" << std::endl;
  std::cout << "\t-o, --flavour\t\tcustom (version independent) content flavour"
            << std::endl;
  std::cout << "\t-s, --scraper\t\tname & version of tool used to produce HTML content"
            << std::endl;
  std::cout << "\t--skip-libmagic-check\tAccept to run even if magic file cannot be loaded (mimetypes in the zim file may be wrong)." << std::endl;
  // --no-uuid and --dont-check-arguments are dev options, let's keep them secret
  // std::cout << "\t-U, --no-uuid\t\tdon't generate a random UUID" << std::endl;
  // std::cout << "\t-B, --dont-check-arguments\t\tdon't check arguments (and possibly produce a broken ZIM file)" << std::endl;
  std::cout << std::endl;

  std::cout << "Example:" << std::endl;
  std::cout
      << "\tzimwriterfs --welcome=index.html --illustration=illustration.png --language=fra --title=foobar --description=mydescription \\\n\t\t\
--creator=Wikipedia --publisher=Kiwix --name tests_fra_my-project ./my_project_html_directory tests_fra_my-project_202412.zim"
      << std::endl;
  std::cout << std::endl;

  std::cout << "Documentation:" << std::endl;
  std::cout << "\tzimwriterfs source code: https://github.com/openzim/zim-tools"
            << std::endl;
  std::cout << "\tZIM format: https://openzim.org" << std::endl;
  std::cout << std::endl;
}


void parse_args(int argc, char** argv)
{
  /* Argument parsing */
  static struct option long_options[]
      = {{"help", no_argument, 0, 'h'},
         {"verbose", no_argument, 0, 'v'},
         {"version", no_argument, 0, 'V'},
         {"longDescription", required_argument, 0, 'L'},
         {"welcome", required_argument, 0, 'w'},
         {"clusterSize", required_argument, 0, 'm'},
         {"name", required_argument, 0, 'n'},
         {"source", required_argument, 0, 'e'},
         {"flavour", required_argument, 0, 'o'},
         {"scraper", required_argument, 0, 's'},
         {"redirects", required_argument, 0, 'r'},
         {"inflateHtml", no_argument, 0, 'x'},
         {"illustration", required_argument, 0, 'I'},
         {"language", required_argument, 0, 'l'},
         {"title", required_argument, 0, 't'},
         {"tags", required_argument, 0, 'a'},
         {"description", required_argument, 0, 'd'},
         {"creator", required_argument, 0, 'c'},
         {"publisher", required_argument, 0, 'p'},
         {"withoutFTIndex", no_argument, 0, 'j'},
         {"threads", required_argument, 0, 'J'},
         {"no-uuid", no_argument, 0, 'U'},
         {"dont-check-arguments", no_argument, 0, 'B'},
         {"skip-libmagic-check", no_argument, 0, 'M'},

         // Only for backward compatibility
         {"withFullTextIndex", no_argument, 0, 'i'},

         {0, 0, 0, 0}};
  int option_index = 0;
  int c;

  do {
    c = getopt_long(
        argc, argv, "a:hVvijxuw:I:t:d:c:l:p:r:e:n:m:J:UBL:", long_options, &option_index);

    if (c != -1) {
      switch (c) {
        case 'a':
          tags = optarg;
          break;
        case 'V':
          printZimWriterFsVersions();
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
        case 'I':
          illustration = optarg;
          break;
        case 'i':
          withoutFTIndex = false;
          break;
        case 'j':
          withoutFTIndex = true;
          break;
        case 'l':
          language = optarg;
          break;
        case 'm':
          clusterSize = atoi(optarg);
          break;
        case 'L':
          longDescription = optarg;
          break;
        case 'n':
          name = optarg;
          break;
        case 'e':
          source = optarg;
          break;
        case 'o':
          flavour = optarg;
          break;
        case 's':
          scraper = optarg;
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
        case 'w':
          welcome = optarg;
          break;
        case 'J':
          threads = atoi(optarg);
          break;
        case 'U':
          noUuid = true;
          break;
        case 'B':
          dontCheckArgs = true;
          break;
        case 'M':
          continue_without_magic = true;
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

  if ( thereAreMissingArguments() ) {
    if (argc > 1)
      std::cerr << "zimwriterfs: too few arguments!" << std::endl;
    usage();
    exit(1);
  }

  /* Check arguments */

  // delete / from the end of filename
  if (directoryPath[directoryPath.length() - 1] == '/') {
    directoryPath = directoryPath.substr(0, directoryPath.length() - 1);
  }

  /* Check metadata */
  if (!dontCheckArgs && !fileExists(directoryPath + "/" + welcome)) {
    std::cerr << "zimwriterfs: unable to find welcome page at '"
              << directoryPath << "/" << welcome
              << "'. --welcome path/value must be relative to HTML_DIRECTORY."
              << std::endl;
    exit(1);
  }

  if (!dontCheckArgs && !fileExists(directoryPath + "/" + illustration)) {
    std::cerr << "zimwriterfs: unable to find illustration at " << directoryPath
              << "/" << illustration
              << "'. --illustration path/value must be relative to HTML_DIRECTORY."
              << std::endl;
    exit(1);
  }

  if (fileExists(zimPath)) {
    std::cerr << "zimwriterfs: Error: destination .zim file '" << zimPath << "' already exists."
              << std::endl;
    exit(1);
  }

  /* System tags */
  tags += tags.empty() ? "" : ";";
  if (withoutFTIndex) {
    tags += "_ftindex:no";
  } else {
    tags += "_ftindex:yes";
    tags += ";_ftindex"; // For backward compatibility
  }
}

void create_zim(const zim::Metadata& metadata)
{
  ZimCreatorFS zimCreator(directoryPath);
  zimCreator.configVerbose(isVerbose())
            .configNbWorkers(threads)
            .configClusterSize(clusterSize)
            .configIndexing(!withoutFTIndex, language);
  if ( noUuid ) {
    zimCreator.setUuid(zim::Uuid());
  }
  if (zimPath.size() >= (MAXPATHLEN-1)) {
    throw std::invalid_argument("Target .zim file path is too long");
  }

  char buf[MAXPATHLEN];
  strncpy(buf, zimPath.c_str(), sizeof(buf)-1);
  // dirname() can modify its argument, so need to pass a copy
  std::string zimdir = dirname(buf);

  if (realpath(zimdir.c_str(), buf) != buf) {
    throw std::invalid_argument(
          Formatter() << "Unable to canonicalize target directory of .zim "
                      << zimdir << ": " << strerror(errno));
  }

  // Check that the resulting .zim file isn't located under source HTML directory
  if (std::string(buf).find(zimCreator.canonicalBaseDir()) == 0) {
    throw std::invalid_argument(".zim file to create cannot be located inside of source HTML directory");
  }

  zimCreator.startZimCreation(zimPath);

  addMetadata(zimCreator, metadata);

  if ( !welcome.empty() )  {
    zimCreator.setMainPath(welcome);
  }

  /* Directory visitor */
  zimCreator.visitDirectory(directoryPath);

  /* Check redirects file and read it if necessary*/
  if (!redirectsPath.empty()) {
    if (!fileExists(redirectsPath)) {
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
  }
  zimCreator.finishZimCreation();
}


/* Main program entry point */
int main(int argc, char** argv)
{
  parse_args(argc, argv);

  /* Init */
  magic = magic_open(MAGIC_MIME);
  if (magic_load(magic, NULL) != 0) {
    std::cerr << "Impossible to load magic file. Set `MAGIC` environment variable to a `magic` (or `magic.mgc`) file." << std::endl;
    if (! continue_without_magic) {
      exit(1);
    }
  }
  pthread_mutex_init(&verboseMutex, NULL);

  try {
    const zim::Metadata metadata = makeMetadata();

    if ( !checkMetadata(metadata) ) {
      exit(1);
    }

    create_zim(metadata);
  }
  catch(std::exception &e) {
    std::cerr << "zimwriterfs: " << e.what() << std::endl;
    exit(1);
  }

  magic_close(magic);
  /* Destroy mutex */
  pthread_mutex_destroy(&verboseMutex);
}
