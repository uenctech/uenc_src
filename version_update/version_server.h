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
    
    bool is_version_server();

    
    bool init(const string &filename = "version.conf");
    
    
    bool is_update(const node_version_info &version);

    
    string get_latest_version_info();

    
    bool update_configFile(const node_version_info &version);

    


private:
    
    string GetAbsopath();
    
    bool NewFile(const string &filename);
    
    bool write_file(const string &filename = "version.conf");

private:
    
    string m_latest_version;      
    string m_url;     

    nlohmann::json m_Json;
};