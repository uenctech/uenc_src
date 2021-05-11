/*
 * @Author: your name
 * @Date: 2021-03-27 13:58:00
 * @LastEditTime: 2021-03-27 14:03:34
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \ebpc\common\version.h
 */


#ifndef _VERSION_H_
#define _VERSION_H_
#include <string>


const std::string g_LinuxCompatible = "1.5";
const std::string g_WindowsCompatible = "1.0";
const std::string g_IOSCompatible = "4.0.4";
const std::string g_AndroidCompatible = "3.1.0";


typedef enum CAVERSION
{
    kUnknown = 0,
    kLINUX   = 1,        // linux版本前缀
    kWINDOWS = 2,        // windows版本前缀
    kIOS     = 3,        // ios版本前缀
    kANDROID = 4,        // android版本前缀
} Version;

/* ====================================================================================  
 # @description:  获取版本号
 # @param  NONE
 # @return 返回版本
 # @Mark 返回结构为3部分组成，以下划线分割，第一部分为系统号，第二部分为版本号，第三部分为运行环境
 # 例如：1_0.3_t,
 # 系统号：1为linux，2为windows，3为iOS，4为Android
 # 版本号： 两级版本号如1.0
 # 运行环境：m为主网，t为测试网
 ==================================================================================== */
std::string getVersion();
std::string getEbpcVersion();
/* ====================================================================================  
 # @description: 获取系统类型
 # @param  NONE
 # @return 返回为系统号
 # @mark
 # 系统号：1为linux，2为windows，3为iOS，4为Android
 ==================================================================================== */
Version getSystem();


#endif // !_VERSION_H_