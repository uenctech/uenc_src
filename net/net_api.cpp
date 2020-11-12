#include <sstream>
#include "net_api.h"
#include "./global.h"
#include <string>
#include <arpa/inet.h>
#include <signal.h>
#include <net/if.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <errno.h>
#include "./dispatcher.h"
#include "net.pb.h"
#include "common.pb.h"
#include "../../utils/time_task.h"
#include "../utils/singleton.h"
#include "./socket_buf.h"
#include "./work_thread.h"
#include "./epoll_mode.h"
#include "http_server.h"
#include <utility>

static std::atomic<bool> proxy_flag;

int net_tcp::Socket(int family, int type, int protocol)
{
	int n;

	if ((n = socket(family, type, protocol)) < 0)
		perror("can't create socket file");
	return n;
}

int net_tcp::Accept(int fd, struct sockaddr *sa, socklen_t *salenptr)
{
	int n;

	if ((n = accept(fd, sa, salenptr)) < 0)
	{
		if ((errno == ECONNABORTED) || (errno == EINTR) || (errno == EWOULDBLOCK))
		{
			goto ret;
		}
		else
		{
			perror("accept error");
		}
	}
ret:
	return n;
}

int net_tcp::Bind(int fd, const struct sockaddr *sa, socklen_t salen)
{
	int n;

	if ((n = bind(fd, sa, salen)) < 0)
		perror("bind error");
	return n;
}

int net_tcp::Connect(int fd, const struct sockaddr *sa, socklen_t salen)
{
	int n;
	
	int nBufLen;
	int nOptLen = sizeof(nBufLen);
	getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void *)&nBufLen, (socklen_t *)&nOptLen);
	
	info("socket recv buff default size %d !", nBufLen);

	int nRecvBuf = 1 * 1024 * 1024;
	Setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (const void *)&nRecvBuf, sizeof(int));
	int nSndBuf = 1 * 1024 * 1024;
	Setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (const void *)&nSndBuf, sizeof(int));

	getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void *)&nBufLen, (socklen_t *)&nOptLen);
	
	info("modify socket recv buff size %d !", nBufLen);
	
	if ((n = connect(fd, sa, salen)) < 0)
	{
		debug(RED "Connect fun  %d" RESET, n);
	}

	return n;
}

int net_tcp::Listen(int fd, int backlog)
{
	int n;

	if ((n = listen(fd, backlog)) < 0)
		perror("listen error");
	return n;
}

int net_tcp::Send(int sockfd, const void *buf, size_t len, int flags)
{
	if (sockfd < 0)
	{
		debug("Send 文件描述符错误...");
		return -1;
	}
	int bytes_left;
	int written_bytes;
	char *ptr;
	ptr = (char *)buf;
	bytes_left = len;
	while (bytes_left > 0)
	{
		written_bytes = write(sockfd, ptr, bytes_left);
		if (written_bytes <= 0) 
		{
			if (written_bytes == 0)
			{
				continue;
			}
			if (errno == EINTR)
			{
				continue;
			}
			else if (errno == EAGAIN) 
			{

				return len - bytes_left;
			}
			else 
			{
				error("net_tcp::Send write error delete node,fd is:%d",sockfd);
				Singleton<PeerNode>::get_instance()->delete_by_fd(sockfd);
				return -1;
			}
		}

		bytes_left -= written_bytes;
		ptr += written_bytes; 
	}
	return len;
}

int net_tcp::Setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen)
{
	int ret;

	if ((ret = setsockopt(fd, level, optname, optval, optlen)) == -1)
		perror("setsockopt error");
	return ret;
}

int net_tcp::listen_server_init(int port, int listen_num)
{
	struct sockaddr_in servaddr;
	int listener;
	int opt = 1;
	listener = Socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
	servaddr.sin_port = htons(port);

	Setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt,
			   sizeof(opt));
	Setsockopt(listener, SOL_SOCKET, SO_REUSEPORT, (const void *)&opt,
			   sizeof(opt));

	Bind(listener, (struct sockaddr *)&servaddr, sizeof(servaddr));

	int nRecvBuf = 1 * 1024 * 1024;
	Setsockopt(listener, SOL_SOCKET, SO_RCVBUF, (const void *)&nRecvBuf, sizeof(int));
	int nSndBuf = 1 * 1024 * 1024;
	Setsockopt(listener, SOL_SOCKET, SO_SNDBUF, (const void *)&nSndBuf, sizeof(int));
	Listen(listener, listen_num);

	return listener;
}

int net_tcp::set_fd_noblocking(int sockfd)
{
	if (fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0) | O_NONBLOCK) == -1)
	{
		perror("setnonblock error");
		return -1;
	}
	return 0;
}


int net_com::connect_init(u32 u32_ip, u16 u16_port)
{

	int confd = 0;
	struct sockaddr_in servaddr = {0};
	struct sockaddr_in my_addr = {0};
	int ret = 0;

	confd = Socket(AF_INET, SOCK_STREAM, 0);
	int flags = 1;
	Setsockopt(confd, SOL_SOCKET, SO_REUSEADDR, &flags, sizeof(int));
	flags = 1;
	Setsockopt(confd, SOL_SOCKET, SO_REUSEPORT, &flags, sizeof(int));

	
	memset(&my_addr, 0, sizeof(my_addr));
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(SERVERMAINPORT);

	ret = bind(confd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr));
	if (ret < 0)
		perror("bind hold port");

	
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(u16_port);
	struct in_addr addr = {0};
	memcpy(&addr, &u32_ip, sizeof(u32_ip));
	inet_pton(AF_INET, inet_ntoa(addr), &servaddr.sin_addr);

	
	if (set_fd_noblocking(confd) < 0)
	{
		debug("setnonblock error");
		return -1;
	}

	ret = Connect(confd, (struct sockaddr *)&servaddr, sizeof(servaddr));

	if (ret != 0)
	{
		if (errno == EINPROGRESS)
		{
			debug("Doing connection.");
			struct epoll_event newPeerConnectionEvent;
			int epollFD = -1;
			struct epoll_event processableEvents;
			unsigned int numEvents = -1;

			if ((epollFD = epoll_create(1)) == -1)
			{
				error("Could not create the epoll FD list!");
				close(confd);
				return -1;
			}     

			newPeerConnectionEvent.data.fd = confd;
			newPeerConnectionEvent.events = EPOLLOUT | EPOLLIN | EPOLLERR;

			if (epoll_ctl(epollFD, EPOLL_CTL_ADD, confd, &newPeerConnectionEvent) == -1)
			{
				error("Could not add the socket FD to the epoll FD list!");
				close(confd);
				close(epollFD);
				return -1;
			}

			numEvents = epoll_wait(epollFD, &processableEvents, 1, 3*1000);

			if (numEvents < 0)
			{
				error("Serious error in epoll setup: epoll_wait () returned < 0 status!");
				close(epollFD);
				close(confd);
				return -1;
			}
			int retVal = -1;
			socklen_t retValLen = sizeof (retVal);
			if (getsockopt(confd, SOL_SOCKET, SO_ERROR, &retVal, &retValLen) < 0)
			{
				error("getsockopt SO_ERROR error!");
				close(confd);
				close(epollFD);
				return -1;
			}

			if (retVal == 0)  
			{
				close(epollFD);
				return confd;
			} 
			else
			{
				error("getsockopt SO_ERROR retVal error : %s", strerror(retVal));
				close(epollFD);
				close(confd);
				return -1;
			}	
		}
		else
		{
			error("not EINPROGRESS: %s", strerror(errno));
			close(confd);
			return -1;			
		}
	}

	return confd;
}


bool net_com::send_one_message(const Node &to, const net_pack &pack)
{
	auto msg = Pack::packag_to_str(pack);	
	return send_one_message(to, msg);
}

bool net_com::send_one_message(const Node &to, const std::string &msg)
{
	MsgData send_data;
	send_data.type = E_WRITE;
	send_data.fd = to.fd;
	send_data.ip = to.public_ip;
	send_data.port = to.public_port;

	if (to.fd <= 0 && BYSERV != to.conn_kind)
	{
		return false;
	}

	if (BYSERV == to.conn_kind)
	{
		net_com::SendTransMsgReq(to, msg);
		return true;
	}

	Singleton<BufferCrol>::get_instance()->add_buffer(send_data.ip, send_data.port, send_data.fd);
	Singleton<BufferCrol>::get_instance()->add_write_pack(send_data.ip, send_data.port, msg);

	bool bRet = global::queue_write.push(send_data);
	return bRet;
}

bool net_com::send_one_message(const MsgData& to, const net_pack &pack)
{
	MsgData send_data;
	send_data.type = E_WRITE;
	send_data.fd = to.fd;
	send_data.ip = to.ip;
	send_data.port = to.port;

	auto msg = Pack::packag_to_str(pack);	
	Singleton<BufferCrol>::get_instance()->add_buffer(send_data.ip, send_data.port, send_data.fd);
	Singleton<BufferCrol>::get_instance()->add_write_pack(send_data.ip, send_data.port, msg);

	bool bRet = global::queue_write.push(send_data);
	return bRet;
}


uint64_t net_data::pack_port_and_ip(uint16_t port, uint32_t ip)
{
	uint64_t ret = port;
	ret = ret << 32 | ip;
	return ret;
}

uint64_t net_data::pack_port_and_ip(int port, std::string ip)
{
	uint64_t ret = port;
	uint32_t tmp;
	inet_pton(AF_INET, ip.c_str(), &tmp);
	ret = ret << 32 | tmp;
	return ret;
}
std::pair<uint16_t, uint32_t> net_data::apack_port_and_ip_to_int(uint64_t port_and_ip)
{
	uint64_t tmp = port_and_ip;
	uint32_t ip = tmp << 32 >> 32;
	uint16_t port = port_and_ip >> 32;
	return std::pair<uint16_t, uint32_t>(port, ip);
}
std::pair<int, std::string> net_data::apack_port_and_ip_to_str(uint64_t port_and_ip)
{
	uint64_t tmp = port_and_ip;
	uint32_t ip = tmp << 32 >> 32;
	uint16_t port = port_and_ip >> 32;
	char buf[100];
	inet_ntop(AF_INET, (void *)&ip, buf, 16);
	return std::pair<uint16_t, std::string>(port, buf);
}

int net_data::get_mac_info(vector<string> &vec)
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
			
			if (string(buf[interfaceNum].ifr_name) == "lo")
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

std::string net_data::get_mac_md5()
{
	std::vector<string> vec;
	std::string data;
	net_data::get_mac_info(vec);
	for (auto it = vec.begin(); it != vec.end(); ++it)
	{
		data += *it;
	}
	string md5 = getMD5hash(data);

	return md5;
}


int net_com::parse_conn_kind(Node &to)
{
	auto self = Singleton<PeerNode>::get_instance()->get_self_node();

	if (Singleton<Config>::get_instance()->GetIsPublicNode())
	{
		if (to.is_public_node == true)
		{
			to.conn_kind = DRTO2O; 
		}
		else if (to.is_public_node == false)
		{
			to.conn_kind = DRTO2I; 
		}
		else
		{
			debug("Public Ip connect error!");
			to.conn_kind = BYSERV;
			return -1;
		}
	}
	else if (!Singleton<Config>::get_instance()->GetIsPublicNode())
	{
		if (to.is_public_node == true)
		{
			to.conn_kind = DRTI2O; 
		}
		
		else if ((self.public_ip == to.public_ip && self.local_ip != to.local_ip) || (strncmp(IpPort::ipsz(self.public_ip), "192", 3) == 0 && strncmp(IpPort::ipsz(to.public_ip), "192", 3) == 0 && self.local_ip != to.local_ip) ||
				 (strncmp(IpPort::ipsz(to.public_ip), "192", 3) == 0 && self.local_ip != to.local_ip))
		{
			to.conn_kind = DRTI2I; 
		}
		else if (to.is_public_node == false)
		{
			to.conn_kind = BYSERV; 
		}
		else
		{
			debug("Local Ip connect error!");
			to.conn_kind = BYSERV;
			return -1;
		}
	}
	else
	{
		debug("unknwon conn_kind error!");
		to.conn_kind = BYSERV;
		return -1;
	}

	return to.conn_kind;
}







bool read_id_file()
{
	debug(YELLOW "read_id_file start" RESET);
	std::string strID = Singleton<Config>::get_instance()->GetKID();
	debug(YELLOW "read_id_file string strID(%s)" RESET, strID.c_str());
	if (strID == "")
	{
		Singleton<PeerNode>::get_instance()->make_rand_id();
		Singleton<Config>::get_instance()->SetKID(
			Singleton<PeerNode>::get_instance()->get_self_id()
		);
	}
	else
	{
		Singleton<PeerNode>::get_instance()->set_self_id(strID);
	}
	debug(YELLOW "read_id_file end" RESET);
	return true;
}

void handle_pipe(int sig)
{
	
}

bool net_com::net_init()
{
	
	global::cpu_nums = sysconf(_SC_NPROCESSORS_ONLN);
	
	uint32_t seed[1] = {0};
	srand((unsigned long)seed);

	
	struct sigaction sa;
	sa.sa_handler = handle_pipe;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGPIPE, &sa, NULL);

	
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGPIPE);
	sigprocmask(SIG_BLOCK, &set, NULL);

	
	signal(SIGPIPE, SIG_IGN);

	
	global::mac_md5 = net_data::get_mac_md5();
	Singleton<PeerNode>::get_instance()->set_self_mac_md5(global::mac_md5);

	
	char buf[1024] = {0};
	sprintf(buf, "firewall-cmd --add-port=%hu/tcp", SERVERMAINPORT);
	system(buf);


	
	Singleton<ProtobufDispatcher>::get_instance()->registerAll();

	
	global::nodelist_refresh_time = Singleton<Config>::get_instance()->GetVarInt("k_refresh_time");
	global::local_ip = Singleton<Config>::get_instance()->GetVarString("local_ip");

	if (global::nodelist_refresh_time <= 0)
	{
		global::nodelist_refresh_time = K_REFRESH_TIME;
	}

	
	if (false == read_id_file())
	{
		debug("创建或者读取自己ID失败");
		return false;
	}

	
	if (global::local_ip == "")
	{
		
		if (false == IpPort::get_localhost_ip())
		{
			debug("获取本机内网ip失败");
			return false;
		}
	}
	else
	{
		info("内网不为空");

        if(!Singleton<Config>::get_instance()->GetIsPublicNode())
        {
                if (false == IpPort::get_localhost_ip())
                {
                        debug("获取本机内网ip失败");
                        return false;
                }
        }
        if(Singleton<Config>::get_instance()->GetIsPublicNode())
        {
            Singleton<PeerNode>::get_instance()->set_self_ip_l(IpPort::ipnum(global::local_ip.c_str()));
            Singleton<PeerNode>::get_instance()->set_self_port_l(SERVERMAINPORT);
			Singleton<PeerNode>::get_instance()->set_self_ip_p(IpPort::ipnum(global::local_ip.c_str()));
			Singleton<PeerNode>::get_instance()->set_self_port_p(SERVERMAINPORT);
		}
	}
	
	Singleton<PeerNode>::get_instance()->set_self_public_node(Singleton<Config>::get_instance()->GetIsPublicNode());


	
	Singleton<WorkThreads>::get_instance()->start();

	
	Singleton<EpollMode>::get_instance()->start();
	
	
	Singleton<PeerNode>::get_instance()->nodelist_refresh_thread_init();

	
	global::g_timer.StartTimer(HEART_INTVL * 1000, net_com::DealHeart);


	
	HttpServer::start();


	return true;
}




int net_com::input_send_one_message()
{

	debug(RED "input_send_one_message start" RESET);
	string id;
	cout << "please input id:";
	cin >> id;

	while (true)
	{
		
		bool result = Singleton<PeerNode>::get_instance()->is_id_valid(id);
		if (false == result)
		{
			cout << "invalid id , please input id:";
			cin >> id;
			continue;
		}
		else
		{
			break;
		}
	};
	Node tmp_node;
	if (!Singleton<PeerNode>::get_instance()->find_node(id_type(id), tmp_node))
	{
		debug("invaild id, not in my peer node");
		return -1;
	}

	string msg;
	cout << "please input msg:";
	cin >> msg;

	int num;
	cout << "please input num:";
	cin >> num;

	bool bl;
	for (int i = 0; i < num; ++i)
	{
		bl = net_com::SendPrintMsgReq(tmp_node, msg);

		if (bl)
		{
			printf("第 %d 次发送成功\n", i + 1);
		}
		else
		{
			printf("第 %d 次发送失败\n", i + 1);
		}

	}
	return bl ? 0 : -1;
}


int net_com::test_broadcast_message()
{
	string str_buf = "Hello Canon!!!";

	PrintMsgReq printMsgReq;
	printMsgReq.set_data(str_buf);

	net_com::broadcast_message(printMsgReq);
	return 0;
}


bool net_com::test_send_big_data()
{
	string id;
	cout << "please input id:";
	cin >> id;
	auto is_vaild = [](string id_str) {
		int count = 0;
		for (auto i : id_str)
		{
			if (i != '1' || i != '0')
				return false;
			count++;
		}
		return count == 16;
	};
	while (is_vaild(id))
	{
		cout << "invalid id , please input id:";
		cin >> id;
	};
	Node tmp_node;
	if (!Singleton<PeerNode>::get_instance()->find_node(id_type(id), tmp_node))
	{
		debug("invaild id, not in my peer node");
		return false;
	}
	string tmp_data;
	int txtnum;
	cout << "please input test byte num:";
	cin >> txtnum;
	for (int i = 0; i < txtnum; i++)
	{
		char x, s;									  
		s = (char)rand() % 2;						  
		if (s == 1)									  
			x = (char)rand() % ('Z' - 'A' + 1) + 'A'; 
		else
			x = (char)rand() % ('z' - 'a' + 1) + 'a'; 
		tmp_data.push_back(x);						  
	}
	tmp_data.push_back('z');
	tmp_data.push_back('z');
	tmp_data.push_back('z');
	tmp_data.push_back('z');
	tmp_data.push_back('z');


	net_com::SendPrintMsgReq(tmp_node, tmp_data, 1);
	return true;
}



void net_com::InitRegisterNode()
{
	vector<Node> nodelist;
	auto serverlist = Singleton<Config>::get_instance()->GetServerList();
	for (auto server : serverlist)
	{
		Node node;
		std::string ip = std::get<0>(server);
		int port = std::get<1>(server);
		node.local_ip = IpPort::ipnum(ip);
		node.public_ip = IpPort::ipnum(ip);
		node.local_port = port;
		node.public_port = port;
		node.is_public_node = true;
		if (ip.size() > 0 && ip != global::local_ip)
		{
			nodelist.push_back(node);
		}
	}

	std::random_shuffle(nodelist.begin(), nodelist.end());
	bool get_nodelist_flag = true;
	for (auto &node : nodelist)
	{	
		if (node.is_public_node)
		{
			bool res = net_com::SendRegisterNodeReq(node, get_nodelist_flag);
			if(res){
				get_nodelist_flag = false;
			}
		}
	}
}


bool net_com::SendPrintMsgReq(Node &to, const std::string data, int type)
{
	PrintMsgReq printMsgReq;
	printMsgReq.set_data(data);
	printMsgReq.set_type(type);
	net_com::send_message(to, printMsgReq);
	return true;
}


bool net_com::SendRegisterNodeReq(Node& dest, bool get_nodelist)
{
	RegisterNodeReq getNodes;
	getNodes.set_is_get_nodelist(get_nodelist);
	NodeInfo* mynode = getNodes.mutable_mynode();
	mynode->set_local_ip( Singleton<PeerNode>::get_instance()->get_self_node().local_ip);
	mynode->set_local_port( Singleton<PeerNode>::get_instance()->get_self_node().local_port);
	mynode->set_is_public_node(Singleton<Config>::get_instance()->GetIsPublicNode());
	mynode->set_mac_md5(global::mac_md5);

	auto self_id = Singleton<PeerNode>::get_instance()->get_self_id();
	mynode->set_node_id(self_id);
	mynode->set_fee( Singleton<PeerNode>::get_instance()->get_self_node().fee );
	mynode->set_package_fee(Singleton<PeerNode>::get_instance()->get_self_node().package_fee);
	mynode->set_base58addr( Singleton<PeerNode>::get_instance()->get_self_node().base58address );

	u32 dest_ip = dest.public_ip;
	u16 port = dest.public_port;
	int fd = dest.fd;
	
	if (fd > 0)
	{
		net_com::send_message(dest, getNodes);
		return true;
	}
	else
	{
		int cfd = connect_init(dest_ip, port);
		if (cfd <= 0)
		{
			return false;
		}

		dest.fd = cfd;
		Singleton<BufferCrol>::get_instance()->add_buffer(dest_ip, port, cfd);
		Singleton<EpollMode>::get_instance()->add_epoll_event(cfd, EPOLLIN | EPOLLET);
		if(get_nodelist) {
			net_com::parse_conn_kind(dest);
			Singleton<PeerNode>::get_instance()->add(dest);
		}
		
		net_com::send_message(dest, getNodes);
	}

	return true;
}


void net_com::SendConnectNodeReq(Node& dest)
{
	ConnectNodeReq connectNodeReq;

	NodeInfo* mynode = connectNodeReq.mutable_mynode();
	mynode->set_local_ip( Singleton<PeerNode>::get_instance()->get_self_node().local_ip);
	mynode->set_local_port( Singleton<PeerNode>::get_instance()->get_self_node().local_port);
	
	
	mynode->set_is_public_node(Singleton<Config>::get_instance()->GetIsPublicNode());
	mynode->set_mac_md5(global::mac_md5);
	mynode->set_conn_kind(dest.conn_kind);
	mynode->set_fee(Singleton<PeerNode>::get_instance()->get_self_node().fee);
	mynode->set_package_fee(Singleton<PeerNode>::get_instance()->get_self_node().package_fee);
	mynode->set_base58addr( Singleton<PeerNode>::get_instance()->get_self_node().base58address );

	auto self_id = Singleton<PeerNode>::get_instance()->get_self_id();
	mynode->set_node_id(self_id);

	if(dest.conn_kind == BYSERV)
	{
		CommonMsg msg;
		Pack::InitCommonMsg(msg, connectNodeReq);

		net_pack pack;
		Pack::common_msg_to_pack(msg, pack);		
		auto msg1 = Pack::packag_to_str(pack);
		net_com::SendTransMsgReq(dest, msg1);
	}else
	{
		net_com::send_message(dest, connectNodeReq);
	}
}


void net_com::SendTransMsgReq(Node dest, const std::string msg)
{
	TransMsgReq transMsgReq;

	NodeInfo* destnode = transMsgReq.mutable_dest();
	destnode->set_node_id(dest.id);

	transMsgReq.set_data(msg);


	vector<Node> nodelist = Singleton<PeerNode>::get_instance()->get_nodelist(NODE_PUBLIC);

	std::random_shuffle(nodelist.begin(), nodelist.end());
	for(auto node:nodelist)
	{
		net_com::send_message(node, transMsgReq);
		return;
	}
	error("SendTransMsgReq error no public node!");
	return;
}

void net_com::SendNotifyConnectReq(const Node& dest)
{
	NotifyConnectReq notifyConnectReq;

	NodeInfo* server_node = notifyConnectReq.mutable_server_node();
	server_node->set_node_id(Singleton<PeerNode>::get_instance()->get_self_node().id);
	server_node->set_local_ip( Singleton<PeerNode>::get_instance()->get_self_node().local_ip);
	server_node->set_local_port( Singleton<PeerNode>::get_instance()->get_self_node().local_port);
	server_node->set_public_ip( Singleton<PeerNode>::get_instance()->get_self_node().public_ip);
	server_node->set_public_port( Singleton<PeerNode>::get_instance()->get_self_node().public_port);	
	server_node->set_is_public_node(Singleton<Config>::get_instance()->GetIsPublicNode());
	server_node->set_mac_md5(global::mac_md5);
	server_node->set_fee(Singleton<PeerNode>::get_instance()->get_self_node().fee);
	server_node->set_package_fee(Singleton<PeerNode>::get_instance()->get_self_node().package_fee);
	server_node->set_base58addr( Singleton<PeerNode>::get_instance()->get_self_node().base58address );

	NodeInfo* client_node = notifyConnectReq.mutable_client_node();
	client_node->set_node_id(dest.id);

	vector<Node> nodelist = Singleton<PeerNode>::get_instance()->get_nodelist(NODE_PUBLIC);
	for(auto node:nodelist)
	{
		net_com::send_message(node, notifyConnectReq);
	}
}

void net_com::SendPingReq(const Node& dest)
{
	PingReq pingReq;
	pingReq.set_id(Singleton<PeerNode>::get_instance()->get_self_id());
	net_com::send_message(dest, pingReq);
}



void net_com::SendPongReq(const Node& dest)
{
	PongReq pongReq;
	pongReq.set_id(Singleton<PeerNode>::get_instance()->get_self_id());
	net_com::send_message(dest, pongReq);
}

void net_com::DealHeart()
{
	Node mynode = Singleton<PeerNode>::get_instance()->get_self_node();
	vector<Node> nodelist = Singleton<PeerNode>::get_instance()->get_nodelist();

	for(auto &node:nodelist)
	{
		if(mynode.is_public_node)
		{
			node.heart_time -= HEART_INTVL;
			node.heart_probes -= 1;
			if(node.heart_probes <= 0)
			{
				node.print();
				Singleton<PeerNode>::get_instance()->delete_node(node.id);
			}else
			{
				net_com::SendPingReq(node);
			}
			Singleton<PeerNode>::get_instance()->update(node);
		}
	}	
}

bool net_com::SendSyncNodeReq(const Node& dest)
{
	SyncNodeReq syncNodeReq;
	vector<Node> nodelist = Singleton<PeerNode>::get_instance()->get_nodelist();
	for(auto& node:nodelist)
	{	
		syncNodeReq.add_ids(std::move(node.id));
	}
	return net_com::send_message(dest, syncNodeReq);
}

