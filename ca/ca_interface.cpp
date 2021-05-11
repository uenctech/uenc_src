#include <stdio.h>
#include <iostream>
#include <assert.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <vector>
#include<unistd.h>  
#include<fcntl.h>
#include <netdb.h>
#include <map>
#include "ca_interface.h"
#include "ca_transaction.h"
#include "ca_message.h"
#include "ca_hexcode.h"
#include "ca_buffer.h"
#include "ca_serialize.h"
#include "ca_util.h"
#include "ca_global.h"
#include "ca_coredefs.h"
#include "ca_hexcode.h"
#include "ca_console.h"
#include "Crypto_ECDSA.h"
#include "../include/cJSON.h"
#include "ca_test.h"
#include "ca_device.h"
#include "../include/net_interface.h"
#include "../include/logging.h"
#include "ca_base64.h"
#include "../include/ScopeGuard.h"
#include "ca_pwdattackchecker.h"
#include "ca_txvincache.h"
#include <thread>
#include "ca_rocksdb.h"
#include<sstream>
#include "../common/config.h"
#include <random>
#include "common.pb.h"
#include "getmac.pb.h"
#include "MagicSingleton.h"
#include "ca_synchronization.h"
#include "ca.h"
#include "ca_txhelper.h"
#include "../utils/time_util.h"
#include "../utils/util.h"

int senddata(int fdnum);

int recvdata(int fdnum,std::vector<std::string> * ip_vect,std::vector<map<std::string,std::string>> *outip_mac,int i);

int interface_GetMnemonic(const char *bs58address, char *out, int outLen)
{
    return g_AccountInfo.GetMnemonic(bs58address, out, outLen);
}

bool interface_GenerateKey()
{
    return g_AccountInfo.GenerateKey();
}

bool interface_GetPrivateKeyHex(const char *bs58address, char *pridata, unsigned int iLen)
{
     return g_AccountInfo.GetPrivateKeyHex(bs58address, pridata, iLen);
}
bool interface_ImportPrivateKeyHex(const char *pridata)
{
    return g_AccountInfo.ImportPrivateKeyHex(pridata);
}

int  interface_GetKeyStore(const char *bs58address, const char *pass, char *buf, unsigned int bufLen)
{
    return g_AccountInfo.GetKeyStore(bs58address, pass, buf, bufLen);
}

bool interface_GetBase58Address(char *bs58address, unsigned int iLen)
{
    if(g_AccountInfo.CurrentUser.Bs58Addr.size()<=0)
        return false;
    if(bs58address == NULL || iLen < g_AccountInfo.CurrentUser.Bs58Addr.size())
        return false;
    memcpy(bs58address, g_AccountInfo.CurrentUser.Bs58Addr.c_str(), g_AccountInfo.CurrentUser.Bs58Addr.size());
    return true;
}

bool interface_ShowMeTheMoney(const char * bs58Address, double amount, char *outdata, size_t *outdataLen)
{
    if (bs58Address == NULL || strlen(bs58Address) == 0 || 
        outdata == NULL || outdataLen == NULL)
    {
        error("param error");
        return false;
    }

    char pamt[128] = {0};
    double amt = amount;
    sprintf(pamt, "%.0lf", amt);

    cstring *send = cstr_new_sz(0);
    cstring *pto = cstr_new((char *)bs58Address);
    cstring *pmoney = cstr_new((char *)pamt);

    ser_varstr(send, pto);
    ser_varstr(send, pmoney);

    cstring *data = message_str(g_chain_metadata.netmagic, GETMONEY, send->str, send->len);
    if (data->len > *outdataLen)
    {
        *outdataLen = data->len;
        
        cstr_free(pmoney, true);
        cstr_free(pto, true);
        cstr_free(send, true);
        cstr_free(data, true);

        return false;
    }

    memcpy(outdata, data->str, data->len);
    *outdataLen = data->len;
    
    cstr_free(pmoney, true);
    cstr_free(pto, true);
    cstr_free(send, true);
    cstr_free(data, true);
    return true;
}

void interface_Init(const char *path)
{
    InitAccount(&g_AccountInfo, path);
}

int CreateTx(const char* From, const char * To, const char * amt, const char *ip, uint32_t needVerifyPreHashCount, std::string minerFees)
{
    info("CreateTx ===== 1");
    if (From == nullptr || To == nullptr || amt == nullptr)
    {
        printf("param error\n");
        return -1;
    }

    if (! strcmp(From, To))
    {
        printf("The From addr is can not be same as the To Addr\n");
        return -2;
    }

    double dAmount = stod(amt);
    double dminerFee = stod(minerFees);

    uint64_t amount = (dAmount + FIX_DOUBLE_MIN_PRECISION) * DECIMAL_NUM;
    uint64_t minerFeesConvert =  (dminerFee + FIX_DOUBLE_MIN_PRECISION )* DECIMAL_NUM ;
    
    if(From != NULL)
    {
        if (!g_AccountInfo.SetKeyByBs58Addr(g_privateKey, g_publicKey, From)) {
            std::cout << "非法账号" << std::endl;
            return -2;
        }
    }
    else
    {
        g_AccountInfo.SetKeyByBs58Addr(g_privateKey, g_publicKey, NULL);
    }

    std::vector<std::string> fromAddr;
    fromAddr.emplace_back(From);

    std::map<std::string, int64_t> toAddrAmount;
    std::string to_addr(To);
    toAddrAmount[to_addr] = amount;

    TxHelper::DoCreateTx(fromAddr,toAddrAmount, needVerifyPreHashCount, minerFeesConvert);
    return 0;
}

int free_buf(char **buf)
{
    if(*buf)
        free(*buf);

    *buf = NULL;

    return 0;
}

bool net_segment_allip(const char *ip, const char *mask, std::vector<std::string> & ip_vect)
{
    struct in_addr net_ip, net_mask, tmp;
    unsigned int max, min;

    inet_aton(ip, &net_ip);
    inet_aton(mask, &net_mask);

    tmp.s_addr = net_ip.s_addr & net_mask.s_addr;
    min = ntohl(tmp.s_addr);

    tmp.s_addr = net_ip.s_addr | ~net_mask.s_addr;
    max = ntohl(tmp.s_addr);

    unsigned int temp_min;
    if(max < min)
    {
        std::cout << "network segment start and end failed !" << std::endl;
        return false;
    }

	unsigned int b = ntohl(net_ip.s_addr);
    for(temp_min = min + 1; temp_min != max; temp_min++)
    {
        // 不记录自己的地址
        if(temp_min == b)
        {
            continue;
        }

        unsigned int temp = htonl(temp_min);
        struct in_addr a;
        a.s_addr = temp;
        ip_vect.push_back(inet_ntoa(a));
    }
    return true;
}

bool getuserip_all(std::vector<std::string> * ip_vect, unsigned int scanport, std::vector<map<std::string,std::string>> *outip_mac, unsigned int * current, unsigned int && total, ScanPortProc spp)
{
    if(!ip_vect ||!outip_mac)
    {
        return false;
    }
    for(unsigned long int i = 0; i < ip_vect->size(); i++)
    {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if(fd < 0)
        {
            perror("socket create failed !");
            continue;
        }

        struct sockaddr_in serveraddr;
        serveraddr.sin_family = AF_INET;
        serveraddr.sin_port = htons(scanport);
        serveraddr.sin_addr.s_addr = inet_addr(ip_vect->at(i).c_str());

        int flags = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &flags, sizeof(int));	
	    flags = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &flags, sizeof(int));
    

         int flag = fcntl(fd, F_GETFL,0);
		 fcntl(fd, F_SETFL, flag | O_NONBLOCK);
		

		struct timeval tm;
		tm.tv_sec = 0;
		tm.tv_usec = 300000;
        
        struct servent * sp;
    	fd_set rset;
        fd_set wset; 
      
		//connect为非阻塞，连接不成功立即返回-1
		if (!connect(fd,(struct sockaddr*)&serveraddr,sizeof(struct sockaddr)))
		{
			sp=getservbyport(htons(scanport),"tcp");
            senddata(fd); 
            if(0 != recvdata(fd, ip_vect,outip_mac,i)) 
            {
                close(fd);
                (*current)++;
                continue;
            }
        
 			printf("%s tcp port %d open:%s\n ",ip_vect->at(i).c_str(), scanport,sp->s_name);

        }
		else 
		{
            // perror("connect error:");
            //假如连接不成功，则运行select,直到超时
			FD_ZERO(&rset);
			FD_ZERO(&wset);
			FD_SET(fd, &rset);
			FD_SET(fd, &wset);
			int error; //错误代码
			socklen_t len = sizeof(error); 
			//5秒后查看socket的状态变化 
			if ( select(fd+1, &rset, &wset, NULL, &tm) > 0 )
            {
				getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len);
                cout<<"errornumber=" <<error <<endl;
				if(error == 0)
                {
                    senddata(fd); 
                    if( 0 != recvdata(fd, ip_vect, outip_mac, i) ) 
                    {
                        senddata(fd); 
                        close(fd);
                        (*current)++;
                        continue; 
                    }  
                }
			}
        }

        (*current)++;
        if (spp)
        {
            spp(ip_vect->at(i).c_str(), *current, total);
        }
        close(fd);
    }
    return true;
}


unsigned int sum = 0;

bool interface_ScanPort(const char *ip, const char *mask, unsigned int port, char *outdata, size_t *outdatalen, ScanPortProc spp,int i,map<int64_t,string> &i_str)
{
    if(ip == NULL || mask == NULL || outdata == NULL || outdatalen == NULL)
    {
        error("param error");
        return false;
    }

    if(port == 0)
    {
        port = 11187;
    }

    std::vector<std::string> ip_vect;
    std::vector<map<std::string,std::string>> outip_mac;
    net_segment_allip(ip, mask, ip_vect);

    int n = ip_vect.size() / 10;
    if(n == 0)
    {
        sum = 0;
        getuserip_all(&ip_vect, port, &outip_mac, &sum, ip_vect.size(), spp);
    }
    else
    {     
        if((ip_vect.size() % 10) != 0)
            n++;

        int tmp = 0;    
        int len = n;
        std::vector<std::vector<std::string>> vv;
        for (int i = 0; i < 10; i++)
        {
            int j = 0;
            if((ip_vect.size() - tmp) < (uint64_t)n)
                len = ip_vect.size() - tmp - 1;
            std::vector<std::string> v;
            for (j = 0; j < len; j++)
            {
                tmp = i * n + j;
                v.push_back(ip_vect[tmp]);
            }
            vv.push_back(v);
        }

        sum = 0;
        std::thread threads[10]; 
        for (int i = 0; i < 10; ++i)
        {  
            threads[i] = std::thread(getuserip_all, &vv[i], port, &outip_mac, &sum, ip_vect.size(), spp);   // move-assign threads 这里调用move复制函数  
        }

        std::cout << "Done spawning threads. Now waiting for them to join:\n";  
        for (int i = 0; i<10; ++i)
        {
            threads[i].join();  
        }  
            
        std::cout << "All threads joined!\n";  
    
    }

    cJSON * root =  cJSON_CreateObject();

	cJSON_AddItemToObject(root, "code", cJSON_CreateNumber(0));
	cJSON_AddItemToObject(root, "message", cJSON_CreateString("msg"));

    cJSON * data =  cJSON_CreateArray();

    for(size_t i =0;i< outip_mac.size();i++)
    {
        cJSON * ipaddr = cJSON_CreateObject();
        map<string,string>::iterator iter;
        iter = outip_mac[i].begin();
        string mac = iter->second;
        cout<< "macaddress=" << mac.c_str() <<endl;
        char name1[256] = {0};
        sprintf(name1, "%s", mac.c_str());
        cJSON_AddItemToObject(ipaddr, "titile", cJSON_CreateString(name1));
        cJSON_AddItemToObject( ipaddr, "IP", cJSON_CreateString( iter->first.c_str() ) );
        cJSON_AddItemToObject(ipaddr, "port", cJSON_CreateNumber(port));
        cJSON_AddItemToArray(data, ipaddr);
    }
    
	cJSON_AddItemToObject(root, "data", data);

    cstring * cs = cstr_new(cJSON_PrintUnformatted(root));
    cout << "cs:" << cs->str <<endl;
    cJSON_Delete(root);

    string str = cs->str;
    i_str.insert(pair<int64_t,string>(i,str));
    cout<<"str.size()="<<str.size()<<endl;
    size_t  length = cs->len;
    cout<<"length ="<<length <<endl;
    cout<<"i="<<i<<endl;
    cout<<"*outdatalen="<<*outdatalen<<endl;

    if(cs->len > *outdatalen)
    {
        *outdatalen = cs->len;
        cstr_free(cs, true);
        return false;
    }
    
    memcpy(outdata, cs->str, cs->len);
    *outdatalen = cs->len;
   
    cstr_free(cs, true);

    return true;
}

int senddata(int fdnum)
{
    GetMacReq getMacReq;
	CommonMsg msg;
	msg.set_type("GetMacReq");
	msg.set_version(getVersion());
	msg.set_data(getMacReq.SerializeAsString());

	std::string data = msg.SerializeAsString();
	int len = data.size() + 3 * sizeof(int);
    
	int checksum = Util::adler32((unsigned char * )data.c_str(), data.size());
    int flag = 0;
    int end_flag = 7777777;

	char* buff = new char[len + sizeof(int)];
	memcpy(buff, &len, 4);
	memcpy(buff + 4, data.data(), data.size());
	memcpy(buff + 4 + data.size(), &checksum, 4);
    memcpy(buff + 4 + data.size() + 4, &flag, 4);
	memcpy(buff + 4 + data.size() + 4 + 4, &end_flag, 4);
    int s = send(fdnum, buff, len + 4, 0);     //打包发送的数据
    cout<<"datas ="<<s<<endl;
    delete buff;
    return s;
} 


void ScanPortProcFun(const char * ip, unsigned int current, unsigned int total)
{
    cout << "================ " << (double)current / (double)total * 100 << "% ================" << endl;
    cout<<"================"<<ip<<"========================="<<endl;
}

ScanPortProc pScanPortProcFun = ScanPortProcFun;

void interface_testdevice_mac()
{
    map<int64_t,string> addrvalue;
    for(int i =0; i <5; i++)
    {
        char out_data[10240] = {0};
        size_t data_len = 10240;
        if(interface_ScanPort("192.168.1.60", "255.255.255.0",11185,out_data,&data_len, pScanPortProcFun,i,addrvalue))   
        {
            cout<<"scanport success"<<endl;
            sleep(1);
        } 
        else
        {
            cout<<"scanport failure"<<endl;
        }
    }
    map<int64_t,string>::iterator  it;  
    int fd = -1;
    cout<<"addrvalue.size()="<<addrvalue.size() <<endl;
    if (addrvalue.size() > 1) 
    {
        fd = open("i_str.txt", O_CREAT | O_WRONLY);
    }
    CaTestFormatType formatType;
    if (fd == -1) 
    {
        formatType = kCaTestFormatType_Print;
    } 
    else 
    {
        formatType = kCaTestFormatType_File;
    }
    for(it = addrvalue.begin(); it != addrvalue.end(); it++)  
    {  
        blkprint(formatType, fd, "%s,%u\n", it->first,it->second.c_str()); 
    } 
    close(fd); 
}


int  recvdata(int fdnum,std::vector<std::string> * ip_vect,std::vector<map<std::string,std::string>> *outip_mac,int i)
{
    char mybuf[1024] ={0};
    usleep(30000);      
    recv(fdnum,&mybuf,sizeof(mybuf),MSG_WAITALL); 
    
    int datalen; 
    memcpy(&datalen,mybuf,4);
    std::cout << "datalen:" << datalen << std::endl;
    if (datalen >0)
    {
        char splitdata[datalen - 3 * sizeof(int)] = {0};
        memcpy(splitdata, mybuf + 4,datalen - 3 * sizeof(int));     //内存recv接收过来的数据mac数据

        std::string read_data(splitdata, datalen - 3 * sizeof(int));

        CommonMsg common_msg;
        int r = common_msg.ParseFromString(read_data);
        if (!r) 
        {
            error("parse CommonMsg error");
            return 0;
        }

        std::string type = common_msg.type();
        std::cout << "recvdata type:" << type << std::endl;

        
        GetMacAck getMacAck;
        r = getMacAck.ParseFromString(common_msg.data());
        if (!r) 
        {
            error("parse getMacAck error");
            return 0;
        }
        cout<<"mac=:"<<getMacAck.mac()<<endl;
        map<string,string> mac_and_ip;
        mac_and_ip.insert(make_pair(ip_vect->at(i).c_str(),getMacAck.mac()));
        outip_mac->push_back(mac_and_ip);  
        return 0;
    }
    else
    {
        return -1;
    }       
}