/*
 * Copyright (C) 2026 Nidhi Gahlawat.
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

#include "zimsplit_size.h"
#include "tools.h"

#include <array>
#include <charconv>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace
{

constexpr auto partSizeTooLargeError
    = "part size must be smaller than the input ZIM file";

uint64_t getUnitMultiplier(std::string unit)
{
  static const std::array<std::pair<std::string, uint64_t>, 14>
      multipliers{{
          {"", 1},
          {"B", 1},
          {"KB", 1000ULL},
          {"MB", 1000ULL * 1000},
          {"GB", 1000ULL * 1000 * 1000},
          {"TB", 1000ULL * 1000 * 1000 * 1000},
          {"PB", 1000ULL * 1000 * 1000 * 1000 * 1000},
          {"EB", 1000ULL * 1000 * 1000 * 1000 * 1000 * 1000},
          {"KiB", 1ULL << 10},
          {"MiB", 1ULL << 20},
          {"GiB", 1ULL << 30},
          {"TiB", 1ULL << 40},
          {"PiB", 1ULL << 50},
          {"EiB", 1ULL << 60},
      }};

  const auto normalizedUnit = asciitolower(unit);
  for (const auto& multiplier : multipliers) {
    if (asciitolower(multiplier.first) == normalizedUnit) {
      return multiplier.second;
    }
  }

  throw std::invalid_argument("invalid size unit: " + unit);
}

}  // namespace

uint64_t parseByteSize(std::string value)
{
  value = trimAsciiWhitespace(value);
  if (value.empty()) {
    throw std::invalid_argument("invalid size value: empty input");
  }

  uint64_t size = 0;
  const char* const end = value.data() + value.size();
  const auto parseResult = std::from_chars(value.data(), end, size);
  if (parseResult.ec == std::errc::result_out_of_range) {
    throw std::invalid_argument(partSizeTooLargeError);
  }
  if (parseResult.ec != std::errc() || parseResult.ptr == value.data()) {
    throw std::invalid_argument("invalid size value: " + value);
  }

  std::string unit(parseResult.ptr, end);
  unit = trimAsciiWhitespace(unit);
  const auto multiplier = getUnitMultiplier(unit);
  if (size == 0) {
    throw std::invalid_argument("part size must be positive: " + value);
  }
  if (size > std::numeric_limits<uint64_t>::max() / multiplier) {
    throw std::invalid_argument(partSizeTooLargeError);
  }

  return size * multiplier;
}

void validatePartSize(uint64_t partSize, uint64_t archiveSize)
{
  if (partSize >= archiveSize) {
    throw std::invalid_argument(partSizeTooLargeError);
  }
}
