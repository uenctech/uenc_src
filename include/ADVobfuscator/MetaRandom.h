


















#ifndef MetaRandom_h
#define MetaRandom_h






#include <random>

namespace andrivet { namespace ADVobfuscator {

namespace
{
    

    constexpr char time[] = __TIME__; 

    
    constexpr int DigitToInt(char c) { return c - '0'; }
    const int seed = DigitToInt(time[7]) +
                     DigitToInt(time[6]) * 10 +
                     DigitToInt(time[4]) * 60 +
                     DigitToInt(time[3]) * 600 +
                     DigitToInt(time[1]) * 3600 +
                     DigitToInt(time[0]) * 36000;
}






template<int N>
struct MetaRandomGenerator
{
private:
    static constexpr unsigned a = 16807;        
    static constexpr unsigned m = 2147483647;   

    static constexpr unsigned s = MetaRandomGenerator<N - 1>::value;
    static constexpr unsigned lo = a * (s & 0xFFFF);                
    static constexpr unsigned hi = a * (s >> 16);                   
    static constexpr unsigned lo2 = lo + ((hi & 0x7FFF) << 16);     
    static constexpr unsigned hi2 = hi >> 15;                       
    static constexpr unsigned lo3 = lo2 + hi;

public:
    static constexpr unsigned max = m;
    static constexpr unsigned value = lo3 > m ? lo3 - m : lo3;
};

template<>
struct MetaRandomGenerator<0>
{
    static constexpr unsigned value = seed;
};




template<int N, int M>
struct MetaRandom
{
    static const int value = MetaRandomGenerator<N + 1>::value % M;
};

}}

#endif
