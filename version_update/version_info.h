#pragma once
#include <iostream>
#include <string>

using namespace std;

#define NETCONFIG "./config.json"


struct node_version_info
{
    string mac;
    string version;
    string local_ip;
};


struct latest_version_info
{
    string version;
    string url;
    string hash;
};