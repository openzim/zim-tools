/*
 * Copyright 2011-2014 Emmanuel Engelhart <kelson@kiwix.org>
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

#include "pathTools.h"

#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <limits.h>
#elif _WIN32
#include <windows.h>
#include "Shlwapi.h"
#endif

#ifdef _WIN32
#else
#include <unistd.h>
#endif

#ifdef _WIN32
#define SEPARATOR "\\"
#else
#define SEPARATOR "/"
#endif

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif


std::string appendToDirectory(const std::string &directoryPath, const std::string &filename) {
  std::string newPath = directoryPath + SEPARATOR + filename;
  return newPath;
}


std::string getExecutablePath() {
  char binRootPath[PATH_MAX];

#ifdef _WIN32
  GetModuleFileName( NULL, binRootPath, PATH_MAX);
  return std::string(binRootPath);
#elif __APPLE__
  uint32_t max = (uint32_t)PATH_MAX;
  _NSGetExecutablePath(binRootPath, &max);
  return std::string(binRootPath);
#else
  ssize_t size =  readlink("/proc/self/exe", binRootPath, PATH_MAX);
  if (size != -1) {
    return std::string(binRootPath, size);
  }
#endif

  return "";
}

bool writeTextFile(const std::string &path, const std::string &content) {
  std::ofstream file;
  file.open(path.c_str());
  file << content;
  file.close();
  return true;
}
