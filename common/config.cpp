#include <iostream>
#include <fstream>
#include <tuple>
#include "./config.h"
#include "../net/ip_port.h"
#include "../include/logging.h"
#include "../net/peer_node.h"
#include "../ca/ca_global.h"
#include "version.h"

std::string Config::GetAbsopath()
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

bool Config::NewFile(std::string strFile)
{
    	// If the file exists, creating will overwrite the original file 
   
    ofstream file( strFile.c_str(), fstream::out );
    if( file.fail() )
    {
        debug(" file_path = %s", strFile.c_str());
		return false;
    }
	//json format website ：http://www.bejson.com/jsonviewernew/
    //If you want to add a new configuration, go to the above website to format it, and then format it back. 
    std::string jsonstr;
    if(g_testflag == 1)
    {
        jsonstr = "{\"version\":\"1.0\",\"target_version\":\"\",\"loglevel\":\"FATAL\",\"logFilename\":\"log.txt\",\"sync_data_count\":200,\"http_port\":11190,\"SyncDataPollTime\":100,\"script_rt_price_ip\":\"47.105.219.186\",\"client\":{\"iOSMinVersion\":\"4.0.4\",\"AndroidMinVersion\":\"3.1.0\"},\"node\":[{\"local\":\"华南\",\"info\":[{\"name\":\"深圳\",\"ip\":\"120.79.216.93\",\"port\":\"11188\",\"enable\":\"1\"}]},{\"local\":\"成都\",\"info\":[{\"name\":\"成都\",\"ip\":\"47.108.52.94\",\"port\":\"11188\",\"enable\":\"1\"}]}],\"is_public_node\":false,\"proxy_server\":{\"proxy_id\":\"\",\"type\":0},\"server\":[{\"IP\":\"47.108.52.94\",\"PORT\":11188},{\"IP\":\"120.79.216.93\",\"PORT\":11188}],\"var\":{\"k_bucket\":\"\",\"k_refresh_time\":100,\"local_ip\":\"\",\"local_port\":11188,\"work_thread_num\":2},\"DevicePassword\":\"3a943223670994b064f0acad9dfc220a4abc5edc9b697676ca93413884ebbb99\"}";
    }
    else
    {
        jsonstr = "{\"version\":\"1.0\",\"target_version\":\"\",\"loglevel\":\"FATAL\",\"logFilename\":\"log.txt\",\"sync_data_count\":200,\"http_port\":11190,\"SyncDataPollTime\":100,\"script_rt_price_ip\":\"47.105.219.186\",\"client\":{\"iOSMinVersion\":\"4.0.4\",\"AndroidMinVersion\":\"3.1.0\"},\"node\":[{\"local\":\"华北\",\"info\":[{\"name\":\"青岛\",\"ip\":\"47.104.254.152\",\"port\":\"11187\",\"enable\":\"1\"}]},{\"local\":\"华东\",\"info\":[{\"name\":\"杭州\",\"ip\":\"114.55.102.14\",\"port\":\"11187\",\"enable\":\"1\"}]},{\"local\":\"华南\",\"info\":[{\"name\":\"深圳\",\"ip\":\"47.106.247.169\",\"port\":\"11187\",\"enable\":\"1\"}]},{\"local\":\"成都\",\"info\":[{\"name\":\"成都\",\"ip\":\"47.108.65.199\",\"port\":\"11187\",\"enable\":\"1\"}]}],\"is_public_node\":false,\"proxy_server\":{\"proxy_id\":\"\",\"type\":0},\"server\":[{\"IP\":\"47.104.254.152\",\"PORT\":11187},{\"IP\":\"114.55.102.14\",\"PORT\":11187},{\"IP\":\"47.106.247.169\",\"PORT\":11187},{\"IP\":\"47.108.65.199\",\"PORT\":11187}],\"var\":{\"k_bucket\":\"\",\"k_refresh_time\":100,\"local_ip\":\"\",\"local_port\":11187,\"work_thread_num\":2},\"DevicePassword\":\"3a943223670994b064f0acad9dfc220a4abc5edc9b697676ca93413884ebbb99\"}";
    }
    auto json = nlohmann::json::parse(jsonstr);
    file << json.dump(4);
    file.close();
    return true;
}

void Config::WriteFile(const std::string &name )
{
    std::ofstream fconf;
    std::string fileName ;

    fileName = this->GetAbsopath() + name;
    
    fconf.open( fileName.c_str() );
    fconf << this->m_Json.dump(4);
    fconf.close();
}

void Config::WriteServerToFile(const std::string &name )
{
    std::ofstream fconf;
    std::string fileName ;

    fileName = this->GetAbsopath() + name;
    
    fconf.open( fileName.c_str() );
    fconf << this->m_UpdateJson.dump(4);
    fconf.close();
}

bool Config::InitFile(const std::string &name)
{
    std::ifstream fconf;
    std::string conf;
    std::string tmp = this->GetAbsopath();
    tmp += name;
    if (access(tmp.c_str(), 0) < 0)
    {
        if (false == NewFile(tmp) )
        {
            error("Invalid file...  Please check if the configuration file exists!");
            return false;
        }
    }
    fconf.open(tmp.c_str());
    fconf >> this->m_Json;
    if (!VerifyConf(this->m_Json))
    {
        error("Invalid file...  Lack of necessary fields!");
        fconf.close();
        return false;
    }
    fconf.close();
    return true;
}

bool Config::GetIsPublicNode()
{
    return this->m_Json["is_public_node"].get<bool>();
}

/*If not defined, the default is false */
bool Config::GetCond(const std::string &cond_name)
{
    auto ret = this->m_Json["cond"][cond_name.c_str()].get<bool>();
    return ret;
}

std::string Config::GetVarString(const std::string &cond_name)
{
    auto ret = this->m_Json["var"][cond_name.c_str()].get<std::string>();
    return ret;
}

int Config::GetVarInt(const std::string &cond_name)
{
    auto ret = this->m_Json["var"][cond_name.c_str()].get<int>();
    return ret;
}

std::vector<std::tuple<std::string,int>> Config::GetServerList()
{
    std::vector<std::tuple<std::string,int>> vec;
    for(auto server : m_servers)
    {
        vec.push_back( std::make_tuple(server["IP"].get<std::string>(), server["PORT"].get<int>()) );
    }
    return vec;
}


std::string Config::GetKID()
{
    return this->m_Json["var"]["k_bucket"].get<std::string>();
}

std::string Config::GetLocalIP()
{
    return this->m_Json["var"]["local_ip"].get<std::string>();
}

int Config::GetLocalPort()
{
    return this->m_Json["var"]["local_port"].get<int>();
}

string Config::GetBucket()
{
    return this->m_Json["var"]["k_bucket"].get<std::string>();
}

bool Config::SetKID(std::string strKID)
{
    this->m_Json["var"]["k_bucket"] = strKID;
    WriteFile();
    return true;
}

//Set up local IP 
bool Config::SetLocalIP(const std::string strIP)
{
    this->m_Json["var"]["local_ip"] = strIP;
    WriteFile();
    return true;
}

std::string Config::GetDevPassword()
{
    auto ret = this->m_Json["DevicePassword"].get<std::string>();
    return ret;    
}

bool Config::SetDevPassword(const std::string & password)
{
    if (password.size() == 0)
    {
        return false;
    }
    this->m_Json["DevicePassword"] = password;
    WriteFile();
    return true;
}


/*Dynamically load configuration files */
void Config::Reload(const std::string &name)
{
    std::ifstream fconf;
    std::string tmp = this->GetAbsopath();
	nlohmann::json tmp_CJSon;
    tmp += name;
    if (access(tmp.c_str(), 0) < 0)
    {
        error("Invalid file...  Please check if the configuration file exists!");
        return ;
    }
    fconf.open(tmp.c_str());
    fconf >> tmp_CJSon;
    if (!VerifyConf(tmp_CJSon))
    {
        error("Invalid file...  Lack of necessary fields!");
        fconf.close();
        return ;
    }
    fconf.close();
    this->m_Json = tmp_CJSon;
}

/*Check whether the configuration file has required configuration */
bool Config::VerifyConf(nlohmann::json &target)
{
    for(auto i:target["server"])
    {
        debug("Set IP : %s",(i["IP"].get<std::string>()).c_str());
        debug("Set PORT : %d",(i["PORT"].get<u16>()));
        m_servers.push_back(i);
    }
    return true;
}

///////////////////////////////////////////////////////
const char * KCfgFile = "./config.json";
const char * kCfgJSonKeyVersion = "version";
const char * kCfgJSonKeyEnable = "enable";
const char * kcfgJSonLogLevel = "loglevel";
const char * kcfgJSonLogFilename = "logFilename";
const char * kcfgJSonSyncData = "SyncData";
const char * kCfgJSonSyncDataCount = "sync_data_count"; 
const char * kCfgJSonSyncDataPollTime = "SyncDataPollTime";
const char * kCfgJSonPyRtPriceIp = "script_rt_price_ip";


const char * kCfgJSonKeyFlag = "flag";
const char * kCfgJSonKeyTargetVersion = "target_version";
const char * kCfgJSonKeyVersionServer = "version_server";

// client
const char * kCfgJSonKeyClient = "client";
const char * kCfgJSonKeyiOSMinVersion = "iOSMinVersion";
const char * kCfgJSonKeyAndroidMinVersion = "AndroidMinVersion";


// node
const char * kCfgJNode = "node";
const char * kCfgJNodeLocal = "local";
const char * kCfgJNodeInfo = "info";
const char * kCfgJNodeName = "name";
const char * kCfgJNodeIp = "ip";
const char * kCfgJNodePort = "port";
const char * kCfgJNodeEnable = "enable";



const int kCfgSyncDataCount = 500;
const int kCfgSyncDataPollTime = 60;
const char * kCfgPyScriptIp = "47.105.219.186";
const int kCfgStartSyncDelay = 60;


const char * kCfgServer = "server";
const char * kCfgServerIp = "IP";
const char * kCfgServerPort = "PORT";

const char * kCfgHttpCallback = "http_callback";
const char * kCfgHttpCallbackIp = "ip";
const char * kCfgHttpCallbackPort = "port";

std::string Config::GetVersion()
{
    auto ret = this->m_Json[kCfgJSonKeyVersion].get<std::string>();
    return ret;
}

std::string Config::GetTargetVersion()
{
    auto ret = this->m_Json[kCfgJSonKeyTargetVersion].get<std::string>();
    return ret;
}

std::string Config::GetVersionServer()
{
    auto ret = this->m_Json[kCfgJSonKeyVersionServer].get<std::string>();
    return ret;
}

std::string Config::GetClientVersion(ClientType clientType)
{
    std::string ret;
    if (clientType == kClientType_iOS)
    {
        ret = this->m_Json[kCfgJSonKeyClient][kCfgJSonKeyiOSMinVersion].get<std::string>();
    }
    else if (clientType == kClientType_Android)
    {
        ret = this->m_Json[kCfgJSonKeyClient][kCfgJSonKeyAndroidMinVersion].get<std::string>();
    }
    return ret;
}

std::string Config::GetLogLevel()
{
    auto ret = this->m_Json[kcfgJSonLogLevel].get<std::string>();
    return ret;
}

std::string Config::GetLogFilename()
{
    auto ret = this->m_Json[kcfgJSonLogFilename].get<std::string>();
    return ret;
}

unsigned int Config::GetSyncDataCount()
{
    // return 50; /// Temporary adjustment 

    auto ret = this->m_Json[kCfgJSonSyncDataCount].get<int>();
    return ret;
}

int Config::GetHttpPort()
{
    auto ret = this->m_Json["http_port"].get<int>();
    return ret;
}

unsigned int Config::GetSyncDataPollTime()
{
    auto ret = this->m_Json[kCfgJSonSyncDataPollTime].get<int>();
    return ret;
}

std::string Config::GetNodeInfo()
{
    auto ret = this->m_Json[kCfgJNode].dump(4);
    return ret;   
}

std::string Config::GetPyRtPirceIp()
{
    auto ret = this->m_Json[kCfgJSonPyRtPriceIp].get<std::string>();
    return ret; 
}

std::vector<node_info> Config::GetNodeInfo_s()
{   
    std::vector<node_info> vec;
    for(auto node:m_Json[kCfgJNode])
    {
        for(auto info:node[kCfgJNodeInfo])
        {
            node_info node;
            node.name = info[kCfgJNodeName].get<std::string>();
            node.ip = info[kCfgJNodeIp].get<std::string>();
            node.port = info[kCfgJNodePort].get<u16>();
            node.enable = info[kCfgJNodeEnable].get<std::string>();
            vec.push_back(node);
        }
    }
    return vec;
} 

//Update configuration file 
//Parameters: 1. Update flag 2. Target version number 3. Version server ip 4. Current version number 
bool Config::UpdateConfig(int flag,const std::string &target_version, const std::string &ip,const std::string &latest_version)
{
    if (latest_version != "")   //Equal to empty without updating 
    {
        this->m_Json[kCfgJSonKeyVersion] = latest_version;
    }
    if (ip != "not") //Equal to not not updated 
    {
        this->m_Json[kCfgJSonKeyVersionServer] = ip;
    }
    if (flag != 2)  //Equal to 2 don't update 
    {
        this->m_Json[kCfgJSonKeyFlag] = flag;
    }
    if (target_version != "not")    //Equal to not not updated 
    {
        this->m_Json[kCfgJSonKeyTargetVersion] = target_version;
    }

    WriteFile();
    return true;
}

int Config::ClearNode()
{
    m_UpdateJson[kCfgJNode].clear();

    return 0;
}

// Declare: add new node to config, add 20201111
int Config::AddNewNode(const node_info& node)
{
    nlohmann::json jsonNodeInfo;
    jsonNodeInfo[kCfgJNodeIp] = node.ip;
    jsonNodeInfo[kCfgJNodePort] = std::to_string(node.port);
    jsonNodeInfo[kCfgJNodeName] = node.name;
    jsonNodeInfo[kCfgJNodeEnable] = node.enable;

    nlohmann::json jsonNode;
    jsonNode[kCfgJNodeInfo].push_back(jsonNodeInfo);
    jsonNode[kCfgJNodeLocal] = node.name;

    m_UpdateJson[kCfgJNode].push_back(jsonNode);

    return 0;
}

int Config::ClearServer()
{
    m_UpdateJson[kCfgServer].clear();
    return 0;
}

int Config::AddNewServer(const std::string& ip, unsigned short port)
{
    nlohmann::json jsonServerInfo;
    jsonServerInfo[kCfgServerIp] = ip;
    jsonServerInfo[kCfgServerPort] = port;

    m_UpdateJson[kCfgServer].push_back(jsonServerInfo);

    return 0;
}

std::string Config::GetProxyID()
{    
    string id;
    try
    {
        id = this->m_Json["proxy_server"]["proxy_id"].get<std::string>();
    }
    catch(const std::exception& e)
    {
        id = "";
    }
 
    return id;
} 

Type  Config::GetProxyTypeStatus()
{
    Type type = kNONE;
    try
    {
        type = Type(this->m_Json["proxy_server"]["type"].get<int>());
    }
    catch(const std::exception& e)
    {
        type = kNONE;
    }
    return type;
}


void  Config::UpdateNewConfig(const std::string &name)
{
    std::ofstream fconf;
    std::string fileName ;
    fileName = this->GetAbsopath() + name;
    fconf.open( fileName.c_str() );

    auto k_bucket = GetVarString("k_bucket");
    auto local_ip = GetVarString("local_ip");
    auto local_port = GetLocalPort();
    auto nodelist_refresh_time = GetVarInt("k_refresh_time");
    auto work_thread_num = GetVarInt("work_thread_num");
    this->m_UpdateJson["var"]["k_bucket"] = k_bucket;
    this->m_UpdateJson["var"]["local_ip"] = local_ip;
    this->m_UpdateJson["var"]["local_port"] = local_port;
    this->m_UpdateJson["var"]["k_refresh_time"] = nodelist_refresh_time;
    this->m_UpdateJson["var"]["work_thread_num"] = work_thread_num;

    auto port = GetHttpPort();
    this->m_UpdateJson["http_port"] = port ;

    auto id =  GetProxyID();
    this->m_UpdateJson["proxy_server"]["proxy_id"] = id;
    auto type =  GetProxyTypeStatus();
    this->m_UpdateJson["proxy_server"]["type"] = type;
    
    auto sync_data_count =  GetSyncDataCount();
    this->m_UpdateJson[kCfgJSonSyncDataCount] = sync_data_count;

    auto  polltime =  GetSyncDataPollTime();
    this->m_UpdateJson[kCfgJSonSyncDataPollTime] = polltime;

    std::string ios = GetClientVersion(kClientType_iOS);
    std::string android = GetClientVersion(kClientType_Android);
    this->m_UpdateJson[kCfgJSonKeyClient][kCfgJSonKeyiOSMinVersion] = ios;
    this->m_UpdateJson[kCfgJSonKeyClient][kCfgJSonKeyAndroidMinVersion] = android;

    auto ispublic = GetIsPublicNode();
    this->m_UpdateJson["is_public_node"] = ispublic;
    auto logfile = GetLogFilename();
    this->m_UpdateJson[kcfgJSonLogFilename] = logfile;
    auto loglevel = GetLogLevel();
    this->m_UpdateJson[kcfgJSonLogLevel] = loglevel;

    auto cbport = GetHttpCallbackPort();
    auto cbip = GetHttpCallbackIp();
    this->m_UpdateJson[kCfgHttpCallback][kCfgHttpCallbackIp] = cbip;
    this->m_UpdateJson[kCfgHttpCallback][kCfgHttpCallbackPort] = cbport;

    string EbpcVersion = getEbpcVersion();
    this->m_UpdateJson[kCfgJSonKeyVersion] = EbpcVersion;
    
    this->m_UpdateJson[kCfgJNode] =  this->m_Json[kCfgJNode];
    this->m_UpdateJson[kCfgServer] = this->m_Json[kCfgServer];
    
    fconf << this->m_UpdateJson.dump(4);
    fconf.close();
    return ;
}

bool Config::Removefile()
{
    string path = GetAbsopath();
    string newname = path +"config.json";
    
    const char *trace = newname.c_str();
	if(remove(trace) == 0)
    {  
        cout<<"successfully deleted "<<endl;
        return true;     
    }
    else 
    {
        cout<<"failed to delete "<<endl;  
        return false;
    }
}
   
bool Config::RenameConfig()
{
     string path = GetAbsopath();
    string newname = path +"config.json";
    string oldrename = path +"newconfig.json";
    
    const char * oldname = oldrename.c_str();
    const char *trace = newname.c_str();
    if(rename(oldname,trace) ==0)
    {
        cout<<"rename(oldname,trace)true"<<endl;
        return true;
    }
    else
    {
        cout<<"rename(oldname,trace)false"<<endl;
        return false;
    } 
}

std::string Config::GetHttpCallbackIp()
{
    std::string ip;
    try
    {
        ip = this->m_Json[kCfgHttpCallback][kCfgHttpCallbackIp].get<std::string>();
    }
    catch(const std::exception& e)
    {
        ip = "";
    }    
    return ip;
}

int Config::GetHttpCallbackPort()
{
    int port = 0;
    try
    {
        port = this->m_Json[kCfgHttpCallback][kCfgHttpCallbackPort].get<int>();
    }
    catch(const std::exception& e)
    {
        port = 0;
    }
 
    return port;
}

bool Config::HasHttpCallback()
{
    std::string ip = GetHttpCallbackIp();
    int port = GetHttpCallbackPort();
    return (!ip.empty() && port > 0);
}
