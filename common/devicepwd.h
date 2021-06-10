#ifndef _DEVICEPWD_H_
#define _DEVICEPWD_H_



#include <string>
#include <map>
#include <vector>
#include <tuple>
#include "../utils/json.hpp"
#include "../utils/singleton.h"
#include "../include/clientType.h"
#include "../common/config.h"

class DevicePwd
{
    friend class Singleton<DevicePwd>;
public:
    DevicePwd(){};
    ~DevicePwd(){};
    //Construct a configuration file based on the file name 
    DevicePwd(const std::string &name){if(!this->InitPWDFile(name)){_Exit(0);}};
    //Write configuration file according to m_Json 
    void WriteDevPwdFile(const std::string &name = "devpwd.json");
   
    //Initialize the device password file 
    bool InitPWDFile(const std::string &name = "devpwd.json");
    bool NewDevPWDFile(std::string strFile);
    std::string GetDevPassword();
    bool SetDevPassword(const std::string & password);
    bool UpdateDevPwdConfig(const std::string &name = "devpwd.json");  
private:
    std::string GetAbsopath();
private:
	nlohmann::json m_Json;
};



#endif