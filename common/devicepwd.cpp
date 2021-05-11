#include <iostream>
#include <fstream>
#include "./devicepwd.h"
#include "../net/ip_port.h"
#include "../include/logging.h"
#include "../net/peer_node.h"
#include "../ca/ca_global.h"


std::string DevicePwd::GetAbsopath()
{
    int i;
    char buf[1024];
    int len = readlink("/proc/self/exe", buf, 1024 - 1);

    if (len < 0 || (len >= 1024 - 1))
    {
        return "";
    }

    buf[len] = '\0';
    for (i = len; i >= 0; i--)
    {
        if (buf[i] == '/')
        {
            buf[i + 1] = '\0';
            break;
        }
    }
    return std::string(buf);
}

bool DevicePwd::NewDevPWDFile(std::string strFile)
{
    // 如果文件存在 创建会覆盖原文件
    ofstream file( strFile.c_str(), fstream::out );
    if( file.fail() )
    {
        debug(" file_path = %s", strFile.c_str());
		return false;
    }
	//json格式化网站：http://www.bejson.com/jsonviewernew/
    //如果要新添加配置，去上面的网站格式化一下，然后再格式化回来就行 
    std::string jsonstr;
    {
        jsonstr = "{\"DevicePassword\":\"3a943223670994b064f0acad9dfc220a4abc5edc9b697676ca93413884ebbb99\"}";
    }
    auto json = nlohmann::json::parse(jsonstr);
    file << json.dump(4);
    file.close();
    return true;
}


void DevicePwd::WriteDevPwdFile(const std::string &name )
{
    std::ofstream fconf;
    std::string fileName ;

    fileName = this->GetAbsopath() + name;
    
    fconf.open( fileName.c_str() );
    fconf << this->m_Json.dump(4);
    fconf.close();
}

bool DevicePwd::InitPWDFile(const std::string &name)
{
    std::ifstream fconf;
    std::string conf;
    std::string tmp = this->GetAbsopath();
    tmp += name;
    if (access(tmp.c_str(), 0) < 0)
    {
        if (false == NewDevPWDFile(tmp) )
        {
            error("Invalid file...  Please check if the configuration file exists!");
            return false;
        }
    }
    fconf.open(tmp.c_str());
    fconf >> this->m_Json;
    fconf.close();
    return true;
}

std::string DevicePwd::GetDevPassword()
{
    string devpwd;
    try
    {
        devpwd = this->m_Json["DevicePassword"].get<std::string>();
    }
    catch(const std::exception& e)
    {
        cout<<"exception DevicePassword"<<endl;
        devpwd = "";
    }
    return devpwd;    
}

bool DevicePwd::SetDevPassword(const std::string & password)
{
    if (password.size() == 0)
    {
        return false;
    }
    this->m_Json["DevicePassword"] = password;
    WriteDevPwdFile();
    return true;
}

bool DevicePwd::UpdateDevPwdConfig(const std::string &name)
{
    string configversion = Singleton<Config>::get_instance()->GetVersion();
    string version = "1.0";
    if(!configversion.compare(version))
    {
       string password =  Singleton<Config>::get_instance()->GetDevPassword();
       if(SetDevPassword(password))
       {
           return true;
       }
       else
       {
            return false; 
       } 
    }    
    return true;  
}