/*
 * Copyright (C) 2007 Tommi Maekitalo
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
#include <zim/search.h>
#include <zim/archive.h>

#include "version.h"

void printSearchResults(zim::Search& search)
{
    for (zim::Search::iterator it = search.begin(); it != search.end(); ++it)
    {
      std::cout << "article " << it->getIndex() << "\nscore " << it.get_score() << "\t:\t" << it->getTitle() << std::endl;
    }
}

int main(int argc, char* argv[])
{
  try
  {

    // version number
    if (argc > 1 && std::string(argv[1]) == "-v")
    {
      version();
      return 0;
    }

    if (argc <= 2)
    {
      std::cerr << "usage: " << argv[0] << " [-x indexfile] zimfile searchstring\n"
                   "\n"
                   "options\n"
                   "  -x indexfile   specify indexfile\n"
                   "  -v             print software version\n"
                << std::endl;
      return 1;
    }

    std::string s = argv[2];
    for (int a = 3; a < argc; ++a)
    {
      s += ' ';
      s += argv[a];
    }

    zim::Archive zimarchive(argv[1]);
    zim::Search search(zimarchive);
    search.set_query(s);
    printSearchResults(search);
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }
}

