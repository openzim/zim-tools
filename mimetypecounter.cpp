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

#include "mimetypecounter.h"
#include <sstream>

MetadataCounterArticle::MetadataCounterArticle(MimetypeCounter* counter):
    MetadataArticle("Counter"),
    counter(counter)
{}

zim::Blob MetadataCounterArticle::getData() const
{
  std::stringstream stream;
  for (std::map<std::string, unsigned int>::iterator it = counter->counters.begin(); it != counter->counters.end(); ++it) {
    stream << it->first << "=" << it->second << ";";
  }
  data = stream.str();
  return zim::Blob(data.data(), data.size());
}

void MimetypeCounter::handleArticle(Article* article) {
  if (!article->isRedirect()) {
    std::string mimeType = article->getMimeType();
    if (counters.find(mimeType) == counters.end()) {
      counters[mimeType] = 1;
    } else {
      counters[mimeType]++;
    }
  }
}
