


















#ifndef Log_h
#define Log_h

#include <iomanip>
#include <iostream> 

namespace andrivet { namespace ADVobfuscator {



struct HexChar
{
    unsigned char c_;
    unsigned width_;
    HexChar(unsigned char c, unsigned width) : c_{c}, width_{width} {}
};

inline std::ostream& operator<<(std::ostream& o, const HexChar& c)
{
    return (o << std::setw(c.width_) << std::setfill('0') << std::hex << (int)c.c_ << std::dec);
}

inline HexChar hex(char c, int w = 2)
{
    return HexChar(c, w);
}

}}

#if (defined(DEBUG) && DEBUG == 1) || (defined(ADVLOG) && ADVLOG == 1)
#define LOG(str) std::cerr << str << std::endl
#else
#define LOG(str) ((void)0)
#endif

#endif
