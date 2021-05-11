
#include "./string_util.h"


void StringUtil::Trim(std::string& str, bool bLeft, bool bRight)
{
    static const std::string delims = " \t\r\n";
    if(bRight)
        str.erase(str.find_last_not_of(delims)+1); // trim right
    if(bLeft)
        str.erase(0, str.find_first_not_of(delims)); // trim left
}



void StringUtil::SplitString(const std::string& s, std::vector<std::string>& v, const std::string& c)
{
    std::string::size_type pos1, pos2;
    pos2 = s.find(c);
    pos1 = 0;
    while(std::string::npos != pos2)
    {
        v.push_back(s.substr(pos1, pos2-pos1));
        
        pos1 = pos2 + c.size();
        pos2 = s.find(c, pos1);
    }
    if(pos1 != s.length())
        v.push_back(s.substr(pos1));
}


