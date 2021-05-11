#pragma once
#include <iostream>
#include <string>

using namespace std;

#define NETCONFIG "./config.json"

//当前节点版本信息
struct node_version_info
{
    string mac;
    string version;
    string local_ip;
};

//收到服务器发来的最新版本信息
struct latest_version_info
{
    string version;
    string url;
    string hash;
};