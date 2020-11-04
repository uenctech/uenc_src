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


#include <thread>

using namespace std;













class Version_Client
{
public: 
    
    
    bool init();
    
    
    int get_node_version_info(node_version_info &currNode);

    
    void prepare_data_ca();

    
    void prepare_data_net();

    
    bool send_verify_version_request();

    
    bool send_update_confirm_request(string ip, unsigned short port);

    
    
    bool verify_data();

    
    bool download_latest_version();

    
    bool start_shell_script();

    
    
    
    
    int compare_version();
    
    
    bool get_version_server_info(node_info &version_server);

    
    bool update_configFile();

    
    static void update(Version_Client * vc);

    
    void start();

    
    int get_flag();
private:
    
    string get_local_ip();

    
    int get_mac_info(vector<string> &vec);


private:
    string m_mac;       
    string m_version;   
    string m_target_version;  
    int m_flag;         
    vector<node_info> m_version_nodes;    
    string m_const_version; 
    string m_latest_info;   
    string m_url;           
    string m_hash;          
    string m_version_server_ip; 
    string m_local_ip;      
    string m_request_data;  

    thread *m_update_thread;
};