#pragma once
#include <iostream>
#include <string>
#include <vector>
#include "../utils/json.hpp"
#include "../common/config.h"
#include "../include/logging.h"
#include <stdlib.h>
#include <cstring>
#include <net/if.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "../ca/Crypto_ECDSA.h"
#include "TcpSocket.h"
#include "version_info.h"
// #include <signal.h>
// #include <sys/time.h>
#include <thread>

using namespace std;
// #define TIMER 50

// //启动版本更新程序，每12小时校验一次
// //信号捕捉回调函数
// void start(int signal);

// //创建版本更新线程函数
// void create_thread();
// void *thr_fn(void *arg);

// //定时启动更新
// void time_to_start_update();

class Version_Client
{
public: 
    //初始化成员变量,根据标志位进行初始化
    //检查配置文件标志位，如果为1，说明刚更新完版本,如果为0，说明需要进行版本校验
    bool init();
    
    //获取当前节点版本信息
    int get_node_version_info(node_version_info &currNode);

    //准备要发送的请求数据 ca包装
    void prepare_data_ca();

    //准备要发送的请求数据 net包装
    void prepare_data_net();

    //循环遍历服务器版本节点，发送版本更新请求
    bool send_verify_version_request();

    //发送版本更新完成确认请求
    bool send_update_confirm_request(string ip, unsigned short port);

    //验证数据
    //bool verify_data(const latest_version_info & latest_info);
    bool verify_data();

    //收到响应后，下载最新版本
    bool download_latest_version();

    //下载完毕，将配置文件标志位置为1，代表正在更新中，同时记录下目标版本的版本号，然后启动更新脚本
    bool start_shell_script();

    //比对当前版本
    //返回值为0，代表相等，更新成功
    //返回值为1，代表服务端版本比客户端版本新，更新失败
    //返回值为-1，代表客户端版本比服务器版本新，客户端版本错误
    int compare_version();
    
    //获取版本服务器信息，用于发送版本更新完成的消息
    bool get_version_server_info(node_info &version_server);

    //收到服务器确认更新完毕的消息，更新配置文件
    bool update_configFile();

    //启动版本更新程序，每12小时校验一次
    static void update(Version_Client * vc);

    //创建更新线程，设置线程回调
    void start();

    //获取m_flag值
    int get_flag();
private:
    //获取本机ip
    string get_local_ip();

    //获取本机mac地址
    int get_mac_info(vector<string> &vec);


private:
    string m_mac;       //mac地址
    string m_version;   //当前版本号
    string m_target_version;  //更新的目标目标版本号
    int m_flag;         //是否正在更新
    vector<node_info> m_version_nodes;    //节点的ip和端口信息
    string m_const_version; //程序中常量版本号，用来做更新比对
    string m_latest_info;   //记录最新版本信息
    string m_url;           //记录下载地址
    string m_hash;          //接收到的hash值
    string m_version_server_ip; //上一次版本更新服务器的ip地址
    string m_local_ip;      //本机ip
    string m_request_data;  //要发送的请求数据

    thread *m_update_thread;
};