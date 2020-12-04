#ifndef _CA_DEVICE_H_
#define _CA_DEVICE_H_

#include <string>

/**
 * @description: 根据密钥产生密钥hash值
 * @param {type} 
 * @return: 
 */
std::string generateDeviceHashPassword(const std::string & text);

/**
 * @description: 判断设备密码是否正确（仅为字母和数字组成）
 * @param 密码字符串
 * @return: 仅为字母和数字组成返回true，否则返回false
 */
bool isDevicePasswordValid(const std::string & text);

extern const char * default_pwd;
extern const char * super_pwd;

#endif // !_CA_DEVICE_H_