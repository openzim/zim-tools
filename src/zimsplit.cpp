/*
 * Copyright (C) 2017 Matthieu Gautier.
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

#include <fstream>
#include <stdexcept>

#define ZIM_PRIVATE
#include <zim/archive.h>

#include <docopt/docopt.h>

#include "version.h"

#define BUFFER_SIZE 4096

#define DEFAULT_PART_SIZE 2147483648

class ZimSplitter
{
  private:
    zim::Archive archive;
    const std::string prefix;
    zim::size_type partSize;

    char first_index, second_index;

    std::ifstream ifile;
    std::ofstream ofile;
    std::string part_name;
    zim::size_type out_size;
    char* batch_buffer;

  public:
    ZimSplitter(const std::string& fname, const std::string& out_prefix, zim::size_type partSize)
      : archive(fname),
        prefix(out_prefix),
        partSize(partSize),
        first_index(0),
        second_index(0),
        ifile(fname, std::ios::binary),
        out_size(0)
      {
        batch_buffer = new char[BUFFER_SIZE];
    }

    ~ZimSplitter() {
        close_file();
        ifile.close();
        delete batch_buffer;
    }

    std::string get_new_suffix() {
        auto out = std::string(1, 'a'+first_index) + std::string(1, 'a'+second_index++);
        if (second_index > 25) {
            first_index++;
            second_index = 0;
        }
        if (first_index > 25) {
            std::cerr << "To many parts" << std::endl;
            exit(-1);
        }
        return out;
    }

    void close_file() {
        if (out_size > partSize) {
           std::cout << "WARNING: Part " << part_name << " is bigger that max part size."
            << " (" << out_size << ">" << partSize << ")" << std::endl;
        }
        ofile.close();
    }

    void new_file() {
        close_file();
        part_name = prefix + get_new_suffix();
        std::cout << "opening new file " << part_name << std::endl;
        ofile.open(part_name, std::ios::binary);
        out_size = 0;
    }

    void copy_out(zim::size_type size) {
        while (size > 0) {
           auto size_to_copy = std::min<zim::size_type>(size, BUFFER_SIZE);
           ifile.read(batch_buffer, size_to_copy);
           if (!ifile) {
               throw std::runtime_error("Error while reading zim file");
           }
           ofile.write(batch_buffer, size_to_copy);
           if (!ofile) {
               throw std::runtime_error("Error while writing zim part");
           }
           out_size += size_to_copy;
           size -= size_to_copy;
        }
    }

    std::vector<zim::offset_type> getOffsets() {
        std::vector<zim::offset_type> offsets;
        auto nbClusters = archive.getClusterCount();
        for(zim::offset_type i=0;i<nbClusters;i++) {
            offsets.push_back(archive.getClusterOffset(i));
        }
        offsets.push_back(archive.getFilesize());
        std::sort(offsets.begin(), offsets.end());
        return offsets;
    }

    void run() {
        new_file();

        auto offsets = getOffsets();

        zim::offset_type last(0);
        for(auto offset:offsets) {
            auto currentSize = offset-last;
            if (currentSize > partSize) {
                // One part is bigger than what we want :/
                // Still have to write it.
                if (out_size) {
                    new_file();
                }
                copy_out(currentSize);
                new_file();
            } else {
                if (out_size+currentSize > partSize) {
                    // It would be too much to write the current part in the current file.
                    new_file();
                }
                copy_out(currentSize);
            }
            last = offset;
        }
    }

    bool check() {
        bool error = false;

        auto offsets = getOffsets();

        zim::offset_type last(0);
        for(auto offset:offsets) {
            auto currentSize = offset-last;
            if (currentSize > partSize) {
                // One part is bigger than what we want :/
                // Still have to write it.
                std::cout << "The part (probably a cluster) is to big to fit in one part." << std::endl;
                std::cout << "    size is " << currentSize << "(" << offset << "-" << last << ")." << std::endl;
                error = true;
            }
            last = offset;
        }
       return error;
    }
};

static const char USAGE[] = R"(
    zimsplit splits smartly a ZIM file in smaller parts.

Usage:
    zimsplit [--prefix=PREFIX] [--force] [--size=N] <file>
    zimsplit --version

Options:
    --prefix=PREFIX     Prefix of output file parts. Default: <file>
    --size=N            The file size for each part. Default: 2GB
    --force             Create zim parts even if it is impossible to have all part size smaller than requested
    -h, --help          Show this help message
    --version           Show zimsplit version.
)";

int main(int argc, char* argv[])
{
  try
  {

    std::string versionstr("zimsplit " + std::string(VERSION));
    std::map<std::string, docopt::value> args = docopt::docopt(USAGE,
                                                            {argv + 1, argv + argc},
                                                            true,
                                                            versionstr);

    std::string prefix = args["<file>"].asString();
    if (args["--prefix"])
        prefix = args["--prefix"].asString();

    zim::size_type size = DEFAULT_PART_SIZE;
    if (args["--size"])
        size = args["--size"].asLong();

    // initalize app
    ZimSplitter app(args["<file>"].asString(), prefix, size);

    if (!args["--force"] && app.check()) {
        std::cout << "Creation of zim parts canceled because of previous errors." << std::endl;
        std::cout << "Use --force option to create zim parts anyway." << std::endl;
        return -1;
    }

    app.run();
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
    return -2;
  }
  return 0;
}

