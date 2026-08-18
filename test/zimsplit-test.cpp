#include <cstdint>
#include <limits>
#include <stdexcept>

#include "../src/zimsplit_size.h"
#include "gtest/gtest.h"

TEST(ZimSplitSize, ParsesBytes)
{
  EXPECT_EQ(parseByteSize("1"), 1U);
  EXPECT_EQ(parseByteSize("2147483648"), 2147483648ULL);
  EXPECT_EQ(parseByteSize("42B"), 42U);
  EXPECT_EQ(parseByteSize("18446744073709551615"),
            std::numeric_limits<uint64_t>::max());
}

TEST(ZimSplitSize, ParsesDecimalUnits)
{
  EXPECT_EQ(parseByteSize("2KB"), 2000U);
  EXPECT_EQ(parseByteSize("3MB"), 3000000U);
  EXPECT_EQ(parseByteSize("4GB"), 4000000000ULL);
  EXPECT_EQ(parseByteSize("1TB"), 1000000000000ULL);
  EXPECT_EQ(parseByteSize("1PB"), 1000000000000000ULL);
  EXPECT_EQ(parseByteSize("1EB"), 1000000000000000000ULL);
}

TEST(ZimSplitSize, ParsesBinaryUnits)
{
  EXPECT_EQ(parseByteSize("2KiB"), 2048U);
  EXPECT_EQ(parseByteSize("3MiB"), 3145728U);
  EXPECT_EQ(parseByteSize("4GiB"), 4294967296ULL);
  EXPECT_EQ(parseByteSize("1TiB"), 1099511627776ULL);
  EXPECT_EQ(parseByteSize("1PiB"), 1125899906842624ULL);
  EXPECT_EQ(parseByteSize("1EiB"), 1152921504606846976ULL);
}

TEST(ZimSplitSize, RejectsZero)
{
  EXPECT_THROW(parseByteSize("0"), std::invalid_argument);
  EXPECT_THROW(parseByteSize("0GB"), std::invalid_argument);
}

TEST(ZimSplitSize, RejectsMalformedValues)
{
  EXPECT_THROW(parseByteSize(""), std::invalid_argument);
  EXPECT_THROW(parseByteSize("-1"), std::invalid_argument);
  EXPECT_THROW(parseByteSize("+1"), std::invalid_argument);
  EXPECT_THROW(parseByteSize("1.5GB"), std::invalid_argument);
  EXPECT_THROW(parseByteSize("1 GB"), std::invalid_argument);
  EXPECT_THROW(parseByteSize("1gb"), std::invalid_argument);
  EXPECT_THROW(parseByteSize("GB"), std::invalid_argument);
  EXPECT_THROW(parseByteSize("1K"), std::invalid_argument);
  EXPECT_THROW(parseByteSize("1Ki"), std::invalid_argument);
}

TEST(ZimSplitSize, RejectsOverflow)
{
  EXPECT_THROW(parseByteSize("18446744073709551616"), std::invalid_argument);
  EXPECT_THROW(parseByteSize("19EB"), std::invalid_argument);
  EXPECT_THROW(parseByteSize("16EiB"), std::invalid_argument);
}
