
#ifndef _DEFINE_H_
#define _DEFINE_H_

#include <cstdint>
#include <string>

#define RESET "\033[0m"
#define BLACK "\033[30m"	
#define RED "\033[31m"		
#define GREEN "\033[32m"	
#define YELLOW "\033[33m"	
#define BLUE "\033[34m"		



constexpr int END_FLAG              = 7777777;             
constexpr int IP_LEN				= 16;	
			  
constexpr int MAXEPOLLSIZE			= 100000;
constexpr int MAXLINE				= 10240l;
			   
constexpr int K_ID_LEN				= 160;	
constexpr int K_REFRESH_TIME		= 5*10;

constexpr int HEART_TIME      =  30;  
constexpr int HEART_INTVL     =  100;   
constexpr int HEART_PROBES    =  6;   





typedef int int32;
typedef unsigned int uint32;
typedef unsigned char uint8;
typedef char int8;

using u64 = std::uint64_t;
using u32 = std::uint32_t;
using u16 = std::uint16_t;
using u8 = std::uint8_t;

using i64 = std::int64_t;

using nll = long long;
using ull = unsigned long long;




typedef struct net_pack
{
	uint32_t	len				= 0;
	std::string	data			= "";
	uint32_t   	checksum		= 0;
	uint32_t    end_flag        = END_FLAG;
}net_pack;

#endif