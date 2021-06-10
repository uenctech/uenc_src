#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <string>
#include <map>
#include <vector>
#include <tuple>
#include "../utils/json.hpp"
#include "../utils/singleton.h"
#include "../include/clientType.h"
#include "../common/devicepwd.h"
#include<string.h>
#include<unistd.h>
#include<stdio.h>
#include<sys/types.h>
#include<dirent.h>
#include<stdlib.h>

typedef enum ProxyType
{
    kNONE = 0,     // default 
    kMANUAL   = 1,    // Manual proxy 
    kAUTOMUTIC = 2,   // Automatic proxy    
} Type;

//Structure for storing version server node information 
struct node_info
{
    std::string name;
    std::string ip;
    unsigned short port;
    std::string enable;
};

using u32 = std::uint32_t;
using u16 = std::uint16_t;

#define SERVERMAINPORT			(Singleton<Config>::get_instance()->GetLocalPort())

class Config
{
    friend class Singleton<Config>;
public:
    Config(){};
    ~Config(){};
    //Construct a configuration file based on the file name 
    Config(const std::string &name){if(!this->InitFile(name)){_Exit(0);}};

    //Dynamically load configuration files 
    void Reload(const std::string &name = "config.json");
    //Write configuration file according to m_Json 
    void WriteFile(const std::string &name = "config.json");
    void WriteServerToFile(const std::string &name = "config.json");
    //Write configuration file according to m_Json 
   // void WriteDevPwdFile(const std::string &name = "devpwd.json");
    //Initialize the configuration file 
    bool InitFile(const std::string &name = "config.json");
    //Initialize the device password file 
  //  bool InitPWDFile(const std::string &name = "devpwd.json");
   // bool NewDevPWDFile(std::string strFile);
    //Create a configuration file 
    bool NewFile(std::string strFile);

    //******************Net layer related configuration*******************
    //Get whether it is an external network node  is_public_node
    bool GetIsPublicNode();

    //Get the status of the server 
    bool GetCond(const std::string &cond_name);

    //Get the variables defined by the net layer k_bucket、k_refresh_time、local_ip、work_thread_num
    int GetVarInt(const std::string &cond_name);
    std::string GetVarString(const std::string &cond_name);

    //Get and set the k-bucket node ID 
    std::string GetKID();
    bool SetKID(std::string strKID);

    //Get and set the intranet ip address of the node 
    std::string GetLocalIP();
    std::string GetBucket();
    bool SetLocalIP(std::string strIP);
    int GetLocalPort();
    int GetHttpPort();
    std::vector<std::tuple<std::string,int>> GetServerList();

    //******************Ca layer related configuration ***************************

    /**
     * @description: Get the ID of the proxy server 
     * @return: Whether the operation is successful, the corresponding ID value is returned if successful, and an empty string is returned on failure 
     */
    
    std::string GetProxyID();
  /**
     * @description: Get the type of proxy server 
     * @return: Whether the operation is successful, and different types are returned successfully (kAUTOMUTIC = 0,kMANUAL   = 1, kNONE = 2)
     */
   Type  GetProxyTypeStatus();
    /**
     * @description: Return profile version 
     * @param no 
     * @return: Return profile version 
     */
    std::string GetVersion();

    /**
     * @description: Get the update target version number 
     * @param no 
     * @return: Get the update target version number 
     */
    std::string GetTargetVersion();
    //Get information about four version servers 
    std::vector<node_info> GetNodeInfo_s();
    //Update the configuration file, the default value of the parameter means not to update the field 
    bool UpdateConfig(int flag = 2,const std::string &target_version = "not", const std::string &ip = "not", const std::string &latest_version = "");
    //Get the corresponding version server ip of the previous request to update 
    std::string GetVersionServer();

    void UpdateNewConfig(const std::string &name = "newconfig.json");

    /**
     * @description: Return the supported client version number according to clientType 
     * @param clientType
     * @return: Returns the client version number indicated by clientType 
     * @mark The version number returned is the minimum client version supported by ebpc 
     */
    std::string GetClientVersion(ClientType clientType);

    /**
     * @description: Get log level 
     * @param 
     * @return: Returns the log level, if the query fails, an empty string is returned 
     */
    std::string GetLogLevel();

    /**
     * @description: Get the log file name 
     * @param 
     * @return: Return the log file name, or an empty string if the query fails 
     */
    std::string GetLogFilename();

    
    /**
     * @description: Default number of synchronized blocks within time 
     * @param 
     * @return: Returns the number of JSON read blocks, the default is 10 
     */
    unsigned int GetSyncDataCount();
    unsigned int SetSyncDataCount(unsigned int data);
    /**
     * @description:Get synchronization interval 
     * @param 
     * @return:  Returns the synchronization interval 
     */
    unsigned int GetSyncDataPollTime();

    /**
     * @description: Get server node information 
     * @param 
     * @return: Json string of node information 
     */
    std::string GetNodeInfo();

    /**
     * @description: Get device key 
     * @param 
     * @return: Return the stored key 
     */
    std::string GetDevPassword();

    /**
     * @description:  Set device key 
     * @param password Key to be written 
     * @return: If the setting is successful, return true, otherwise return false 
     */
    bool SetDevPassword(const std::string & password);

    /**
     * @description:  Set device key to get py script server ip 
     * @param
     * @return: Return py script server ip 
     */
    std::string GetPyRtPirceIp();

    int ClearNode();
    int AddNewNode(const node_info& node);

    int ClearServer();
    int AddNewServer(const std::string& ip, unsigned short port);
    
     bool Removefile();
     bool RenameConfig();

    std::string GetHttpCallbackIp();
    int GetHttpCallbackPort();
    bool HasHttpCallback();

private:
    std::string GetAbsopath();
    bool VerifyConf(nlohmann::json &target);

private:
	nlohmann::json m_Json;
    nlohmann::json m_UpdateJson;
    std::vector<nlohmann::json> m_servers;
};

#endif