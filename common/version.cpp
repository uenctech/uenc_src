/*
 * @Author: your name
 * @Date: 2021-03-27 13:58:13
 * @LastEditTime: 2021-03-27 15:04:25
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \ebpc\common\version.cpp
 */

#include "version.h"
#include <sstream>
#include "global.h"

std::string getEbpcVersion()
{
    static std::string version = g_LinuxCompatible;
    return version;
}

std::string  getVersion()
{
    std::string versionNum = getEbpcVersion();
    std::ostringstream ss;
    ss << getSystem();
    std::string version = ss.str() + "_" + versionNum ; 
    if(g_testflag == 1)
    {
        version = version + "_" + "t";
    }
    else 
    {
        version = version + "_" + "p";
    }
    return version;
}

Version getSystem()
{
#if WINDOWS
    return kWINDOWS;
#else
    return kLINUX;
#endif 
}
