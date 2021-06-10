#ifndef _CA_CLIENTINFO_H_
#define _CA_CLIENTINFO_H_

#include <string>
#include "../include/clientType.h"

/**
 * @description: Read the data in clientInfo.json 
 * @return: Return the data in the file
 * @mark: If the file does not exist, create a default file and return to the initial data 
 */
std::string ca_clientInfo_read();

/**
 * @description: Get updated data 
 * @param s Upgrade data 
 * @param eClientType Client type 
 * @param language Selected language 
 * @param sVersion Version information returned 
 * @param sDesc Descriptive information returned 
 * @param sDownload Download URL returned 
 * @return: Return 0 on success, return other values on failure 
 */
int ca_getUpdateInfo(const std::string & s, ClientType eClientType, ClientLanguage language, std::string & sVersion, std::string & sDesc, std::string & sDownload);

#endif // !1