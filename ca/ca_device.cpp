#include "ca_device.h"
#include "Crypto_ECDSA.h"
#include <string>
#include "../utils/singleton.h"
#include "../utils/string_util.h"

const char * default_pwd = "3a943223670994b064f0acad9dfc220a4abc5edc9b697676ca93413884ebbb99";
const char * super_pwd = "bfdae6bf1b06d6925047294e80357adb957113277826a7726048dd7e87089561";  //fk8eg39z

std::string generateDeviceHashPassword(const std::string & text)
{
    std::string pwd(text);
    Singleton<StringUtil>::get_instance()->Trim(pwd, true, true);
    if (pwd.size() == 0)
    {
        return "";
    }
    
    std::string password = std::string("ebpc_") + pwd;
    password = password + std::string("_ebpc");
    password = getsha256hash(password);
    password = getsha256hash(password);

    return password;
}

bool isDevicePasswordValid(const std::string & text)
{
    std::string pwd(text);
    Singleton<StringUtil>::get_instance()->Trim(pwd, true, true);    
    if (pwd.size() != 8)
    {
        return false;
    }
    
    unsigned int sum = 0;
    for (auto ch : pwd)
    {
        if (isdigit(ch) || isalpha(ch))
        {
            sum++;
        }
    }

    return sum == pwd.size();
}