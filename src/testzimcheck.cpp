#define ZIMCHECK_TEST

#include "zimcheck.cpp"
#include <cassert>


int main()
{
    // normalize_link() tests
    assert(normalize_link("../../a.js", "A/B/C") == "A/a.js" );


    // getLinks() tests
    auto v = getLinks("");
    assert(v.size() == 0);


    // isExternalUrl tests
    assert(isExternalUrl("tel:123"));

    return 0;
}
