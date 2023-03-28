/*
 * Copyright 2023 Veloman Yunkan <veloman.yunkan@gmail.com>
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

#ifndef OPENZIM_METADATA_H
#define OPENZIM_METADATA_H

#include <string>
#include <vector>
#include <map>

namespace zim
{

class Metadata
{
public: // types
  struct ReservedMetadataRecord
  {
    const std::string name;
    const bool        isMandatory;
    const size_t      minLength;
    const size_t      maxLength;
    const std::string regex;
  };

  typedef std::vector<ReservedMetadataRecord> ReservedMetadataTable;

  typedef std::vector<std::string> Errors;

public: // data
  static const ReservedMetadataTable& reservedMetadataInfo;

public: // functions
  void set(const std::string& name, const std::string& value);
  bool has(const std::string& name) const;
  const std::string& operator[](const std::string& name) const;

  bool valid() const;
  Errors check() const;

  static const ReservedMetadataRecord& getReservedMetadataRecord(const std::string& name);

private: // functions
  Errors checkMandatoryMetadata() const;
  Errors checkSimpleConstraints() const;
  Errors checkComplexConstraints() const;

private: // data
  std::map<std::string, std::string> data;
};

} // namespace zim

#endif  // OPENZIM_METADATA_H
