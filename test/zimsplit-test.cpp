#include <cstdint>
#include <limits>
#include <stdexcept>

#include <vector>
#include "testing_helpers.h"

#include "../src/zimsplit_size.h"
#include "gtest/gtest.h"

int zimsplit(const std::vector<const char*>& args);

namespace
{

#define EXPECT_INVALID_ARG_ERROR(statement, expectedError)                      \
  do {                                                                         \
    try {                                                                      \
      statement;                                                               \
      FAIL() << "Expected std::invalid_argument";                              \
    } catch (const std::invalid_argument& error) {                              \
      EXPECT_STREQ(error.what(), expectedError);                                \
    }                                                                          \
  } while (false)

constexpr auto TOO_LARGE_PART_SIZE_ERROR
    = "part size must be smaller than the input ZIM file";

}  // namespace

TEST(ZimSplitSize, ParsesBytes)
{
  EXPECT_EQ(parseByteSize("1"), 1U);
  EXPECT_EQ(parseByteSize("0001"), 1U);
  EXPECT_EQ(parseByteSize("2147483648"), 2147483648ULL);
  EXPECT_EQ(parseByteSize("42B"), 42U);
  EXPECT_EQ(parseByteSize("18446744073709551615"),
            std::numeric_limits<uint64_t>::max());
  EXPECT_EQ(parseByteSize("18446744073709551615b"),
            std::numeric_limits<uint64_t>::max());
}

TEST(ZimSplitSize, ParsesDecimalUnits)
{
  EXPECT_EQ(parseByteSize("2KB"), 2000U);
  EXPECT_EQ(parseByteSize("2kb"), 2000U);
  EXPECT_EQ(parseByteSize("2kB"), 2000U);
  EXPECT_EQ(parseByteSize(" 1 GB "), 1000000000U);
  EXPECT_EQ(parseByteSize("1GB "), 1000000000U);
  EXPECT_EQ(parseByteSize("    1    GB   "), 1000000000U);
  EXPECT_EQ(parseByteSize("3MB"), 3000000U);
  EXPECT_EQ(parseByteSize("4GB"), 4000000000ULL);
  EXPECT_EQ(parseByteSize("1TB"), 1000000000000ULL);
  EXPECT_EQ(parseByteSize("1PB"), 1000000000000000ULL);
  EXPECT_EQ(parseByteSize("1EB"), 1000000000000000000ULL);
  EXPECT_EQ(parseByteSize("18EB"), 18000000000000000000ULL);
}

TEST(ZimSplitSize, ParsesBinaryUnits)
{
  EXPECT_EQ(parseByteSize("2KiB"), 2048U);
  EXPECT_EQ(parseByteSize("2kib"), 2048U);
  EXPECT_EQ(parseByteSize("2KIB"), 2048U);
  EXPECT_EQ(parseByteSize("2kIb"), 2048U);
  EXPECT_EQ(parseByteSize("3MiB"), 3145728U);
  EXPECT_EQ(parseByteSize("4GiB"), 4294967296ULL);
  EXPECT_EQ(parseByteSize("1TiB"), 1099511627776ULL);
  EXPECT_EQ(parseByteSize("1PiB"), 1125899906842624ULL);
  EXPECT_EQ(parseByteSize("1EiB"), 1152921504606846976ULL);
  EXPECT_EQ(parseByteSize("15EiB"), 17293822569102704640ULL);
}

TEST(ZimSplitSize, RejectsZero)
{
  EXPECT_INVALID_ARG_ERROR(parseByteSize("0"), "part size must be positive: 0");
  EXPECT_INVALID_ARG_ERROR(parseByteSize("0GB"),
                           "part size must be positive: 0GB");
}

TEST(ZimSplitSize, RejectsMalformedValues)
{
  EXPECT_INVALID_ARG_ERROR(parseByteSize(""),
                           "invalid size value: empty input");
  EXPECT_INVALID_ARG_ERROR(parseByteSize("   "),
                           "invalid size value: empty input");
  EXPECT_INVALID_ARG_ERROR(parseByteSize("-1"), "invalid size value: -1");
  EXPECT_INVALID_ARG_ERROR(parseByteSize("+1"), "invalid size value: +1");
  EXPECT_INVALID_ARG_ERROR(parseByteSize("1.5GB"),
                           "invalid size unit: .5GB");
  EXPECT_INVALID_ARG_ERROR(parseByteSize("1 G B"), "invalid size unit: G B");
  EXPECT_INVALID_ARG_ERROR(parseByteSize("1GBjunk"),
                           "invalid size unit: GBjunk");
  EXPECT_INVALID_ARG_ERROR(parseByteSize("1e3"), "invalid size unit: e3");
  EXPECT_INVALID_ARG_ERROR(parseByteSize("GB"), "invalid size value: GB");
  EXPECT_INVALID_ARG_ERROR(parseByteSize("1K"), "invalid size unit: K");
  EXPECT_INVALID_ARG_ERROR(parseByteSize("1Ki"), "invalid size unit: Ki");
}

TEST(ZimSplitSize, RejectsOverflow)
{
  EXPECT_INVALID_ARG_ERROR(parseByteSize("18446744073709551616"),
                           TOO_LARGE_PART_SIZE_ERROR);
  EXPECT_INVALID_ARG_ERROR(parseByteSize("19EB"),
                           TOO_LARGE_PART_SIZE_ERROR);
  EXPECT_INVALID_ARG_ERROR(parseByteSize("16EiB"),
                           TOO_LARGE_PART_SIZE_ERROR);
}

TEST(ZimSplitSize, ValidatesPartSizeAgainstArchive)
{
  EXPECT_NO_THROW(validatePartSize(99, 100));
  EXPECT_INVALID_ARG_ERROR(validatePartSize(100, 100),
                           TOO_LARGE_PART_SIZE_ERROR);
  EXPECT_INVALID_ARG_ERROR(validatePartSize(101, 100),
                           TOO_LARGE_PART_SIZE_ERROR);
}


TEST(ZimSplit, RejectsTooSmallSizeWithoutForce)
{
    CapturedStdout out;
    CapturedStderr err;
    int ret = zimsplit({"zimsplit", "--size", "1", "data/zimfiles/good.zim"});
    EXPECT_NE(ret, 0);
    EXPECT_EQ(std::string(err),
              "Error: part size must be at least 104857600 bytes. Use --force to override.\n");
    EXPECT_EQ(std::string(out), "");
}

TEST(ZimSplit, WarnsAndContinuesWithForce)
{
    int ret = zimsplit({"zimsplit", "--size", "1", "--force", "data/zimfiles/good.zim"});
    EXPECT_EQ(ret, 0);
}

TEST(ZimSplit, RejectsInvalidSize)
{
    CapturedStdout out;
    CapturedStderr err;
    int ret = zimsplit({"zimsplit", "--size", "abc", "data/zimfiles/good.zim"});
    EXPECT_NE(ret, 0);
    EXPECT_FALSE(std::string(err).empty());
    EXPECT_EQ(std::string(out), "");
}

TEST(ZimSplit, RejectsZeroSize)
{
    CapturedStdout out;
    CapturedStderr err;
    int ret = zimsplit({"zimsplit", "--size", "0", "data/zimfiles/good.zim"});
    EXPECT_NE(ret, 0);
    EXPECT_FALSE(std::string(err).empty());
    EXPECT_EQ(std::string(out), "");
}

TEST(ZimSplit, NonexistentFile)
{
    CapturedStdout out;
    CapturedStderr err;
    int ret = zimsplit({"zimsplit", "--size", "100000000", "data/zimfiles/nonexistent.zim"});
    EXPECT_NE(ret, 0);
    EXPECT_FALSE(std::string(err).empty());
    EXPECT_EQ(std::string(out), "");
}
