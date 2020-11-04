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
    
    
    
    
    x_uint64_t getNtpTimestamp(bool is_sync = false);

    
    x_uint64_t getNtpTimestampConf();

    
    x_uint64_t getlocalTimestamp();

    
    x_uint64_t getTimestamp();

    
    
    
    bool setLocalTime(x_uint64_t timestamp);

    
    void testNtpDelay();

	
};


#endif