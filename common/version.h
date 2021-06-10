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
    kLINUX   = 1,        // linux version prefix 
    kWINDOWS = 2,        // windows Version prefix 
    kIOS     = 3,        // ios Version prefix 
    kANDROID = 4,        // android Version prefix 
} Version;

/* ====================================================================================  
 # @description:  Get the version number 
 # @param  NONE
 # @return Return version 
 # @Mark The return structure is composed of 3 parts, separated by underscores, the first part is the system number, the second part is the version number, and the third part is the operating environment 
 # E.g ：1_0.3_t,
 # System number ：1为linux，2为windows，3为iOS，4为Android
 # Version number: Two-level version number such as 1.0
 # Operating environment: m is the main network, t is the test network 
 ==================================================================================== */
std::string getVersion();
std::string getEbpcVersion();
/* ====================================================================================  
 # @description: Get system type 
 # @param  NONE
 # @return Return to the system number
 # @mark
 # System number ：1 is linux, 2 is windows, 3 is iOS, 4 is Android 
 ==================================================================================== */
Version getSystem();


#endif // !_VERSION_H_