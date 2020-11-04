#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <regex>
#include "./ip_port.h"
#include "../utils/singleton.h"
#include "./peer_node.h"
#include "../../include/logging.h"
#include "../../common/config.h"
#include <netdb.h> 
#include <stdlib.h>
#include <unistd.h>
#include "../utils/util.h"




char g_localhost_ip[16];
char g_public_net_ip[16];
char g_publicIP[30];             
int g_my_port;


bool IpPort::get_localhost_ip(char* g_localhost_ip)
{
	assert(g_localhost_ip != NULL);

	struct ifaddrs* if_addr_ptr = NULL;
	void* tmp_addr_ptr = NULL;

	getifaddrs(&if_addr_ptr);

	while (if_addr_ptr != NULL)
	{
		if (if_addr_ptr->ifa_addr->sa_family == AF_INET)
		{
			tmp_addr_ptr = &((struct sockaddr_in*)if_addr_ptr->ifa_addr)->sin_addr;
			inet_ntop(AF_INET, tmp_addr_ptr, g_localhost_ip, 32);

			if ((strlen(g_localhost_ip)) > 9)
				return true;
		}

		if_addr_ptr = if_addr_ptr->ifa_next;
	}

	return false;
}




bool IpPort::get_localhost_ip()
{
	struct ifaddrs* if_addr_ptr = NULL;
	void* tmp_addr_ptr = NULL;
	
	char* locla_ip = new char[IP_LEN];
	
	bzero(locla_ip, IP_LEN * sizeof(char));

	ExitCaller ec([=] { delete[] locla_ip; });

	getifaddrs(&if_addr_ptr);

	while (if_addr_ptr != NULL)
	{
		if (if_addr_ptr->ifa_addr->sa_family == AF_INET)
		{
			tmp_addr_ptr = &((struct sockaddr_in*)if_addr_ptr->ifa_addr)->sin_addr;
			
			
			inet_ntop(AF_INET, tmp_addr_ptr, locla_ip, IP_LEN);
			info("获取到本地IP");
			info("===========%s==============", locla_ip);
			if ((strlen(locla_ip)) > 9)
			{
				u32 local_ip = IpPort::ipnum(locla_ip);
				if (IpPort::is_local_ip(local_ip) && string(locla_ip) != string("255.255.255.255"))
				{	
					Singleton<PeerNode>::get_instance()->set_self_ip_l(local_ip);
					Singleton<PeerNode>::get_instance()->set_self_port_l(SERVERMAINPORT);
		
					info("属于本地IP");
					info("===========%s==============", locla_ip);
					Singleton<Config>::get_instance()->SetLocalIP(string(locla_ip));
					return true;
				}
				else
				{
					return false;
				}
			}
		}

		if_addr_ptr = if_addr_ptr->ifa_next;
	}

	
	freeifaddrs(if_addr_ptr);

	return false;
}




int IpPort::check_localhost_ip(const char* usr_ip)
{
	info("local ip:%s",g_localhost_ip);
	info("usr ip:%s" ,usr_ip);
	if ((strcmp(g_localhost_ip, usr_ip)) == 0)
	{
		info("The usr ip  %s is my localhost ip , do not need connected...", usr_ip);
		return 1;
	}
	return 0;
}



u32 IpPort::ipnum(const char* sz_ip)
{
	u32 num_ip = inet_addr(sz_ip);
	return num_ip;
}
u32 IpPort::ipnum(const string& sz_ip)
{
	u32 num_ip = inet_addr(sz_ip.c_str());
	return num_ip;
}


const char* IpPort::ipsz(const u32 num_ip)
{
	struct in_addr addr = {0};
	memcpy(&addr, &num_ip, sizeof(u32));
	const char* sz_ip = inet_ntoa(addr);
	return sz_ip;
}


bool IpPort::is_valid_ip(std::string const& str_ip)
{
	try
	{
		string pattern = "^(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|[1-9])\\.(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)\\.(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)\\.(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)$";
		std::regex re(pattern);
		bool rst = std::regex_search(str_ip, re);
		return rst;
	}
	catch (std::regex_error const& e)
	{
		debug("is_valid_ip e(%s)", e.what());
	}
	return false;
}


bool IpPort::is_valid_ip(u32 u32_ip)
{
	string str_ip = ipsz(u32_ip);
	return is_valid_ip(str_ip);
}


bool IpPort::is_local_ip(std::string const& str_ip)
{
	if (false == is_valid_ip(str_ip))
		return false;

	try
	{
		std::string pattern{ "^(127\\.0\\.0\\.1)|(localhost)|(10\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})|(172\\.((1[6-9])|(2\\d)|(3[01]))\\.\\d{1,3}\\.\\d{1,3})|(192\\.168\\.\\d{1,3}\\.\\d{1,3})$" };
		std::regex re(pattern);
		bool rst = std::regex_search(str_ip, re);
		return rst;
	}
	catch (std::regex_error const& e)
	{
		debug("is_valid_ip e(%d)", e.code());
	}
	return false;
}


bool IpPort::is_local_ip(u32 u32_ip)
{
	string str_ip = ipsz(u32_ip);
	return is_local_ip(str_ip);
}


bool IpPort::is_public_ip(std::string const& str_ip)
{
	if (false == is_valid_ip(str_ip))
		return false;

	return false == is_local_ip(str_ip);
}


bool IpPort::is_public_ip(u32 u32_ip)
{
	string str_ip = ipsz(u32_ip);
	return is_public_ip(str_ip);
}


bool IpPort::is_valid_port(u16 u16_port)
{
	return u16_port > 0 && u16_port <= 65535;
}


char* IpPort::get_peer_ip(int sockconn)
{
	struct sockaddr_in sa { 0 };
	socklen_t len = sizeof(sa);
	if (!getpeername(sockconn, (struct sockaddr*) & sa, &len))
		return inet_ntoa(sa.sin_addr);
	else
		debug("get_peer_ip getpeername fail.");
	
	return NULL;
}


u32 IpPort::get_peer_nip(int sockconn)
{
	struct sockaddr_in sa { 0 };
	socklen_t len = sizeof(sa);
	if (!getpeername(sockconn, (struct sockaddr*) & sa, &len))
		return ipnum(inet_ntoa(sa.sin_addr));
	else
		debug("get_peer_nip getpeername fail.");

	return 0;
}


u16 IpPort::get_peer_port(int sockconn)
{
	struct sockaddr_in sa { 0 };
	socklen_t len;
	if (!getpeername(sockconn, (struct sockaddr*) & sa, &len))
		return ntohs(sa.sin_port);
	else
		debug("get_peer_port getpeername fail.");

	return 0;
}


bool IpPort::isLAN(std::string const& ipString)
{
	istringstream st(ipString);
	int ip[2];
	for(int i = 0; i < 2; i++)
	{
		string temp;
		getline(st,temp,'.');
		istringstream a(temp);
		a >> ip[i];
	}
	if((ip[0]==10) || (ip[0]==172 && ip[1]>=16 && ip[1]<=31) || (ip[0]==192 && ip[1]==168))
	{
		return true;
	}else{
		return false;
	}
}


/*char* IpPort::getPublicIP(char *url)
{

	 int BUF_SIZE = 512;

    struct sockaddr_in pin;
    struct hostent *nlp_host;
    int sd = 0;
    int len = 0;
    char buf[BUF_SIZE] = { 0 };
    char myurl[100] = {0};
    char host[100] = { 0 };
    char GET[100] =  {0};
    char header[240] =  {0};
    char *pHost = 0;

    
    strcpy(myurl, url);
    for (pHost = myurl; *pHost != '/' && *pHost != '\0'; ++pHost) ;
    if ((unsigned int)(pHost - myurl) == strlen(myurl))
        strcpy(GET, "/");
    else
        strcpy(GET, pHost);
    *pHost = '\0';
    strcpy(host, myurl);

    
    if ((nlp_host = gethostbyname(host)) == 0)
    {
        perror("error get host\n");
        return NULL;
    }

    bzero(&pin, sizeof(pin));
    pin.sin_family = AF_INET;
    pin.sin_addr.s_addr = htonl(INADDR_ANY);
    pin.sin_addr.s_addr = ((struct in_addr *)(nlp_host->h_addr))->s_addr;
    pin.sin_port = htons(80);

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Error opening socket!!!\n");
        return NULL;
    }
    
	
	strcat(header, "GET");
	strcat(header, " ");
	strcat(header, GET);
	strcat(header, " ");
	strcat(header, "HTTP/1.1\r\n");
	strcat(header, "HOST:");
	strcat(header, host);
	strcat(header, "\r\n");*/
	
	


    
	
	/*if (::connect(sd, (sockaddr *) &pin, sizeof(pin)) == -1) {
		::close(sd);
		printf("error connect to socket\n");
		return NULL;
	}

	if (send(sd, header, strlen(header), 0) == -1) {
		::close(sd);
		perror("error in send \n");
		return NULL;
	}
    
	len = recv(sd, buf, BUF_SIZE, 0);
	if (len < 0) {
		::close(sd);
		printf("receive data error!!!\n");
		return NULL;
	} else {
		memset(g_publicIP, 0, sizeof(g_publicIP));
		
		
		sscanf(strstr(buf, "utf-8") + 9, "%*[^\n]\n%[^\n]", g_publicIP);
		
		::close(sd);
		return g_publicIP;
    }
}*/

/*int IpPort::execute_shell(std::string ip_port)
{
	debug(YELLOW "ip_port is : %s" RESET,ip_port.c_str());
	int i = 0;
   pid_t status;
   string str = "nc -v -z -w2 ";
   str+=ip_port;
   debug(YELLOW "str is : %s" RESET,str.c_str());
	status = system(str.c_str());

	if (-1 == status)
	{
	    printf("system error!");
	    i = -1;
	}
	else
	{
	    printf("exit status value = [0x%x]\n", status);

	    if (WIFEXITED(status))
	     {
	        if (0 == WEXITSTATUS(status))
	           {
	            printf("run shell script successfully.\n");
	            i = 1;
	           }
	        else
	           {
	            printf("run shell script fail, script exit code: %d\n", WEXITSTATUS(status));
	            i = -1;
	           }
	     }
	    else
	     {
	         printf("exit status = [%d]\n", WEXITSTATUS(status));
	     }
	}


	if(status == -1)
	{
	    printf("system error... \n");
	    i = -1;
	}
	else
	{
	    if(WIFEXITED(status))
	     {
	        if(0 == WEXITSTATUS(status))
	           {
	            printf("run  successfully... \n");
	            i = 1;
	           }
	        else
	           {
	            printf("run failed %d \n",WEXITSTATUS(status));
	            i = -1;
	           }
	     }
	    else
	     {
	        printf("exit code %d \n",WEXITSTATUS(status));
	     }
	}
	return i;
}*/

