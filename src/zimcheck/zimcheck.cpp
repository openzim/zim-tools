/*
 * Copyright (C) 2006 Tommi Maekitalo
 * Copyright (C) Kiran Mathew Koshy
 * Copyright (C) Matthieu Gautier <mgautier@kymeria.fr>
 * Copyright (C) Emmanuel Engelhart <kelson@kiwix.org>
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

#include <zim/archive.h>
#include <docopt/docopt.h>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <list>
#include <algorithm>
#include <regex>
#include <ctime>
#include <unordered_map>
#include <cmath>
#include <iostream>

#ifndef _WIN32
#include <unistd.h>
#endif

#include "../progress.h"
#include "../version.h"
#include "../tools.h"
#include "checks.h"

static const char USAGE[] =
R"(Zimcheck checks the quality of a ZIM file.

Usage:
  zimcheck [options] [ZIMFILE]

Options:
 -A --all             run all tests. Default if no flags are given.
 -0 --empty           Empty content
 -C --checksum        Internal CheckSum Test
 -I --integrity       Low-level correctness/integrity checks
 -M --metadata        MetaData Entries
 -F --favicon         Favicon
 -P --main            Main page
 -R --redundant       Redundant data check
 -U --url_internal    URL check - Internal URLs
 -X --url_external    URL check - External URLs
 -Q --quick           Report at most one error of each type per ZIM entry
 -B --progress        Print progress report
 -J --json            Output in JSON format
    --meta            Report archive metadata after checks complete
 -H --help            Displays Help
 -V --version         Displays software version
 -L --redirect_loop   Checks for the existence of redirect loops
 -W=<nb_thread> --threads=<nb_thread>  count of threads to utilize [default: 1]

Examples:
 zimcheck -A wikipedia.zim
 zimcheck --checksum --redundant wikipedia.zim
 zimcheck -F -R wikipedia.zim
 zimcheck -M --favicon wikipedia.zim)";


// Older version of docopt doesn't define Options
using Options = std::map<std::string, docopt::value>;

template<class T>
std::string stringify(const T& x);

struct ArchiveMetadataReport
{
  struct MetadataItem
  {
    std::string name;
    std::string value;
  };

  struct Illustration
  {
    std::string name;
    std::string mimetype;
    std::string data;
  };

  std::string id;
  zim::entry_index_type articleCount;
  zim::entry_index_type mediaCount;
  std::vector<MetadataItem> metadata;
  std::vector<Illustration> illustrations;
  std::map<std::string, unsigned long long> counter;
};

ArchiveMetadataReport getArchiveMetadataReport(const zim::Archive& archive)
{
  ArchiveMetadataReport report{
    stringify(archive.getUuid()),
    archive.getArticleCount(),
    archive.getMediaCount(),
    {},
    {},
    {},
  };

  for (const auto& key : archive.getMetadataKeys()) {
    const auto item = archive.getMetadataItem(key);
    const std::string value = item.getData();
    if (key.rfind("Illustration_", 0) == 0) {
      report.illustrations.push_back({
        key,
        item.getMimetype(),
        base64_encode(
          reinterpret_cast<const unsigned char*>(value.data()),
          value.size()
        ),
      });
    } else {
      report.metadata.push_back({key, value});
    }

    if (key == "Counter") {
      std::istringstream counter(value);
      std::string entry;
      while (std::getline(counter, entry, ';')) {
        const auto equals = entry.find('=');
        if (equals == std::string::npos) {
          continue;
        }
        try {
          report.counter.emplace(
            entry.substr(0, equals),
            std::stoull(entry.substr(equals + 1))
          );
        } catch (const std::exception&) {
          // Invalid Counter metadata is reported by the metadata check; omit
          // only its parsed representation while preserving the raw value.
        }
      }
    }
  }

  return report;
}

JSON::OutputStream& operator<<(JSON::OutputStream& out, const ArchiveMetadataReport& report)
{
  out << JSON::startObject;
  out << JSON::property("uuid", report.id);
  out << JSON::property("article_count", report.articleCount);
  out << JSON::property("media_count", report.mediaCount);
  out << JSON::property("metadata", JSON::startObject);
  for (const auto& item : report.metadata) {
    out << JSON::property(item.name, item.value);
  }
  out << JSON::endObject;
  out << JSON::property("illustrations", JSON::startArray);
  for (const auto& illustration : report.illustrations) {
    out << JSON::startObject;
    out << JSON::property("name", illustration.name);
    out << JSON::property("mime_type", illustration.mimetype);
    out << JSON::property("base64", illustration.data);
    out << JSON::endObject;
  }
  out << JSON::endArray;
  out << JSON::property("counter", JSON::startObject);
  for (const auto& entry : report.counter) {
    out << JSON::property(entry.first, entry.second);
  }
  out << JSON::endObject;
  out << JSON::endObject;
  return out;
}

void printArchiveMetadataReport(const ArchiveMetadataReport& report)
{
  std::cout << "[INFO] Archive metadata:" << std::endl;
  std::cout << "  uuid: " << report.id << std::endl;
  std::cout << "  article count: " << report.articleCount << std::endl;
  std::cout << "  media count: " << report.mediaCount << std::endl;
  std::cout << "  metadata:" << std::endl;
  for (const auto& item : report.metadata) {
    std::cout << "    " << item.name << ": " << item.value << std::endl;
  }
  for (const auto& illustration : report.illustrations) {
    std::cout << "    " << illustration.name << " (" << illustration.mimetype
              << "): " << illustration.data << std::endl;
  }
  if (!report.counter.empty()) {
    std::cout << "  counter:" << std::endl;
    for (const auto& entry : report.counter) {
      std::cout << "    " << entry.first << ": " << entry.second << std::endl;
    }
  }
}

template<class T>
std::string stringify(const T& x)
{
  std::ostringstream ss;
  ss << x;
  return ss.str();
}

int zimcheck(const std::map<std::string, docopt::value>& args);

int zimcheck(const std::vector<const char*>& args) {
    std::vector<std::string> args_string;
    bool first = true;
    for (auto arg:args) {
        if (first) {
            first = false;
            continue;
        }
        args_string.emplace_back(arg);
    }

    Options parsed_args;
    try {
      parsed_args = docopt::docopt_parse(
        USAGE,
        args_string,
        false,
        false);
    } catch (docopt::DocoptArgumentError const& error) {
		std::cerr << error.what() << std::endl;
		std::cout << USAGE << std::endl;
		return 1;
	}
    return zimcheck(parsed_args);
}

int zimcheck(const Options& args)
{
    // To calculate the total time taken by the program to run.
    const auto starttime = std::chrono::steady_clock::now();

    // The boolean values which will be used to store the output from
    // getopt_long().  These boolean values will be then read by the
    // program to execute the different parts of the program.

    bool run_all = false;
    ZimCheckOptions options;
    EnabledTests& enabled_tests = options.enabledTests;
    bool no_args = true;
    bool json = false;
    bool report_metadata = false;
    int thread_count = 1;

    std::string filename = "";
    ProgressBar progress(1);

    StatusCode status_code = PASS;

    for(auto const& arg: args) {
        if (arg.first == "--all" && arg.second.asBool()) {
            run_all = true;
            no_args = false;
        } else if (arg.first == "--help" && arg.second.asBool()) {
            std::cout << USAGE << std::endl;
            return -1;
        } else if (arg.first == "--empty" && arg.second.asBool()) {
            enabled_tests.enable(TestType::EMPTY);
            no_args = false;
        } else if (arg.first == "--checksum" && arg.second.asBool()) {
            enabled_tests.enable(TestType::CHECKSUM);
            no_args = false;
        } else if (arg.first == "--integrity" && arg.second.asBool()) {
            enabled_tests.enable(TestType::INTEGRITY);
            no_args = false;
        } else if (arg.first == "--metadata" && arg.second.asBool()) {
            enabled_tests.enable(TestType::METADATA);
            no_args = false;
        } else if (arg.first == "--progress") {
            progress.set_progress_report(arg.second.asBool());
        } else if (arg.first == "--favicon" && arg.second.asBool()) {
            enabled_tests.enable(TestType::FAVICON);
            no_args = false;
        } else if (arg.first == "--main" && arg.second.asBool()) {
            enabled_tests.enable(TestType::MAIN_PAGE);
            no_args = false;
        } else if (arg.first == "--redundant" && arg.second.asBool()) {
            enabled_tests.enable(TestType::REDUNDANT);
            no_args = false;
        } else if (arg.first == "--url_internal" && arg.second.asBool()) {
            enabled_tests.enable(TestType::URL_INTERNAL);
            no_args = false;
        } else if (arg.first == "--url_external" && arg.second.asBool()) {
            enabled_tests.enable(TestType::URL_EXTERNAL);
            no_args = false;
        } else if (arg.first == "--quick" && arg.second.asBool()) {
            options.quick = true;
        } else if (arg.first == "--redirect_loop" && arg.second.asBool()) {
            enabled_tests.enable(TestType::REDIRECT);
            no_args = false;
        } else if (arg.first == "--json") {
            json = arg.second.asBool();
        } else if (arg.first == "--meta") {
            report_metadata = arg.second.asBool();
        } else if (arg.first == "--threads") {
            thread_count = arg.second.asLong();
        } else if (arg.first == "ZIMFILE" && arg.second.isString()) {
            filename = arg.second.asString();
        } else if (arg.first == "--version" && arg.second.asBool()) {
            printVersions();
            return 0;
        }
    }

    if (filename.empty()) {
        std::cerr << "No file provided as argument" << std::endl;
        std::cout << USAGE << std::endl;
        return -1;
    }

    //If no arguments are given to the program, all the tests are performed.
    if ( run_all || no_args )
    {
        enabled_tests.enableAll();
    }

    ErrorLogger error(json);
    error.addInfo("zimcheck_version", std::string(VERSION));
    //Tests.
    try
    {
        error.addInfo("checks", enabled_tests);
        if (options.quick)
            error.addInfo("quick", true);
        error.addInfo("file_name",  filename);
        error.infoMsg("[INFO] Checking zim file " + filename);
        error.infoMsg("[INFO] Zimcheck version is " + std::string(VERSION));

        //Test 0: Low-level ZIM-file structure integrity checks
        bool should_run_full_test = true;
        if(enabled_tests.isEnabled(TestType::INTEGRITY)) {
            should_run_full_test = test_integrity(filename, error);
        } else {
            error.infoMsg("[WARNING] Integrity check is skipped. Any detected errors may in fact be due to corrupted/invalid data.");
        }


        if (should_run_full_test) {
            zim::Archive archive( filename );
            error.addInfo("file_uuid",  stringify(archive.getUuid()));
            error.startLogStream();

            //Test 1: Internal Checksum
            if(enabled_tests.isEnabled(TestType::CHECKSUM)) {
                if ( enabled_tests.isEnabled(TestType::INTEGRITY) ) {
                    error.infoMsg(
                        "[INFO] Avoiding redundant checksum test"
                        " (already performed by the integrity check)."
                    );
                } else {
                    test_checksum(archive, error);
                }
            }

            //Test 2: Metadata Entries:
            //The file is searched for the compulsory metadata entries.
            if(enabled_tests.isEnabled(TestType::METADATA))
                test_metadata(archive, error);

            //Test 3: Test for Favicon.
            if(enabled_tests.isEnabled(TestType::FAVICON))
                test_favicon(archive, error);


            //Test 4: Main Page Entry
            if(enabled_tests.isEnabled(TestType::MAIN_PAGE))
                test_mainpage(archive, error);

            /* Now we want to avoid to loop on the tests but on the article.
             *
             * If we loop of the tests we will have :
             *
             * for (test: tests) {
             *     for(article: articles) {
             *          data = article->getData();
             *          ...
             *     }
             * }
             *
             * And so we will get several the data of an article (and so decompression and so).
             * By looping on the articles first, we have :
             *
             * for (article: articles) {
             *     data = article->getData();
             *     for (test: tests) {
             *         ...
             *     }
             * }
             */

            if ( enabled_tests.isEnabled(TestType::URL_INTERNAL) ||
                 enabled_tests.isEnabled(TestType::URL_EXTERNAL) ||
                 enabled_tests.isEnabled(TestType::REDUNDANT) ||
                 enabled_tests.isEnabled(TestType::EMPTY) )
              test_articles(archive, error, progress, options, thread_count);

            if ( enabled_tests.isEnabled(TestType::REDIRECT))
                test_redirect_loop(archive, error);

            error.endLogStream();
            if (report_metadata) {
                const auto report = getArchiveMetadataReport(archive);
                if (json) {
                    error.addInfo("archive_metadata", report);
                } else {
                    printArchiveMetadataReport(report);
                }
            }
        }
        else
        {
            error.startLogStream();
            error.endLogStream();
        }

        const bool overallStatus = error.overallStatus();
        error.addInfo("status", overallStatus);
        if( overallStatus )
        {
            error.infoMsg("[INFO] Overall Test Status: Pass");
            status_code = PASS;
        }
        else
        {
            error.infoMsg("[INFO] Overall Test Status: Fail");
            status_code = FAIL;
        }

        const auto endtime = std::chrono::steady_clock::now();
        const std::chrono::duration<double> runtime(endtime - starttime);
        const long seconds = lround(runtime.count());
        std::ostringstream ss;
        ss << "[INFO] Total time taken by zimcheck: ";
        if ( seconds < 3 )
        {
          ss << "<3";
        }
        else
        {
          ss << seconds;
        }
        ss << " seconds.";
        error.infoMsg(ss.str());
    }
    catch (const std::exception & e)
    {
        std::cerr << e.what() << std::endl;
        status_code = EXCEPTION;
    }

    return status_code;
}
