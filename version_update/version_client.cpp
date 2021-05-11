#include "version_client.h"
#include "../ca/ca.h"
#include "../ca/ca_message.h"
#include "../ca/ca_global.h"
#include "../ca/ca_serialize.h"
#include <random>
#include <chrono>
#include <sstream>
#include "HttpRequest.h"
#include "../common/config.h"
#include "../utils/util.h"

typedef struct net_pack
{
	int			len				=0;
	int			type			=0;
	int			data_len		=0;
	std::string	data			="";
	std::string	version			="";
}net_pack;


//创建更新线程，设置线程回调
void Version_Client::start()
{
    m_update_thread = new thread(Version_Client::update, this);
    m_update_thread->detach();
    return;
}


//启动版本更新程序,每12小时校验一次
void Version_Client::update(Version_Client * vc)
{   
    while (true)
    {   
        //初始化
        vc->init();
        //根据flag标志位执行不同的代码
        if (vc->get_flag() == 0)
        {   
            //尚未进行版本更新校验
            //准备要发送的数据
            vc->prepare_data_ca();
            vc->prepare_data_net();
            //发送版本更新请求，并接收是否更新版本的信息，并作出对应的处理
            vc->send_verify_version_request();
        }

        if (vc->get_flag() == 1)
        {
            //刚完成版本更新，刚刚启动新版本
            //比对当前版本和目标版本
            int res = vc->compare_version();
            if (res == 0)
            {
                cout << "版本更新成功！" << endl;
                //获取上次版本更新服务器的信息
                node_info ver_node;
                vc->get_version_server_info(ver_node);
                //准备要发送的数据
                vc->prepare_data_ca();
                vc->prepare_data_net();

                //给版本服务器发送版本更新完成的消息
                for (auto it = vc->m_version_nodes.begin(); it != vc->m_version_nodes.end(); ++it)
                {
                    if (it->enable == "1")
                    {
                        bool bl = vc->send_update_confirm_request(it->ip, it->port);
                        //收到服务器确认消息，更新配置文件
                        if (bl)
                        {
                            //收到确认，更新配置文件
                            cout << "收到确认，更新配置文件！" << endl;
                            
                        }
                        else
                        {
                            cout << "版本更新确认请求错误，等待下次更新！" << endl;
                        }       
                    }
                }
                //更新本地配置文件
                vc->update_configFile();
                
            }
            else if (res == 1)
            {
                cout << "============客户端版本更新失败===========" << endl;
                vc->update_configFile();
            }
            else if (res == -1)
            {
                cout << "============客户端版本错误===========" << endl;
                vc->update_configFile();
            }
            else
            {
                cout << "未知错误...." << endl;
                vc->update_configFile();
            }
        }   
        sleep(60);
    }

}

bool Version_Client::init()
{   
    Config *cfg = Singleton<Config>::get_instance();
    
    //获取版本更新标志
    m_flag = cfg->GetFlag();

    m_local_ip = get_local_ip();

    //获取mac地址
    vector<string> vec;
    get_mac_info(vec);
    if (vec.size() != 0)
    {
        m_mac = vec[0];
    }

    //获取版本服务器ip和port
    m_version_nodes = cfg->GetNodeInfo_s();

    if (m_flag == 0)
    {
        //需要进行版本校验
        //获取当前版本信息
        m_version = ca_version();
        return true;
    }
    else if(m_flag == 1)
    {
        //更新完成，获取当前版本和目标更新版本信息
        m_target_version = cfg->GetTargetVersion();
        m_version = m_const_version = ca_version();
        return true;
    }
    else
    {
        return false;
    }   
}

//获取本机ip
string Version_Client::get_local_ip()
{
    std::ifstream fconf;
    std::string conf;
    std::string tmp = NETCONFIG;

    if (access(tmp.c_str(), 0) < 0)
    { 
        error("Invalid file...  Please check if the configuration file exists!");
        return "";
    }
    fconf.open(tmp.c_str());
    nlohmann::json m_Json;
    fconf >> m_Json;
    fconf.close();

    return m_Json["var"]["local_ip"].get<std::string>();
}

int Version_Client::compare_version()
{
    //比对两个版本号，如果一致，说明更新成功
    cout << "对比目标版本：" << endl;
    cout << "m_const_version：" << m_const_version << endl;
    cout << "m_target_version：" << m_target_version << endl;
    int res = -2;
    if (m_const_version == m_target_version)
    {   

        res = 0;
    }
    
    if (m_const_version < m_target_version)
    {
        res = 1;
    }

    if (m_const_version > m_target_version)
    {   
        res = -1;
    }
    
    return res;
}



//获取本机mac地址
int Version_Client::get_mac_info(vector<string> &vec)
{
    int fd;
    int interfaceNum = 0;
    struct ifreq buf[16] = {0};
    struct ifconf ifc;
    char mac[16] = {0};

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket");

        close(fd);
        return -1;
    }
    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = (caddr_t)buf;
    if (!ioctl(fd, SIOCGIFCONF, (char *)&ifc))
    {
        interfaceNum = ifc.ifc_len / sizeof(struct ifreq);
        while (interfaceNum-- > 0)
        {
            //printf("ndevice name: %s\n", buf[interfaceNum].ifr_name);
            if(string(buf[interfaceNum].ifr_name) == "lo")
            {
                continue;
            }
            if (!ioctl(fd, SIOCGIFHWADDR, (char *)(&buf[interfaceNum])))
            {
                memset(mac, 0, sizeof(mac));
                snprintf(mac, sizeof(mac), "%02x%02x%02x%02x%02x%02x",
                    (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[0],
                    (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[1],
                    (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[2],

                    (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[3],
                    (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[4],
                    (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[5]);
                // allmac[i++] = mac;
                std::string s = mac;
                vec.push_back(s);
            }
            else
            {
                printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
                close(fd);
                return -1;
            }
        }
    }
    else
    {
        printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

//获取当前节点版本信息
int Version_Client::get_node_version_info(node_version_info &currNode)
{
    currNode.mac = m_mac;
    currNode.version = m_version;
    currNode.local_ip = m_local_ip;

    return 0;
}

//准备要发送的请求数据 ca包装
void Version_Client::prepare_data_ca()
{   
    cstring *s = cstr_new_sz(0);
    cstring *smac = cstr_new(m_mac.c_str());
    cstring *sversion = cstr_new(m_version.c_str());
    cstring *sip = cstr_new(m_local_ip.c_str());
    
    // cout << "mac :" << m_mac << endl;
	// cout << "version:" << m_version << endl;
	// cout << "local_ip:" << m_local_ip << endl;

    ser_varstr(s, smac);
    ser_varstr(s, sversion);
    ser_varstr(s, sip);
    // cout << "ca层包装前的数据(m_request_data)：" << m_request_data << " ,len= "<< m_request_data.size() << endl;
    cstring * send = message_str((const unsigned char*)"\xf9\xbe\xb4\xd9", UPDATEREQUEST, s->str, s->len);
    
    m_request_data = string(send->str, send->len);
   
    // cout << "ca层包装后的数据：" << send->str << " ,len= "<< send->len << endl;
 
    // cout << "ca层包装后的数据(m_request_data)：" << m_request_data << " ,len= "<< m_request_data.size() << endl;
    cstr_free(s, true);
    cstr_free(send, true);
}
//net层包装
static atomic<unsigned int> serial;
void Version_Client::prepare_data_net()
{    
    string data = m_request_data;
    // cout << "将要发送的ca层数据：" << data << " ,len= "<<data.size() << endl;
    net_pack pack;

	pack.len = (int)data.size();
	pack.data = data;
	// pack.type = CA_UPDATE;
	pack.len = sizeof(int)*2 + pack.data.size() + 32;
    char version[32] = {0};
	srand( (unsigned)time(NULL) + serial++ );
	for (int i = 0; i < 32 - 1; i++)
		version[i] = (rand() % 9) + 48;
	version[32 - 1] = '\0';
    pack.version = string(version, 32);

	//m_request_data = Pack::packag_to_string(pack);
    int pack_len = pack.len + sizeof(int);
	char* buff = new char[pack_len];
	ExitCaller ec([=] { delete[] buff; });
	//Pack::packag_to_buff(pack, buff, pack_len);
    
	memcpy(buff, &pack.len, 4);
	memcpy(buff + 4, &pack.type, 4);
	memcpy(buff + 4 + 4, &pack.len, 4);
	memcpy(buff + 4 + 4 + 4, pack.data.data(), pack.len);
	memcpy(buff + 4 + 4 + 4 + pack.len, pack.version.data(), 32);
	m_request_data = string(buff, pack_len);
}

//循环遍历服务器版本节点，发送版本校验请求
bool Version_Client::send_verify_version_request()
{   
    //cout << "准备连接并发送数据：" << endl;
    
    for (auto it = m_version_nodes.begin(); it != m_version_nodes.end(); ++it)
    {
        if (it->enable == "1")
        {   
            //发送信息
            TcpSocket socket;
            cout << "准备连接版本服务器 ---->  ip: " << it->ip << ", port: " << it->port << endl;
            int ret = socket.connectToHost(it->ip, it->port);
            if (ret != 0)
            {
                cout << "连接版本服务器失败" << endl;
                socket.disConnect();
                //sleep(3);
                continue;
            }
            else
            {   
                cout << "连接版本服务器成功" << endl;
                cout << "开始发送数据：=====》" << endl;
                //cout << m_request_data << " ,len= "<< m_request_data.size() << endl;
                ret = socket.sendMsg(m_request_data);
                if (ret != 0)
                {
                    cout << "发送数据失败" << endl;
                    socket.disConnect();
                    //sleep(3);
                    continue;
                }
                else
                {
                    cout << "发送数据成功" << endl;
                    //接收数据
                    cout << "开始接收数据" << endl;
                    string res = socket.recvMsg();
                    
                    if (res == "")
                    {
                        cout << "接收数据超时或者异常！" << endl;
                        socket.disConnect();
                        sleep(1);
                        continue;
                    }
                    else if (res == "Latest")
                    {
                        cout << "当前版本已经是最新版本！" << res << endl;
                        socket.disConnect();
                        return true;
                    }
                    else 
                    {   
                        cout << "接收到的返回数据：" <<res << endl;
                        
                        stringstream ss(res);
                        ss >> m_target_version;
                        ss >> m_url;
                        ss >> m_hash;

                        cout << "目标版本：" << m_target_version << endl;
                        cout << "下载地址：" << m_url << endl;
                        cout << "验证hash值：" << m_hash << endl;

                        if (verify_data())
                        {
                            cout << "数据验证成功！" << endl;
                            cout << "开始下载最新版本==》" << endl;
                            if (download_latest_version())
                            {
                                cout << "下载成功!" << endl;
                                
                                //记录下选择的版本服务器ip
                                m_version_server_ip = it->ip;

                                //启动shell脚本开始自动更新
                                socket.disConnect();
                                start_shell_script();
                                
                                return true;
                            }
                        }
                        else
                        {
                            cout << "验证数据失败！" << endl;
                            socket.disConnect();
                            return false;
                        }
                    }
                    
                }
            
            }
        } 
        // socket.disConnect();
        // break;   
    }

    return false;
}

bool Version_Client::send_update_confirm_request(string ip, unsigned short port)
{
    // cout << "准备连接并发送确认更新完成数据：" << m_request_data << endl;
    
    //发送信息
    TcpSocket socket;
    int ret = socket.connectToHost(ip, port);
    if (ret != 0)
    {
        cout << "连接版本服务器失败" << endl;
        socket.disConnect();
        return false;
    }
    // cout << "开始发送数据：=====》" << m_request_data << endl;
    // cout << m_request_data << " ,len= "<< m_request_data.size() << endl;
    ret = socket.sendMsg(m_request_data);
    if (ret != 0)
    {
        cout << "发送数据失败" << endl;
        socket.disConnect();
        return false;
    }
    cout << "发送数据成功" << endl;

    //接收数据
    string res = socket.recvMsg();
    socket.disConnect();
    if (res == "")
    {
        cout << "接收数据超时或者异常！" << endl;
        return false;
    }
    else if (res == "Latest")
    {
        cout << "当前版本已经是最新版本！" << res << endl;
        return true;
    }
    else 
    {   
        cout << "版本更新失败，等待下次更新" << endl;
        return false;    
    }    

    return false;
}


//验证数据,防止数据丢失
bool Version_Client::verify_data()
{
    string tmp = m_target_version + m_url;
    string hash = getsha256hash(tmp);
    if (m_hash == hash)
    {   
        debug("数据验证成功，数据完整！");
        return true;
    }
    else
    {
        debug("数据验证失败，数据有误！");
        return false;
    }  
}

//收到响应后，下载最新版本
bool Version_Client::download_latest_version()
{
    debug("开始下载最新版本...");
    cout << "正在下载最新版本..." << endl;

    HttpRequest ImageReq(m_url);
    ImageReq.setRequestMethod("GET");
    ImageReq.setRequestProperty("Cache-Control", "no-cache");
    ImageReq.setRequestProperty("Content-Type", "application/octet-stream");
    ImageReq.setRequestProperty("Connection", "close\r\n");

    ImageReq.connect();
    ImageReq.send();
    ImageReq.handleRead();
    ImageReq.downloadFile("./ebpc.tar.gz");

    debug("最新版本下载完成...");
    cout << "最新版本下载完成..." << endl;
    return true;
}

//下载完毕，将配置文件标志位置为1，代表正在更新中，同时记录下目标版本的版本号，然后启动更新脚本
bool Version_Client::start_shell_script()
{
    debug("准备安装程序，启动更新脚本...");
    if(Singleton<Config>::get_instance()->UpdateConfig(1, m_target_version, m_version_server_ip))
    {
        cout << "配置文件更新完毕，下一步解压文件！" << endl;
        system("tar zxvf ebpc.tar.gz");
        //cout << "启动更新脚本" << endl;
        system("chmod 755 update.sh");
        system("./update.sh");
        
        exit(0);
    }
    return false; 
}

//获取版本服务器信息，用于发送版本更新完成的消息
bool Version_Client::get_version_server_info(node_info &version_server)
{   
    //获取版本服务器ip
    m_version_server_ip = Singleton<Config>::get_instance()->GetVersionServer();
    for (auto it = m_version_nodes.begin(); it != m_version_nodes.end(); ++it)
    {
        if (it->ip == m_version_server_ip && it->enable == "1")
        {
            version_server = *it;
            return true;
        }
    }
    
    return false;
}

//收到服务器确认更新完毕的消息，更新配置文件
bool Version_Client::update_configFile()
{
    if(Singleton<Config>::get_instance()->UpdateConfig(0, "", "", m_const_version))
    {
        return true;
    }
    
    return false;      
}

//获取m_flag值
int Version_Client::get_flag()
{
    return m_flag;
}
