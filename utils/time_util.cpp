#include <sys/time.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include "./time_util.h"


void Trim(std::string& str, bool bLeft, bool bRight)
{
    static const std::string delims = " \t\r\n";
    if(bRight)
        str.erase(str.find_last_not_of(delims)+1); 
    if(bLeft)
        str.erase(0, str.find_first_not_of(delims)); 
}

TimeUtil::TimeUtil()
{
    
}

TimeUtil::~TimeUtil()
{
    
}

x_uint64_t TimeUtil::getlocalTimestamp()
{
    struct timeval tv;
    gettimeofday( &tv, NULL );
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

x_uint64_t TimeUtil::getTimestamp()
{
    x_uint64_t time = getNtpTimestamp();
    if(time == 0)
    {
        time = getlocalTimestamp();
    }
    return time;
}

x_uint64_t TimeUtil::getNtpTimestampConf()
{
    std::vector< std::string > ntp_host;

	std::ifstream infile;
	infile.open("./ntp_server.conf");  
	if (!infile) {
		std::cout << "unable to open ntp_server.conf" << std::endl;
		return 0;
	}
	std::string temp;
	while (getline(infile, temp)) {
		if (temp.size() < 0) break;
		ntp_host.push_back(temp);
	}

    int err = -1;
    x_uint64_t timev = 0ULL;

    srand((int)time(0));
    for(int i = 0; i < 3; i++)
    {
        std::string host = ntp_host[rand()%(ntp_host.size())];
        
        Trim(host,true,true);      
        err = ntp_get_time(host.c_str(), NTP_PORT, 1000, &timev);
        if (0 == err)
        {
            break;
        }
    }
    infile.close();
    return timev/10;
}

x_uint64_t TimeUtil::getNtpTimestamp(bool is_sync)
{
    std::vector< std::string > ntp_host;
    ntp_host.push_back(std::string("ntp1.aliyun.com"));
    ntp_host.push_back(std::string("ntp2.aliyun.com"));
    ntp_host.push_back(std::string("ntp3.aliyun.com"));
    ntp_host.push_back(std::string("ntp4.aliyun.com"));
    ntp_host.push_back(std::string("ntp5.aliyun.com"));
    ntp_host.push_back(std::string("ntp6.aliyun.com"));
    ntp_host.push_back(std::string("ntp7.aliyun.com"));
    ntp_host.push_back(std::string("time1.cloud.tencent.com"));
    ntp_host.push_back(std::string("time2.cloud.tencent.com"));
    ntp_host.push_back(std::string("time3.cloud.tencent.com"));
    ntp_host.push_back(std::string("time4.cloud.tencent.com"));
    ntp_host.push_back(std::string("time5.cloud.tencent.com"));

    int err = -1;
    x_uint64_t timev = 0ULL;

    srand((int)time(0));
    for(int i = 0; i < 3; i++)
    {
        std::string host = ntp_host[rand()%(ntp_host.size())];
        
        err = ntp_get_time(host.c_str(), NTP_PORT, 1000, &timev);
        if (0 == err)
        {
            break;
        }
    }
    if(err == 0 && is_sync)
    {
        setLocalTime(timev/10);
    }
    return timev/10;
}

bool TimeUtil::setLocalTime(x_uint64_t timestamp)
{
    struct timeval tv;
    tv.tv_sec = timestamp/1000000;
	tv.tv_usec = timestamp - timestamp/1000000*1000000;
	if (settimeofday(&tv, NULL) < 0)
	{
		return false;
	}
	return true;
}


void TimeUtil::testNtpDelay()
{
    int err = -1;

    x_uint64_t xut_timev = 0ULL;

    
    std::vector< std::string > ntp_host;
    ntp_host.push_back(std::string("ntp1.aliyun.com"));
    ntp_host.push_back(std::string("ntp2.aliyun.com"));
    ntp_host.push_back(std::string("ntp3.aliyun.com"));
    ntp_host.push_back(std::string("ntp4.aliyun.com"));
    ntp_host.push_back(std::string("ntp5.aliyun.com"));
    ntp_host.push_back(std::string("ntp6.aliyun.com"));
    ntp_host.push_back(std::string("ntp7.aliyun.com"));
    ntp_host.push_back(std::string("time1.cloud.tencent.com"));
    ntp_host.push_back(std::string("time2.cloud.tencent.com"));
    ntp_host.push_back(std::string("time3.cloud.tencent.com"));
    ntp_host.push_back(std::string("time4.cloud.tencent.com"));
    ntp_host.push_back(std::string("time5.cloud.tencent.com"));
    
    for (std::vector< std::string >::iterator itvec = ntp_host.begin(); itvec != ntp_host.end(); ++itvec)
    {
        xut_timev = 0ULL;
        err = ntp_get_time_test(itvec->c_str(), NTP_PORT, 1000, &xut_timev);
        if (0 != err)
        {
            printf("  %s return error code : %d\n", itvec->c_str(), err);
        }
    }    
}