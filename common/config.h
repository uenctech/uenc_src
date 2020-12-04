#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <string>
#include <map>
#include <vector>
#include <tuple>
#include "../utils/json.hpp"
#include "../utils/singleton.h"
#include "../include/clientType.h"

typedef enum emConfigEnabelType
{
    kConfigEnabelTypeTx,
    kConfigEnabelTypeGetBlock,
    kConfigEnabelTypeGetTxInfo,
    kConfigEnabelTypeGetAmount,
    kConfigEnabelTypeGetMoney,
}ConfigEnabelType;

//存储版本服务器节点信息的结构体
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
    //根据文件名构造配置文件
    Config(const std::string &name){if(!this->InitFile(name)){_Exit(0);}};

    //动态加载配置文件
    void Reload(const std::string &name = "config.json");
    //根据m_Json写入配置文件
    void WriteFile(const std::string &name = "config.json");
    //初始化配置文件
    bool InitFile(const std::string &name = "config.json");
    //创建配置文件
    bool NewFile(std::string strFile);


    //******************net层相关的配置*******************
    //获取是否为外网节点 is_public_node
    bool GetIsPublicNode();

    //获取服务器的状态
    bool GetCond(const std::string &cond_name);

    //获取net层定义的变量k_bucket、k_refresh_time、local_ip、work_thread_num
    int GetVarInt(const std::string &cond_name);
    std::string GetVarString(const std::string &cond_name);

    //获取、设置k桶节点ID
    std::string GetKID();
    bool SetKID(std::string strKID);

    //获取、设置节点的内网ip地址
    std::string GetLocalIP();
    bool SetLocalIP(std::string strIP);
    int GetLocalPort();
    int GetHttpPort();
    std::vector<std::tuple<std::string,int>> GetServerList();

    //******************ca层相关的配置***************************
    /**
     * @description: 获得是否启用
     * @param enable 返回是否启用标志 
     * @return: 操作是否成功，若返回false则enable不可用
     */
    bool GetEnable(ConfigEnabelType type, bool * enable);

    /**
     * @description: 设置是否启用
     * @param 是否启用
     * @return: 操作成功返回true，失败返回false
     * @mark: 仅修改内存值，不修改配置文件
     */
    bool SetEnable(ConfigEnabelType tpye, bool enable);

    /**
     * @description: 返回配置文件版本
     * @param 无 
     * @return: 返回配置文件版本
     */
    std::string GetVersion();



    /**
     * @description: 获取更新目标版本号
     * @param 无 
     * @return: 获取更新目标版本号
     */
    std::string GetTargetVersion();
    //获取四个版本服务器的信息
    std::vector<node_info> GetNodeInfo_s();
    //更新配置文件,参数默认值代表不更新该字段
    bool UpdateConfig(int flag = 2,const std::string &target_version = "not", const std::string &ip = "not", const std::string &latest_version = "");
    //获取之前请求更新的对应版本服务器ip
    std::string GetVersionServer();


    /**
     * @description: 根据clientType返回所支持客户端版本号
     * @param clientType
     * @return: 返回clientType所指示的客户端版本号
     * @mark 返回的版本号为此ebpc所支持的最小客户端版本
     */
    std::string GetClientVersion(ClientType clientType);

    /**
     * @description: 获得日志级别
     * @param 
     * @return: 返回日志级别，若查询失败则返回空字符串
     */
    std::string GetLogLevel();

    /**
     * @description: 获得日志文件名
     * @param 
     * @return: 返回日志文件名，若查询失败则返回空字符串
     */
    std::string GetLogFilename();

    
    /**
     * @description: 时间内默认同步块数
     * @param 
     * @return: 返回JSON读取块数，默认为10
     */
    unsigned int GetSyncDataCount();

    /**
     * @description: 获取同步时间间隔
     * @param 
     * @return:  返回同步时间间隔
     */
    unsigned int GetSyncDataPollTime();

    /**
     * @description: 获得服务器节点信息
     * @param 
     * @return: 节点信息的json字符串
     */
    std::string GetNodeInfo();

    /**
     * @description: 获得设备密钥
     * @param 
     * @return: 返回存储的密钥
     */
    std::string GetDevPassword();

    /**
     * @description:  设置设备密钥
     * @param password 待写入的密钥
     * @return: 设置成功返回true，否则返回false
     */
    bool SetDevPassword(const std::string & password);

    /**
     * @description:  设置设备密钥获得py脚本服务端ip
     * @param
     * @return: 返回py脚本服务端ip
     */
    std::string GetPyRtPirceIp();

    int ClearNode();
    int AddNewNode(const node_info& node);

    int ClearServer();
    int AddNewServer(const std::string& ip, unsigned short port);


private:
    std::string GetAbsopath();
    bool VerifyConf(nlohmann::json &target);

private:
	nlohmann::json m_Json;
    std::vector<nlohmann::json> m_servers;
};

#endif