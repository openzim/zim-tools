#include "zimsplit_size.h"

#include <array>
#include <charconv>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace
{

uint64_t getUnitMultiplier(std::string_view unit)
{
  static constexpr std::array<std::pair<std::string_view, uint64_t>, 14>
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

  for (const auto& multiplier : multipliers) {
    if (multiplier.first == unit) {
      return multiplier.second;
    }
  }

  throw std::invalid_argument("invalid size unit: " + std::string(unit));
}

}  // namespace

uint64_t parseByteSize(std::string_view value)
{
  const auto unitOffset = value.find_first_not_of("0123456789");
  const auto number = value.substr(0, unitOffset);
  const auto unit = unitOffset == std::string_view::npos
                        ? std::string_view()
                        : value.substr(unitOffset);

  uint64_t size = 0;
  const auto parseResult
      = std::from_chars(number.data(), number.data() + number.size(), size);
  if (number.empty() || parseResult.ec != std::errc()
      || parseResult.ptr != number.data() + number.size()) {
    throw std::invalid_argument("invalid size value: " + std::string(value));
  }

  const auto multiplier = getUnitMultiplier(unit);
  if (size == 0 || size > std::numeric_limits<uint64_t>::max() / multiplier) {
    throw std::invalid_argument("size must be positive and fit in uint64_t: "
                                + std::string(value));
  }

  return size * multiplier;
}
