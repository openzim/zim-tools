#include <sstream>

#include "gtest/gtest.h"

TEST(ZimMountTest, basic)
{
    std::stringstream ss;
    ss << "Hello, world!";
    EXPECT_EQ(ss.str(), "Hello, world!");
    
}
