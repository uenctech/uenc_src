/*
 * @Author: your name
 * @Date: 2021-01-14 17:59:45
 * @LastEditTime: 2021-02-24 13:00:40
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \ebpc\net\define.h
 */

#ifndef _DEFINE_H_
#define _DEFINE_H_

#include <cstdint>
#include <string>

#define RESET "\033[0m"
#define BLACK "\033[30m"	/* Black */
#define RED "\033[31m"		/* Red */
#define GREEN "\033[32m"	/* Green */
#define YELLOW "\033[33m"	/* Yellow */
#define BLUE "\033[34m"		/* Blue */

//Constant definition  
//---------------------------------------------------------
constexpr int END_FLAG              = 7777777;             
constexpr int IP_LEN				= 16;	//IP length 
			  
constexpr int MAXEPOLLSIZE			= 100000;
constexpr int MAXLINE				= 10240l;
			   
constexpr int K_ID_LEN				= 160;	//I D length 
constexpr int K_REFRESH_TIME		= 5*10;

constexpr int HEART_TIME      =  60;  // How much time has passed since the last time the data was transmitted and no new message was received, it is judged to start the detection ，
constexpr int HEART_INTVL     =  100;   // How often does the test start to send a heartbeat packet ，
constexpr int HEART_PROBES    =  6;   // Send a few heartbeat packets and the other party does not respond, then close the connection ，



//Alias definition 
//---------------------------------------------------------
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
//---------------------------------------------------------


//Network package body 
typedef struct net_pack
{
	uint32_t	len				= 0;
	std::string	data			= "";
	uint32_t   	checksum		= 0;
	uint32_t	flag			= 0;
	uint32_t    end_flag        = END_FLAG;
}net_pack;

#endif