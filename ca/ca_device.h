#ifndef _CA_DEVICE_H_
#define _CA_DEVICE_H_

#include <string>

/**
 * @description: Generate the key hash value according to the key 
 * @param {type} 
 * @return: 
 */
std::string generateDeviceHashPassword(const std::string & text);

/**
 * @description: Determine whether the device password is correct (only composed of letters and numbers) 
 * @param Password string 
 * @return: Return true only for letters and numbers, otherwise return false 
 */
bool isDevicePasswordValid(const std::string & text);

extern const char * default_pwd;
extern const char * super_pwd;

#endif // !_CA_DEVICE_H_