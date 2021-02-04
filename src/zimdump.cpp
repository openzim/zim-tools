/*
 * Copyright (C) 2006 Tommi Maekitalo
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

#include <iostream>
#include <sstream>
#include <fstream>
#include <set>

#define ZIM_PRIVATE
#include <zim/archive.h>
#include <zim/item.h>
#include <stdexcept>
#include <sys/types.h>
#include <docopt/docopt.h>
#include <sys/stat.h>
#include <iomanip>
#include <array>
#include <vector>
#include <codecvt>
#include <unordered_map>

#include "version.h"

#include <fcntl.h>
#ifdef _WIN32
# define SEPARATOR "\\"
# include <io.h>
# include <windows.h>
#else
# define SEPARATOR "/"
#include <unistd.h>
#endif

#define ERRORSDIR "_exceptions/"

std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

inline static void createdir(const std::string &path, const std::string &base)
{
    std::size_t position = 0;
    while(position != std::string::npos) {
        position = path.find('/', position+1);
        if (position != std::string::npos) {
            std::string fulldir = base + SEPARATOR + path.substr(0, position);
            #if defined(_WIN32)
            std::wstring wfulldir = converter.from_bytes(fulldir);
            CreateDirectoryW(wfulldir.c_str(), NULL);
            #else
              ::mkdir(fulldir.c_str(), 0777);
            #endif
        }
    }
}

static bool isReservedUrlChar(const char c)
{
    constexpr std::array<char, 10> reserved = {{';', ',', '?', ':',
                                               '@', '&', '=', '+', '$' }};

    return std::any_of(reserved.begin(), reserved.end(),
                       [&c] (const char &elem) { return elem == c; } );
}

static bool needsEscape(const char c, const bool encodeReserved)
{
  if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
    return false;

  if (isReservedUrlChar(c))
    return encodeReserved;

  constexpr std::array<char, 10> noNeedEscape = {{'-', '_', '.', '!', '~',
                                                '*', '\'', '(', ')', '/' }};

  return not std::any_of(noNeedEscape.begin(), noNeedEscape.end(),
                         [&c] (const char &elem) { return elem == c; } );
}

std::string urlEncode(const std::string& value, bool encodeReserved)
{
  std::ostringstream os;
  os << std::hex << std::uppercase;
  for (std::string::const_iterator it = value.begin();
       it != value.end();
       ++it) {
    if (!needsEscape(*it, encodeReserved)) {
      os << *it;
    } else {
      os << '%' << std::setw(2) << static_cast<unsigned int>(static_cast<unsigned char>(*it));
    }
  }
  return os.str();
}

class ZimDumper
{
    zim::Archive m_archive;
    bool verbose;

  public:
    ZimDumper(const std::string& fname)
      : m_archive(fname),
        verbose(false)
      { }

    void setVerbose(bool sw = true)  { verbose = sw; }

    void printInfo();
    int dumpEntry(const zim::Entry& entry);
    int listEntries(bool info);
    int listEntry(const zim::Entry& entry);
    void listEntryT(const zim::Entry& entr);
    int listEntriesByNamespace(const std::string ns, bool details);

    zim::Entry getEntryByPath(const std::string &path);
    zim::Entry getEntry(zim::size_type idx);

    void dumpFiles(const std::string& directory, bool symlinkdump, std::function<bool (const char c)> nsfilter);
};

zim::Entry ZimDumper::getEntryByPath(const std::string& path)
{
    return m_archive.getEntryByPath(path);
}

zim::Entry ZimDumper::getEntry(zim::size_type idx)
{
    return m_archive.getEntryByPath(idx);
}

void ZimDumper::printInfo()
{
  std::cout << "count-entries: " << m_archive.getEntryCount() << "\n";
  std::cout << "uuid: " << m_archive.getUuid() << "\n"
            <<  "cluster count: " << m_archive.getClusterCount() << "\n";
  if (m_archive.hasChecksum()) {
    std::cout << "checksum: " << m_archive.getChecksum() << "\n";
  } else {
    std::cout <<"no checksum\n";
  }

  if (m_archive.hasMainEntry()) {
    std::cout << "main page: " << m_archive.getMainEntry().getIndex() << "\n";
  } else {
    std::cout << "main page: -\n";
  }

  if (m_archive.hasFaviconEntry()) {
    std::cout << "favicon: " << m_archive.getFaviconEntry().getIndex() << "\n";
  } else {
    std::cout << "favicon: -\n";
  }
  std::cout << std::endl;

  std::cout.flush();
}

int ZimDumper::dumpEntry(const zim::Entry& entry)
{
    if (entry.isRedirect()) {
        std::cerr << "Entry " << entry.getPath() << " is a redirect." << std::endl;
        return -1;
    }

    std::cout << entry.getItem().getData() << std::flush;
    return 0;
}

int ZimDumper::listEntries(bool info)
{
    int ret = 0;
    for (auto& entry:m_archive.iterByPath()) {
        if (info) {
          ret = listEntry(entry);
        } else {
          std::cout << entry.getPath() << '\n';
        }
     }
    return ret;
}

int ZimDumper::listEntry(const zim::Entry& entry)
{
  std::cout <<
    "path: " << entry.getPath() << "\n"
    "\ttitle:           " << entry.getTitle() << "\n"
    "\tidx:             " << entry.getIndex() << "\n"
    "\ttype:            " << (entry.isRedirect()   ? "redirect" : "item") << "\n";

  if (entry.isRedirect()) {
    std::cout <<
      "\tredirect index:  " << entry.getRedirectEntry().getIndex() << "\n";
  }
  else {
    auto item = entry.getItem();
    std::cout <<
      "\tmime-type:       " << item.getMimetype() << "\n"
      "\titem size:    " << item.getSize() << "\n";
  }
  return 0;
}

void ZimDumper::listEntryT(const zim::Entry& entry)
{
  std::cout << entry.getPath()
    << '\t' << entry.getTitle()
    << '\t' << entry.getIndex()
    << '\t' << (entry.isRedirect()?'R':'A');

  if (entry.isRedirect()) {
    std::cout << '\t' << entry.getRedirectEntry().getIndex();
  } else {
    auto item = entry.getItem();
    std::cout << '\t' << item.getMimetype()
              << '\t' << item.getSize();
  }
  std::cout << std::endl;
}

int ZimDumper::listEntriesByNamespace(const std::string ns, bool details)
{
    int ret = 0;
    for (auto& entry:m_archive.findByPath(ns)) {
        if (details) {
          ret = listEntry(entry);
        } else {
          std::cout << entry.getPath() << '\n';
        }
    }
    return ret;
}

void write_to_error_directory(const std::string& base, const std::string relpath, const char *content, ssize_t size)
{
    createdir(ERRORSDIR, base);
    std::string url = relpath;

    std::string::size_type p;
    while ((p = url.find('/')) != std::string::npos)
        url.replace(p, 1, "%2f");

#ifdef _WIN32
    auto fullpath = std::string(base + ERRORSDIR + url);
    std::wstring wpath = converter.from_bytes(fullpath);
    auto fd = _wopen(wpath.c_str(), _O_WRONLY | _O_CREAT | _O_TRUNC, S_IWRITE);

    if (fd == -1) {
        std::cerr << "Error opening file " + fullpath + " cause: " + ::strerror(errno) << std::endl;
        return ;
    }
    if (write(fd, content, size) != size) {
      close(fd);
      std::cerr << "Failed writing: " << fullpath << " - " << ::strerror(errno) << std::endl;
    }
#else
    std::ofstream stream(base + ERRORSDIR + url);

    stream.write(content, size);

    if (stream.fail() || stream.bad()) {
        std::cerr << "Error writing file to errors dir. " << (base + ERRORSDIR + url) << std::endl;
        throw std::runtime_error(
          std::string("Error writing file to errors dir. ") + (base + ERRORSDIR + url));
    } else {
        std::cerr << "Wrote " << (base + relpath) << " to " << (base + ERRORSDIR + url) << std::endl;
    }
#endif
}

inline void write_to_file(const std::string &base, const std::string& path, const char* data, ssize_t size) {
    std::string fullpath = base + path;
#ifdef _WIN32
    std::wstring wpath = converter.from_bytes(fullpath);
    auto fd = _wopen(wpath.c_str(), _O_WRONLY | _O_CREAT | _O_TRUNC, S_IWRITE);
#else
    auto fd = open(fullpath.c_str(), O_WRONLY | O_CREAT | O_TRUNC,
                              S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
#endif
    if (fd == -1) {
        write_to_error_directory(base, path, data, size);
        return ;
    }
    if (write(fd, data, size) != size) {
      write_to_error_directory(base, path, data, size);
    }
    close(fd);
}


void ZimDumper::dumpFiles(const std::string& directory, bool symlinkdump, std::function<bool (const char c)> nsfilter)
{
  unsigned int truncatedFiles = 0;
#if defined(_WIN32)
    std::wstring wdir = converter.from_bytes(directory);
    CreateDirectoryW(wdir.c_str(), NULL);
#else
  ::mkdir(directory.c_str(), 0777);
#endif

  std::vector<std::string> pathcache;
  for (auto& entry:m_archive.iterEfficient()) {
    std::string path = entry.getPath();
    std::string dir = "";
    std::string filename = path;
    auto position = path.find_last_of('/');
    if (position != std::string::npos) {
        dir = path.substr(0, position + 1);
        filename = path.substr(position + 1);
        if (find(pathcache.begin(), pathcache.end(), dir) == pathcache.end()) {
            createdir(dir, directory);
            pathcache.push_back(dir);
        }

    }

    if ( filename.length() > 255 ) {
        std::ostringstream sspostfix, sst;
        sspostfix << (++truncatedFiles);
        sst << filename.substr(0, 254-sspostfix.tellp()) << "~" << sspostfix.str();
        filename = sst.str();
    }

    std::stringstream ss;
    ss << dir << filename;
    std::string relative_path = ss.str();
    std::string full_path = directory + SEPARATOR + relative_path;

    if (entry.isRedirect()) {
        auto redirectItem = entry.getItem(true);
        std::string redirectPath = redirectItem.getPath();
        if (symlinkdump == false && redirectItem.getMimetype() == "text/html") {
            auto encodedurl = urlEncode(redirectPath, true);
            std::ostringstream ss;

            ss << "<!DOCTYPE html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />";
            ss << "<meta http-equiv=\"refresh\" content=\"0;url=" + encodedurl + "\" /><head><body></body></html>";
            auto content = ss.str();
            write_to_file(directory + SEPARATOR, relative_path, content.c_str(), content.size());
        } else {
#ifdef _WIN32
            auto blob = redirectItem.getData();
            write_to_file(directory + SEPARATOR, relative_path, blob.data(), blob.size());
#else
            if (symlink(redirectPath.c_str(), full_path.c_str()) != 0) {
              throw std::runtime_error(
                std::string("Error creating symlink from ") + full_path + " to " + redirectPath);
            }
#endif
        }
    } else {
      auto blob = entry.getItem().getData();
      write_to_file(directory + SEPARATOR, relative_path, blob.data(), blob.size());
    }
  }
}

static const char USAGE[] =
R"(
zimdump tool is used to inspect a zim file and also to dump its contents into the filesystem.

Usage:
  zimdump list [--details] [--idx=INDEX|([--url=URL] [--ns=N])] [--] <file>
  zimdump dump --dir=DIR [--ns=N] [--redirect] [--] <file>
  zimdump show (--idx=INDEX|(--url=URL [--ns=N])) [--] <file>
  zimdump info [--ns=N] [--] <file>
  zimdump -h | --help
  zimdump --version

Selectors:
  --idx INDEX  The index of the article to list/dump.
  --url URL    The url of the article to list/dump
  --ns N       The namespace of the article(s) to list/dump.
               When used with `--url`, default to `A`.
               If no `--url` is provided (for  `zimdump dump`) default to no filter.

Options:
  --details    Show details about the articles. Else, list only the url of the article(s).
  --dir=DIR    Directory where to dump the article(s) content.
  --redirect   Use symlink to dump redirect articles. Else create html redirect file
  -h, --help   Show this help
  --version    Show zimdump version.

Return value:
  0 : If no error
  1 : If no (or more than 1) articles correspond to the selectors.
  2 : If an error/warning has been issue during the dump.
      See DIR/dump_errors.log for the listing of the errors.
)";

int subcmdInfo(ZimDumper &app, std::map<std::string, docopt::value> &args)
{
    app.printInfo();
    return 0;
}

int subcmdDumpAll(ZimDumper &app, const std::string &outdir, bool redirect, std::function<bool (const char c)> nsfilter)
{
#ifdef _WIN32
    app.dumpFiles(outdir, false, nsfilter);
#else
    app.dumpFiles(outdir, redirect, nsfilter);
#endif
    return 0;
}

int subcmdDump(ZimDumper &app,  std::map<std::string, docopt::value> &args)
{
    bool redirect = args["--redirect"].asBool();

    std::function<bool (const char c)> filter = [](const char /*c*/){return true; };
    if (args["--ns"]) {
        std::string nspace = args["--ns"].asString();
        filter = [nspace](const char c){ return nspace.at(0) == c; };
    }
    
    std::string directory = args["--dir"].asString();

    if (directory.empty()) {
        throw std::runtime_error("Directory cannot be empty.");
    }

    if (directory.back() == '/'){
        directory.pop_back();
    }

    return subcmdDumpAll(app, directory, redirect, filter);
}

int subcmdShow(ZimDumper &app,  std::map<std::string, docopt::value> &args)
{
    // docopt guaranty us that we have `--idx` or `--url`.
    try {
        if (args["--idx"]) {
            return app.dumpEntry(app.getEntry(args["--idx"].asLong()));
        } else {
            return app.dumpEntry(app.getEntryByPath(args["--url"].asString()));
        }
    } catch(...) {
        std::cerr << "Entry not found" << std::endl;
        return -1;
    }
}

int subcmdList(ZimDumper &app, std::map<std::string, docopt::value> &args)
{
    bool idx(args["--idx"]);
    bool url(args["--url"]);
    bool details = args["--details"].asBool();
    bool ns(args["--ns"]);

    if (idx || url) {
        try {
            // docopt guaranty us that we have `--idx` or `--url` (or nothing, but not both)
            if (idx) {
                return app.listEntry(app.getEntry(args["--idx"].asLong()));
            } else {
                return app.listEntry(app.getEntryByPath(args["--url"].asString()));
            }
        } catch(...) {
            std::cerr << "Entry not found" << std::endl;
            return -1;
        }
    } else if (ns){
        return app.listEntriesByNamespace(args["--ns"].asString(), details);
    } else {
        return app.listEntries(details);
    }
}

int main(int argc, char* argv[])
{
    int ret = 0;
    std::string versionstr("zimdump " + std::string(VERSION));
    std::map<std::string, docopt::value> args
        = docopt::docopt(USAGE,
                         { argv + 1, argv + argc },
                         true,
                         versionstr);

    try {
        ZimDumper app(args["<file>"].asString());

        std::unordered_map<std::string, std::function<int(ZimDumper&, decltype(args)&)>> dispatchtable = {
            {"info",            subcmdInfo },
            {"dump",            subcmdDump },
            {"list",            subcmdList },
            {"show",            subcmdShow }
        };

        // call the appropriate subcommand handler
        for (const auto &it : dispatchtable) {
            if (args[it.first.c_str()].asBool()) {
                ret = (it.second)(app, args);
                break;
            }
        }
    } catch (std::exception &e) {
        std::cout << "Exception: " << e.what() << '\n';
        return -1;
    }
    return ret;
}
