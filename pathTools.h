/*
 * Copyright 2011 Emmanuel Engelhart <kelson@kiwix.org>
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

#ifndef OPENZIM_ZIMWRITERFS_PATHTOOLS_H
#define OPENZIM_ZIMWRITERFS_PATHTOOLS_H

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <fstream>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <ios>
#include <limits.h>

#ifdef _WIN32
#include <direct.h>
#endif

std::string appendToDirectory(const std::string &directoryPath, const std::string &filename);

std::string getExecutablePath();

bool writeTextFile(const std::string &path, const std::string &content);

#endif //OPENZIM_ZIMWRITERFS_PATHTOOLS_H
