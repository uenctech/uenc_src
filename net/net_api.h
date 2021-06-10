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
#include <stdexcept>      // std::invalid_argument
#include <functional>

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
#include "handle_event.h"
#include "../utils/util.h"

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

	/*udp broadcast start*/
	ssize_t Sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr * to, int tolen);
	ssize_t Recvfrom(int sockfd, const void *buf, size_t len, int flags, struct sockaddr *from, int *fromlen);
	/*udp broadcast end*/



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

	enum class Compress : uint8_t
	{
		kCompress_False = 0,
		kCompress_True = 1
	};

	enum class Encrypt : uint8_t
	{
		kEncrypt_False = 0,
		kEncrypt_True = 1,
	};

	enum class Priority : uint8_t
	{
		kPriority_Low_0 = 0,
		kPriority_Low_1 = 2,
		kPriority_Low_2 = 4,

		kPriority_Middle_0 = 5,
		kPriority_Middle_1 = 8,
		kPriority_Middle_2 = 10,

		kPriority_High_0 = 11,
		kPriority_High_1 = 14,
		kPriority_High_2 = 15,
	};

	int connect_init(u32 u32_ip, u16 u16_port);
	bool send_one_message(const Node& to, const net_pack& pack);
	bool send_one_message(const Node &to, const std::string &msg, const int8_t priority);
	bool send_one_message(const MsgData& to, const net_pack &pack);

	template <typename T>
	bool send_message(const std::string id, 
						T& msg, 
						const net_com::Compress isCompress = net_com::Compress::kCompress_False, 
						const net_com::Encrypt isEncrypt = net_com::Encrypt::kEncrypt_False, 
						const net_com::Priority priority = net_com::Priority::kPriority_Low_0);

	template <typename T>
	bool send_message(const Node &dest, 
						T& msg, 
						const net_com::Compress isCompress = net_com::Compress::kCompress_False, 
						const net_com::Encrypt isEncrypt = net_com::Encrypt::kEncrypt_False, 
						const net_com::Priority priority = net_com::Priority::kPriority_Low_0);

	template <typename T>
	bool send_message(const MsgData& from, 
						T& msg, 
						const net_com::Compress isCompress = net_com::Compress::kCompress_False, 
						const net_com::Encrypt isEncrypt = net_com::Encrypt::kEncrypt_False, 
						const net_com::Priority priority = net_com::Priority::kPriority_Low_0);	

	template <typename T>
	bool broadcast_message(T& msg, 
							const net_com::Compress isCompress = net_com::Compress::kCompress_False, 
							const net_com::Encrypt isEncrypt = net_com::Encrypt::kEncrypt_False, 
							const net_com::Priority priority = net_com::Priority::kPriority_Low_0);

	int parse_conn_kind(Node &to);
	
	bool is_need_send_trans_message(const Node & to);	

	bool net_init();
	int input_send_one_message();
	bool test_send_big_data();
	int test_broadcast_message();


	bool SendPrintMsgReq(Node &to, const std::string data, int type = 0);
	bool SendPrintMsgReq(const std::string & id, const std::string data, int type = 0);
	bool SendRegisterNodeReq(Node& dest, bool get_nodelist);
	void SendConnectNodeReq(Node& dest);
	void SendBroadcastNodeReq();
	void SendTransMsgReq(Node dest, const std::string msg, int8_t priority);
	void SendNotifyConnectReq(const Node& dest);
	void SendPingReq(const Node& dest);
	void SendPongReq(const Node& dest);
	void SendGetHeightReq(const Node& dest, bool is_fetch_public);
	void RegisteToPublic();//liuzg
	void DealHeart();
	bool SendSyncNodeReq(const Node& dest);
	void InitRegisterNode();

	/*Send broadcast */
	ssize_t SendBroadcastMsg(const std::string &msg);
	/*Receive broadcast data thread */
	bool handleBroadcastMsgThread();
	void RecvfromBroadcastMsg();
}

namespace net_callback
{
    void register_chain_height_callback(std::function<unsigned int(void)> callback);

    extern std::function<unsigned int(void)> chain_height_callback;
}

template <typename T>
bool net_com::send_message(const Node &dest, T& msg, const net_com::Compress isCompress, const net_com::Encrypt isEncrypt, const net_com::Priority priority)
{
	CommonMsg comm_msg;
	Pack::InitCommonMsg(comm_msg, msg, (uint8_t)isEncrypt, (uint8_t)isCompress);
	
	net_pack pack;
	Pack::common_msg_to_pack(comm_msg, (uint8_t)priority, pack);

	return net_com::send_one_message(dest, pack);
}

template <typename T>
bool net_com::send_message(const std::string id, T& msg, const net_com::Compress isCompress, const net_com::Encrypt isEncrypt, const net_com::Priority priority)
{
	Node node;
	auto find = Singleton<PeerNode>::get_instance()->find_node(id, node);
	if (find)
	{
		return net_com::send_message(node, msg, isCompress, isEncrypt, priority);
	}
	else
	{
		Node transNode;
		transNode.id = id;
		return net_com::send_message(transNode, msg, isCompress, isEncrypt, priority);
	}	
}

template <typename T>
bool net_com::send_message(const MsgData& from, T& msg, const net_com::Compress isCompress, const net_com::Encrypt isEncrypt, const net_com::Priority priority)
{
	Node node;
	auto find = Singleton<PeerNode>::get_instance()->find_node_by_fd(from.fd, node);
	if (find)
	{
		return net_com::send_message(node, msg, isCompress, isEncrypt, priority);
	}
	else
	{
		CommonMsg comm_msg;
		Pack::InitCommonMsg(comm_msg, msg, (uint8_t)isEncrypt, (uint8_t)isCompress);
		
		net_pack pack;
		Pack::common_msg_to_pack(comm_msg, (uint8_t)priority, pack);		
		return net_com::send_one_message(from, pack);
	}	
}


template <typename T>
bool net_com::broadcast_message(T& msg, const net_com::Compress isCompress, const net_com::Encrypt isEncrypt, const net_com::Priority priority)
{
	CommonMsg comm_msg;
	Pack::InitCommonMsg(comm_msg, msg, (uint8_t)isEncrypt, (uint8_t)isCompress);
	net_pack pack;
	Pack::common_msg_to_pack(comm_msg, (uint8_t)priority, pack);

	const Node & selfNode = Singleton<PeerNode>::get_instance()->get_self_node();

	BroadcaseMsgReq req;
	NodeInfo* pNodeInfo = req.mutable_from();
	pNodeInfo->set_node_id(selfNode.id);
	pNodeInfo->set_is_public_node(selfNode.is_public_node);

	req.set_data(comm_msg.SerializeAsString());
	req.set_priority((uint32_t)priority);

	std::string toId;
	if (selfNode.is_public_node)
	{
		const Node & selfNode = Singleton<PeerNode>::get_instance()->get_self_node();

		// Send to its child node 
		const std::vector<Node> subNodeList = Singleton<PeerNode>::get_instance()->get_sub_nodelist(selfNode.id);
		for (auto & item : subNodeList)
		{
			if (req.from().node_id() != item.id && item.is_public_node == false)
			{
				net_com::send_message(item, req);
			}
		}
	
		// Forward to other public network nodes 
		req.mutable_from()->set_is_public_node(selfNode.is_public_node);
		req.mutable_from()->set_node_id(selfNode.id);

		const std::vector<Node> publicNodeList = Singleton<PeerNode>::get_instance()->get_nodelist(NODE_PUBLIC);
		for (auto & item : publicNodeList)
		{
			if (req.from().node_id() != item.id)
			{
				net_com::send_message(item, req);
			}
		}

	}
	else
	{
		net_com::send_message(selfNode.public_node_id, req, isCompress, isEncrypt, priority );
	}
	
	return true;

}

#endif
