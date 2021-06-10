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
    
    //Get ntp server timestamp, unit: microsecond 1s = 1000000 microseconds 
    //param:
    //  is_sync Whether to synchronize the time obtained from ntp to the local 
    x_uint64_t getNtpTimestamp(bool is_sync = false);

    //Get the ntp server timestamp, read the ntp server from the configuration file, and then request the unit: microseconds 
    x_uint64_t getNtpTimestampConf();

    //Get local timestamp, unit: microsecond     
    x_uint64_t getlocalTimestamp();

    //Get the timestamp, first get it from ntp, if it fails, get it locally 
    x_uint64_t getTimestamp();

    //Set local time 
    //param:
    //  timestamp,Unit: microsecond 
    bool setLocalTime(x_uint64_t timestamp);

    //Test the delay of ntp server acquisition time 
    void testNtpDelay();

	
};


#endif