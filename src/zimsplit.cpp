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

#include "arg.h"
#include "version.h"

#define BUFFER_SIZE 4096

class ZimSplitter
{
  private:
    zim::Archive archive;
    const std::string prefix;
    size_t partSize;

    char first_index, second_index;

    std::ifstream ifile;
    std::ofstream ofile;
    std::string part_name;
    size_t out_size;
    char* batch_buffer;

  public:
    ZimSplitter(const std::string& fname, const std::string& out_prefix, size_t partSize)
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

    void copy_out(zim::offset_type size) {
        while (size > 0) {
           auto size_to_copy = std::min<zim::offset_type>(size, BUFFER_SIZE);
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

    void run() {
        new_file();

        std::vector<zim::offset_type> offsets;
        auto nbClusters = archive.getClusterCount();
        for(zim::offset_type i=0;i<nbClusters;i++) {
            offsets.push_back(archive.getClusterOffset(i));
        }
        offsets.push_back(archive.getFilesize());
        std::sort(offsets.begin(), offsets.end());

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
                    // It would be too much to write thu current part in the current file.
                    new_file();
                }
                copy_out(currentSize);
            }
            last = offset;
        }
    }

    bool check() {
        bool error = false;

        std::vector<zim::offset_type> offsets;
        auto nbClusters = archive.getClusterCount();
        for(zim::offset_type i=0;i<nbClusters;i++) {
            offsets.push_back(archive.getClusterOffset(i));
        }
        offsets.push_back(archive.getFilesize());
        std::sort(offsets.begin(), offsets.end());

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

int main(int argc, char* argv[])
{
  try
  {
    zim::Arg<std::string> out_prefix(argc, argv, 'o');
    zim::Arg<int> part_size(argc, argv, 's');
    zim::Arg<bool> force(argc, argv, "--force");
    zim::Arg<bool> printVersion(argc, argv, "--version");

    // version number
    if (printVersion)
    {
      version();
      return 0;
    }

    if (argc <= 1)
    {
      std::cerr << "usage: " << argv[0] << " [options] zimfile\n"
        "\n"
        "options:\n"
        "  -o            prefix of output file parts\n"
        "  -s            size of each file parts\n"
        "  --force       create zim parts even if it is impossible to have all part size smaller than requested\n"
        "  --version     print the software version\n"
        << std::flush;
      return -1;
    }

    // initalize app
    ZimSplitter app(argv[1], out_prefix, part_size);

    if (!force && app.check()) {
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

