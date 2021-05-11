#pragma once
#include <iostream>
#include <string>
#include "../utils/singleton.h"
#include "../ca/Crypto_ECDSA.h"
#include <unistd.h>
#include "../include/logging.h"
#include "../utils/json.hpp"
#include "version_info.h"

using namespace std;


class Version_Server
{
public:
    //判断是否是版本服务器
    bool is_version_server();

    //配置文件初始化，存在读取信息，不存在创建文件
    bool init(const string &filename = "version.conf");
    
    //校验版本信息,是否需要更新版本
    bool is_update(const node_version_info &version);

    //如果节点版本不是最新，发送最新版本号和url资源地址,以及hash值(用来验证数据的完整性)
    string get_latest_version_info();

    //节点版本更新完毕，服务器更新配置文件
    bool update_configFile(const node_version_info &version);

    //获取要发送给客户端的确认更新信息，通知客户端节点更新本地配置文件


private:
    //获取当前程序绝对路径
    string GetAbsopath();
    //如果文件不存在，创建文件
    bool NewFile(const string &filename);
    //将m_Json写入配置文件
    bool write_file(const string &filename = "version.conf");

private:
    //map<string, string> m_version_of_nodes;   //各节点的版本信息，key为mac地址，值为版本号
    string m_latest_version;      //最新版本
    string m_url;     //最新版本的资源路径

    nlohmann::json m_Json;
};