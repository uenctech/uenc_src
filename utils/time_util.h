#ifndef _TIMEUTIL_H_
#define _TIMEUTIL_H_

#include <string>
#include <fstream>
#include <map>
#include <vector>
#include "VxNtpHelper.h"


class TimeUtil
{
    
public:
    TimeUtil();
    ~TimeUtil();
    
    //获取ntp服务器时间戳,单位：微秒     1s = 1000000微秒
    //param:
    //  is_sync 是否把从ntp获取的时间同步到本地
    x_uint64_t getNtpTimestamp(bool is_sync = false);

    //获取ntp服务器时间戳,从配置文件读取ntp服务器，然后请求 单位：微秒 
    x_uint64_t getNtpTimestampConf();

    //获取本地时间戳，单位：微秒    
    x_uint64_t getlocalTimestamp();

    //获取时间戳，先从ntp获取，若不成功再去本地获取
    x_uint64_t getTimestamp();

    //设置本地时间
    //param:
    //  timestamp,单位：微秒
    bool setLocalTime(x_uint64_t timestamp);

    //测试ntp服务器获得时间的延时
    void testNtpDelay();

	
};


#endif