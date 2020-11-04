#ifndef _NET_API_H_
#define _NET_API_H_

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <assert.h>
#include <netinet/tcp.h>
#include <utility>
#include <string>
#include <set>
#include <unordered_map>
#include <random>
#include <chrono>
#include <stdexcept>      

#include "./peer_node.h"
#include "./pack.h"
#include "../../common/config.h"
#include "../../ca/Crypto_ECDSA.h"
#include "./ip_port.h"
#include "./pack.h"
#include "../include/logging.h"
#include "common.pb.h"
#include "./socket_buf.h"
#include "./global.h"

namespace net_tcp
{
	int Accept(int fd, struct sockaddr *sa, socklen_t *salenptr);
	int Bind(int fd, const struct sockaddr *sa, socklen_t salen);
	int Connect(int fd, const struct sockaddr *sa, socklen_t salen);
	int Listen(int fd, int backlog);
	int Socket(int family, int type, int protocol);
	int Send(int sockfd, const void *buf, size_t len, int flags);
	int Setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen);
	int listen_server_init(int port, int listen_num);
	int set_fd_noblocking(int sockfd);

} 

namespace net_data
{
	uint64_t pack_port_and_ip(uint16_t por, uint32_t ip);
	uint64_t pack_port_and_ip(int port, std::string ip);
	std::pair<uint16_t, uint32_t> apack_port_and_ip_to_int(uint64_t port_and_ip);
	std::pair<int,std::string> apack_port_and_ip_to_str(uint64_t port_and_ip);

	int get_mac_info(vector<string> &vec);
	std::string get_mac_md5();
}

namespace net_com
{
	using namespace net_tcp;
	using namespace net_data;

	int connect_init(u32 u32_ip, u16 u16_port);
	bool send_one_message(const Node& to, const net_pack& pack);
	bool send_one_message(const Node &to, const std::string &msg);
	bool send_one_message(const MsgData& to, const net_pack &pack);
	template <typename T>
	bool send_message(const std::string id, T& msg, bool iscompress = false);

	template <typename T>
	bool send_message(const Node &dest, T& msg, bool iscompress = false);

	template <typename T>
	bool send_message(const MsgData& from, T& msg, bool iscompress = false);	

	template <typename T>
	bool broadcast_message(T& msg, bool iscompress = false);

	int parse_conn_kind(Node &to);

	bool net_init();
	int input_send_one_message();
	bool test_send_big_data();
	int test_broadcast_message();


	bool SendPrintMsgReq(Node &to, const std::string data, int type = 0);
	bool SendRegisterNodeReq(Node& dest, bool get_nodelist);
	void SendConnectNodeReq(Node& dest);
	void SendTransMsgReq(Node dest, const std::string msg);
	void SendNotifyConnectReq(const Node& dest);
	void SendPingReq(const Node& dest);
	void SendPongReq(const Node& dest);
	
	void DealHeart();
	bool SendSyncNodeReq(const Node& dest);
	void InitRegisterNode();
}



template <typename T>
bool net_com::send_message(const Node &dest, T& msg, bool iscompress)
{
	CommonMsg comm_msg;
	Pack::InitCommonMsg(comm_msg, msg, 0, iscompress);
	
	net_pack pack;
	Pack::common_msg_to_pack(comm_msg, pack);

	return net_com::send_one_message(dest, pack);
}


template <typename T>
bool net_com::send_message(const std::string id, T& msg, bool iscompress)
{
	Node node;
	auto find = Singleton<PeerNode>::get_instance()->find_node(id, node);
	if (find){
		return net_com::send_message(node, msg, iscompress);
	}else{
		error("send_message no id:%s", id.c_str());
		return false;
	}	
}

template <typename T>
bool net_com::send_message(const MsgData& from, T& msg, bool iscompress)
{
	Node node;
	auto find = Singleton<PeerNode>::get_instance()->find_node_by_fd(from.fd, node);
	if (find){
		return net_com::send_message(node, msg, iscompress);
	}else{
		CommonMsg comm_msg;
		Pack::InitCommonMsg(comm_msg, msg, 0, iscompress);
		
		net_pack pack;
		Pack::common_msg_to_pack(comm_msg, pack);		
		return net_com::send_one_message(from, pack);
	}	
}


template <typename T>
bool net_com::broadcast_message(T& msg, bool iscompress)
{
	auto nodelist = Singleton<PeerNode>::get_instance()->get_nodelist();
	for(auto& node:nodelist)
	{
		net_com::send_message(node, msg, iscompress);
	}
	return true;
}

#endif