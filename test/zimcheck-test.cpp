#include "gtest/gtest.h"

#include "zim/zim.h"
#include "zim/file.h"
#include "../src/zimfilechecks.h"


TEST(zimfilechecks, test_checksum)
{
    std::string fn = "data/zimfiles/wikibooks_be_all_nopic_2017-02.zim";

    zim::File file(fn);
    ErrorLogger logger;
    
    test_checksum(file, logger);

    ASSERT_TRUE(logger.overalStatus());
    

}

TEST(zimfilechecks, test_metadata)
{
    std::string fn = "data/zimfiles/wikibooks_be_all_nopic_2017-02.zim";

    zim::File file(fn);
    ErrorLogger logger;
    
    test_metadata(file, logger);

    ASSERT_TRUE(logger.overalStatus());
    

}

TEST(zimfilechecks, test_favicon)
{
    std::string fn = "data/zimfiles/wikibooks_be_all_nopic_2017-02.zim";

    zim::File file(fn);
    ErrorLogger logger;
    
    test_favicon(file, logger);

    ASSERT_TRUE(logger.overalStatus());
}

TEST(zimfilechecks, test_mainpage)
{
    std::string fn = "data/zimfiles/wikibooks_be_all_nopic_2017-02.zim";

    zim::File file(fn);
    ErrorLogger logger;
    
    test_mainpage(file, logger);

    ASSERT_TRUE(logger.overalStatus());
}

TEST(zimfilechecks, test_articles)
{
    std::string fn = "data/zimfiles/wikibooks_be_all_nopic_2017-02.zim";

    zim::File file(fn);
    ErrorLogger logger;
    ProgressBar progress(1);

    
    test_articles(file, logger, progress, true, true, true ,true);

    ASSERT_TRUE(logger.overalStatus());
}
