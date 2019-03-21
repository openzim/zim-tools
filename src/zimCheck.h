/*
 * Copyright (C)  Kiran Mathew Koshy
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * is provided AS IS, WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, and
 * NON-INFRINGEMENT.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */


std::vector<std::string> getLinks(const std::string& page, bool withHref = true)           //Returns a vector of the links in a particular page. includes links under 'href' and 'src'
{
    const char* p = page.c_str();
    const char* linkStart;
    std::vector<std::string> links;

    while (*p) {
        if (withHref && strncmp(p, " href", 5) == 0) {
            p += 5;
        } else if (strncmp(p, " src", 4) == 0) {
            p += 4;
        } else {
            p += 1;
            continue;
        }

        while (*p == ' ')
            p += 1 ;
        if (*(p++) != '=')
            continue;
        while (*p == ' ')
            p += 1;
        char delimiter = *p++;
        if (delimiter != '\'' && delimiter != '"')
            continue;

        linkStart = p;
        // [TODO] Handle escape char
        while(*p != delimiter)
            p++;
        links.push_back(std::string(linkStart, p));
        p += 1;
    }
    return links;
}

std::vector<std::string> getDependencies(const std::string& page)           //Returns a vector of the links in a particular page. includes links under 'href' and 'src'
{
    return getLinks(page, false);
}

int adler32(std::string buf)                        //Adler32 Hash Function. Used to hash the BLOB data obtained from each article, for redundancy checks.
{                                                   //Please note that the adler32 hash function has a high number of collisions, and that the hash match is not taken as final.
    unsigned int s1 = 1;
    unsigned int s2 = 0;
    unsigned int sz=buf.size();
    for (size_t n = 0; n <sz; n++)
    {
        s1 = (s1 + buf[n]) % 65521;
        s2 = (s2 + s1) % 65521;
    }
    return (s2 << 16) | s1;
}

inline bool isExternalUrl(const std::string& input_string)
{
    static std::regex external_url_regex = std::regex("[^:/?#]+:\\/\\/.*", std::regex_constants::icase);
    return std::regex_match(input_string, external_url_regex);
}

inline bool isInternalUrl(const std::string& input_string)                 //Checks if a URL is an internal URL or not. Uses RegExp.
{
    return !isExternalUrl(input_string);
}

//Removes extra spaces from URLs. Usually done by the browser, so web authors sometimes tend to ignore it.
//Converts the %20 to space.Essential for comparing URLs.

std::string normalize_link(const std::string& input, const std::string& baseUrl)
{
    std::string output;
    output.reserve(baseUrl.size() + input.size() + 1);

    bool in_query = false;
    bool check_rel = false;
    const char* p = input.c_str();
    if ( *(p) == '/') {
      // This is an absolute url.
      p++;
    } else {
      //This is a relative url, use base url
      output = baseUrl;
      if (output.back() != '/')
          output += '/';
      check_rel = true;
    }

    //URL Decoding.
    while (*p)
    {
        if ( !in_query && check_rel ) {
            if (strncmp(p, "../", 3) == 0) {
                // We must go "up"
                // Remove the '/' at the end of output.
                output.resize(output.size()-1);
                // Remove the last part.
                auto pos = output.find_last_of('/');
                output.resize(pos==output.npos?0:pos);
                // Move after the "..".
                p += 2;
                check_rel = false;
                continue;
            }
            if (strncmp(p, "./", 2) == 0) {
                // We must simply skip this part
                // Simply move after the ".".
                p += 1;
                check_rel = false;
                continue;
            }
        }
        if ( *p == '#' )
            // This is a beginning of the #anchor inside a page. No need to decode more
            break;
        if ( *p == '%')
        {
            char ch;
            sscanf(p+1, "%2hhx", &ch);
            output += ch;
            p += 3;
            continue;
        }
        if ( *p == '?' ) {
            // We are in the query, so don't try to interprete '/' as path separator
            in_query = true;
        }
        if ( *p == '/') {
            check_rel = true;
            if (output.empty()) {
                // Do not add '/' at beginning of output
                p++;
                continue;
            }
        }
        output += *(p++);
    }
    return output;
}

void displayHelp()
{
    std::cout<<"\n"
             "zimcheck\n"
             "Written by : Kiran Mathew Koshy\n"
             "A to check the quality of a ZIM file\n."
             "To list the details of the error reported, add a flag -D.\n"
             "Usage:\n"
             "./zimcheckusage: ./zimcheck [options] zimfile\n"
             "options:\n"
             "-A , --all             run all tests. Default if no flags are given.\n"
             "-C , --checksum        Internal CheckSum Test\n"
             "-M , --metadata        MetaData Entries\n"
             "-F , --favicon         Favicon\n"
             "-P , --main            Main page\n"
             "-R , --redundant       Redundant data check\n"
             "-U , --url_internal    URL check - Internal URLs\n"
             "-X , --url_external    URL check - Internal URLs\n"
             "-E , --mime            MIME checks\n"
             "-D , --details         Details of error\n"
             "-B , --progress        Print progress report\n"
             "-H , --help            Displays Help\n"
             "examples:\n"
             "./zimcheck -A wikipedia.zim\n"
             "./zimcheck --checksum --redundant wikipedia.zim\n"
             "./zimcheck -F -R wikipedia.zim\n"
             "./zimcheck -M --favicon wikipedia.zim\n";
    return;
}


