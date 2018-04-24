
void getLinks(std::string page, std::vector <std::string> *links)           //Returns a vector of the links in a particular page. includes links under 'href' and 'src'
{
    int sz=page.size();
    links->clear();
    int startingPoint,length;
    for(int i = 0; i< sz; i++)
    {
        if(page[i] == ' ' && i+5 < sz)
        {
            if((page[i+1] == 'h') && (page[i+2] == 'r') && (page[i+3] =='e') && (page[i+4]=='f'))      //Links under 'href' category.
            {
                i += 5;
                while(page[i] != '=')
                    i++;
                while(page[i] != '"')
                    i++;
                startingPoint= ++i;
                while(page[i] != '"')
                {
                    i++;
                }
                length=i-startingPoint;
                links->push_back(page.substr(startingPoint,length));
            }

            if( (page[i+1] == 's') && (page[i+2] == 'r') && (page[i+3]=='c'))      //Links under 'src' category.
            {
                i += 4;
                while(page[i] != '=')
                    i++;
                while(page[i] != '"')
                    i++;
                startingPoint= ++i;

                while(page[i] != '"')
                {
                    i++;
                }
                length=i-startingPoint;
                links->push_back(page.substr(startingPoint,length));
            }
        }
    }
}

void getDependencies(std::string page, std::vector <std::string> *links)           //Returns a vector of the links in a particular page. includes links under 'href' and 'src'
{
    int sz=page.size();
    links->clear();
    int startingPoint,length;
    for(int i = 0; i< sz; i++)
    {
        if(page[i] == ' ' && i+5 < sz)
        {
            if( (page[i+1] == 's') && (page[i+2] == 'r') && (page[i+3]=='c'))      //Links under 'src' category.
            {
                i += 4;
                while(page[i] != '=')
                    i++;
                while(page[i] != '"')
                    i++;
                startingPoint= ++i;

                while(page[i] != '"')
                {
                    i++;
                }
                length=i-startingPoint;
                links->push_back(page.substr(startingPoint,length));
            }
        }
    }
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

inline bool isExternalUrl(std::string *input_string)       //Checks if an external URL is a wikipedia URL.
{
    if(std::regex_match( *input_string,std::regex(".*.wikipedia.org/.*")))
        return false;

    if(std::regex_match( *input_string,std::regex("/./.*")))
        return false;

    if(std::regex_match( *input_string,std::regex(".*.wikimedia.org/.*")))
        return  false;
    return true;
}

inline bool isInternalUrl(std::string *input_string)                 //Checks if a URL is an internal URL or not. Uses RegExp.
{
    if(std::regex_match( *input_string,std::regex("/./.*")))
        return true;
    else
        return false;
}

//Removes extra spaces from URLs. Usually done by the browser, so web authors sometimes tend to ignore it.
//Converts the %20 to space.Essential for comparing URLs.

std::string process_links(std::string input)
{
    std::string output;
    output.clear();
    int pos = 0;

    //URL Decoding.
    char ch;
    unsigned int i, ii;
    for (i=0; i < input.size(); i++)
    {
        if ( int ( input[i] ) == 37)
        {
            sscanf( input.substr( i+1 , 2 ).c_str(), "%x", &ii);
            ch=static_cast<char>(ii);
            output +=ch;
            i = i+2;
        }
        else
        {
            output += input[i];
        }
    }
    int k=output.rfind("#");
    return output.substr( 0 , k);
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
             "-H , --help            Displays Help\n"
             "examples:\n"
             "./zimcheck -A wikipedia.zim\n"
             "./zimcheck --checksum --redundant wikipedia.zim\n"
             "./zimcheck -F -R wikipedia.zim\n"
             "./zimcheck -M --favicon wikipedia.zim\n";
    return;
}


