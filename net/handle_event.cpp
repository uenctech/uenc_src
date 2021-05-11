#include <fstream>
#include "handle_event.h"
#include "../include/logging.h"
#include "./pack.h"
#include "./ip_port.h"
#include "peer_node.h"
#include "../include/net_interface.h"
#include "./global.h"
#include "../version_update/TcpSocket.h"
#include "net.pb.h"
#include "common.pb.h"
#include "dispatcher.h"
#include "../common/config.h"
#include "socket_buf.h"
#include <unordered_set>
#include <utility>
#include "node_cache.h"
#include "../common/global.h"
#include "global.h"
#include "../ca/MagicSingleton.h"
#include "../ca/ca_rocksdb.h"
#include "../include/ScopeGuard.h"

static int PrintMsNum = 0;
void handlePrintMsgReq(const std::shared_ptr<PrintMsgReq> &printMsgReq, const MsgData &from)
{
	int type = printMsgReq->type();
	if (type == 0)
	{
		std::cout << ++PrintMsNum << " times data:" << printMsgReq->data() << std::endl;
	}
	else
	{
		ofstream file("bigdata.txt", fstream::out);
		file << printMsgReq->data();
		file.close();
		cout << "write bigdata.txt success!!!" << endl;
	}
}

void handleRegisterNodeReq(const std::shared_ptr<RegisterNodeReq> &registerNode, const MsgData &from)
{
	std::cout << "handleRegisterNodeReq start ======" << std::endl;
	std::cout << "handleRegisterNodeReq from.port:" << from.port << std::endl;
	std::cout << "handleRegisterNodeReq from.ip:" << IpPort::ipsz(from.ip) << std::endl;
	NodeInfo *nodeinfo = registerNode->mutable_mynode();

	if (!Singleton<Config>::get_instance()->GetIsPublicNode())
	{
		return;
	}
	std::string destid = nodeinfo->node_id();
	auto public_self_id = Singleton<PeerNode>::get_instance()->get_self_id();
	Node node;
	node.fd = from.fd;
	node.id = destid;
	node.local_ip = nodeinfo->local_ip();
	node.local_port = nodeinfo->local_port();
	node.public_ip = from.ip;
	node.public_port = from.port;
	node.mac_md5 = nodeinfo->mac_md5();
	node.is_public_node = nodeinfo->is_public_node();
	node.fee = nodeinfo->fee();
	node.package_fee = nodeinfo->package_fee();
	node.base58address = nodeinfo->base58addr();
	node.chain_height = nodeinfo->chain_height();
	node.public_node_id = node.is_public_node ? "" : public_self_id;

	// 修正某些公网外网端口不正确
	if (nodeinfo->is_public_node())
	{
		if (from.port == SERVERMAINPORT)
		{
			node.public_port = from.port;
		}
		else
		{
			node.public_port = SERVERMAINPORT;
		}
	}

	Node tem_node;
	auto find = Singleton<PeerNode>::get_instance()->find_node(node.id, tem_node);
	if ((find && tem_node.conn_kind == NOTYET) || !find)
	{
		node.conn_kind = PASSIV;
	}
	else
	{
		node.conn_kind = tem_node.conn_kind;
	}

	if (find)
	{
		if (tem_node.fd != from.fd)
		{
			std::cout << "if(tem_node.fd != from.fd )" << std::endl;
			close(tem_node.fd);
			Singleton<BufferCrol>::get_instance()->delete_buffer(tem_node.public_ip, tem_node.public_port);
			Singleton<BufferCrol>::get_instance()->add_buffer(from.ip, from.port, from.fd);
		}
		Singleton<PeerNode>::get_instance()->update(node);
		if (node.is_public_node)
		{
			Singleton<PeerNode>::get_instance()->update_public_node(node);
		}
	}
	else
	{
		Singleton<PeerNode>::get_instance()->add(node);
		if (node.is_public_node)
		{
			Singleton<PeerNode>::get_instance()->add_public_node(node);
		}
	}

	Node selfNode = Singleton<PeerNode>::get_instance()->get_self_node();

	RegisterNodeAck registerNodeAck;
	std::vector<Node> nodelist;

	if (nodeinfo->is_public_node() && selfNode.is_public_node)
	{
		// std::vector<Node> tmp = Singleton<PeerNode>::get_instance()->get_nodelist();
		std::vector<Node> tmp = Singleton<PeerNode>::get_instance()->get_sub_nodelist(selfNode.id);
		nodelist.insert(nodelist.end(), tmp.begin(), tmp.end());
		std::vector<Node> publicNode = Singleton<PeerNode>::get_instance()->get_public_node();
		nodelist.insert(nodelist.end(), publicNode.begin(), publicNode.end());
	}
	else if (!nodeinfo->is_public_node() && selfNode.is_public_node)
	{
		nodelist = Singleton<PeerNode>::get_instance()->get_public_node();
	}

	nodelist.push_back(selfNode);
	nodelist.push_back(node);

	for (auto &node : nodelist)
	{

		if (node.is_public_node && node.fd < 0) //liuzg
		{
			if (node.id != selfNode.id)
			{
				continue;
			}
		}

		if (g_testflag == 0 && node.is_public_node) //liuzg
		{
			u32 &localIp = node.local_ip;
			u32 &publicIp = node.public_ip;

			if (localIp != publicIp || IpPort::is_public_ip(localIp) == false)
			{
				continue;
			}
		}
		NodeInfo *nodeinfo = registerNodeAck.add_nodes();
		//std::cout << "--------for(auto & node : nodelist)node.id" << node.id << std::endl;
		nodeinfo->set_node_id(node.id);
		nodeinfo->set_local_ip(node.local_ip);
		nodeinfo->set_local_port(node.local_port);
		nodeinfo->set_public_ip(node.public_ip);
		nodeinfo->set_public_port(node.public_port);
		nodeinfo->set_is_public_node(node.is_public_node);
		nodeinfo->set_mac_md5(node.mac_md5);
		nodeinfo->set_fee(node.fee);
		nodeinfo->set_package_fee(node.package_fee);
		nodeinfo->set_base58addr(node.base58address);
		nodeinfo->set_chain_height(node.chain_height);
		nodeinfo->set_public_node_id(node.public_node_id);
	}

	net_com::send_message(destid, registerNodeAck, net_com::Compress::kCompress_True, net_com::Encrypt::kEncrypt_False, net_com::Priority::kPriority_High_2);
}

void handleRegisterNodeAck(const std::shared_ptr<RegisterNodeAck> &registerNodeAck, const MsgData &from)
{
	// std::cout << "handleRegisterNodeAck start ======" << std::endl;
	// std::cout << "handleRegisterNodeAck from.fd:" << from.fd << std::endl;
	// std::cout << "handleRegisterNodeAck from.ip:" << IpPort::ipsz(from.ip) << std::endl;
	auto self_node = Singleton<PeerNode>::get_instance()->get_self_node();
	//std::cout << "handleRegisterNodeAck self_node_id:" << self_node.id << std::endl;
	for (int i = 0; i < registerNodeAck->nodes_size(); i++)
	{
		const NodeInfo &nodeinfo = registerNodeAck->nodes(i);
		Node node;
		node.id = nodeinfo.node_id();
		node.local_ip = nodeinfo.local_ip();
		node.local_port = nodeinfo.local_port();
		node.public_ip = nodeinfo.public_ip();
		node.public_port = nodeinfo.public_port();
		node.mac_md5 = nodeinfo.mac_md5();
		node.is_public_node = nodeinfo.is_public_node();
		node.fee = nodeinfo.fee();
		node.package_fee = nodeinfo.package_fee();
		node.base58address = nodeinfo.base58addr();
		node.chain_height = nodeinfo.chain_height();
		node.public_node_id = nodeinfo.public_node_id();
		/*
        if(g_testflag == 0 && node.is_public_node)   //liuzg
		{
			u32 & localIp = node.local_ip;
			u32 & publicIp = node.public_ip;

			if (localIp != publicIp || IpPort::is_public_ip(localIp) == false)
			{
				continue;
			}
		}	*/
		//处理公网节点的信息
		if (from.ip == node.public_ip && from.port == node.public_port)
		{
			node.fd = from.fd;
			net_com::parse_conn_kind(node);
		}
		//std::cout << "handleRegisterNodeAck node.id:" << node.id << std::endl;
		if (node.id != Singleton<PeerNode>::get_instance()->get_self_id())
		{
			Node temp_node;
			bool find_result = Singleton<PeerNode>::get_instance()->find_node(node.id, temp_node);
			if (find_result)
			{
				// std::cout << "handleRegisterNodeAck update 175:" << node.fd << std::endl;
				// Singleton<PeerNode>::get_instance()->update(node);
				// if(node.is_public_node)
				// {
				// 	std::cout << "handleRegisterNodeAck update 179:" << node.fd << std::endl;
				// 	Singleton<PeerNode>::get_instance()->update_public_node(node);
				// }
			}
			else
			{
				// std::cout << "handleRegisterNodeAck add 185:" << node.fd << std::endl;
				// std::cout << "handleRegisterNodeAck add 186:" << IpPort::ipsz(node.public_ip) << std::endl;
				Singleton<PeerNode>::get_instance()->add(node);
				if (node.is_public_node)
				{
					Singleton<PeerNode>::get_instance()->add_public_node(node);
				}
			}
		}
		else if (node.id == Singleton<PeerNode>::get_instance()->get_self_id() && !Singleton<Config>::get_instance()->GetIsPublicNode())
		{
			Singleton<PeerNode>::get_instance()->set_self_ip_p(node.public_ip);
			Singleton<PeerNode>::get_instance()->set_self_port_p(node.public_port);
			Singleton<PeerNode>::get_instance()->set_self_public_node_id(node.public_node_id);
		}
		//std::cout << "handleRegisterNodeAck update 200:" << node.fd << std::endl;
		if (node.is_public_node)
		{
			if (self_node.is_public_node && node.fd > 0)
			{
				Singleton<PeerNode>::get_instance()->update(node);
				Singleton<PeerNode>::get_instance()->update_public_node(node);
			}
		}
		else
		{
			Singleton<PeerNode>::get_instance()->update(node);
		}
	}
	Singleton<PeerNode>::get_instance()->conect_nodelist();
}

void handleConnectNodeReq(const std::shared_ptr<ConnectNodeReq> &connectNodeReq, const MsgData &from)
{
	auto self_node = Singleton<PeerNode>::get_instance()->get_self_node();
	NodeInfo *nodeinfo = connectNodeReq->mutable_mynode();

	Node node;
	node.fd = from.fd;
	node.id = nodeinfo->node_id();
	node.local_ip = nodeinfo->local_ip();
	node.local_port = nodeinfo->local_port();
	node.mac_md5 = nodeinfo->mac_md5();
	node.is_public_node = nodeinfo->is_public_node();
	node.fee = nodeinfo->fee();
	node.package_fee = nodeinfo->package_fee();
	node.base58address = nodeinfo->base58addr();
	node.chain_height = nodeinfo->chain_height();
	node.public_node_id = nodeinfo->public_node_id();

	if (nodeinfo->conn_kind() != BYSERV)
	{
		node.conn_kind = PASSIV;
		node.public_ip = from.ip;
		node.public_port = from.port;
	}
	else
	{
		node.conn_kind = BYSERV;
		node.fd = -2;
		// node.public_ip      = nodeinfo->public_port();
		// node.public_port    =  nodeinfo->public_port();
	}

	Node tem_node;
	auto find = Singleton<PeerNode>::get_instance()->find_node(node.id, tem_node);
	if (find)
	{
		if (tem_node.fd != from.fd && node.conn_kind != BYSERV)
		{
			close(tem_node.fd);
			Singleton<BufferCrol>::get_instance()->delete_buffer(tem_node.public_ip, tem_node.public_port);
			Singleton<BufferCrol>::get_instance()->add_buffer(from.ip, from.port, from.fd);
		}
		else if (node.conn_kind == BYSERV && tem_node.fd > 0)
		{
			return;
		}
		if (self_node.id != node.id)
		{
			Singleton<PeerNode>::get_instance()->update(node);
			if (node.is_public_node)
			{
				Singleton<PeerNode>::get_instance()->update_public_node(node);
			}
		}
	}
	else
	{
		if (self_node.id != node.id)
		{
			Singleton<PeerNode>::get_instance()->add(node);
			if (node.is_public_node)
			{
				Singleton<PeerNode>::get_instance()->add_public_node(node);
			}
		}
	}
}

void handleBroadcastNodeReq(const std::shared_ptr<BroadcastNodeReq> &broadcastNodeReq, const MsgData &from)
{
	NodeInfo *nodeinfo = broadcastNodeReq->mutable_mynode();

	Node node;
	node.fd = from.fd;
	node.id = nodeinfo->node_id();
	node.local_ip = nodeinfo->local_ip();
	node.local_port = nodeinfo->local_port();
	node.mac_md5 = nodeinfo->mac_md5();
	node.is_public_node = nodeinfo->is_public_node();
	node.fee = nodeinfo->fee();
	node.package_fee = nodeinfo->package_fee();
	node.base58address = nodeinfo->base58addr();
	node.public_node_id = nodeinfo->public_node_id();

	//根据节点id，从k桶中查找
	Node tmp_node;
	bool find = Singleton<PeerNode>::get_instance()->find_node(node.id, tmp_node);
	if (!find)
	{
		//将节点存入k桶中
		Singleton<PeerNode>::get_instance()->add(node);
		if (node.is_public_node)
		{
			Singleton<PeerNode>::get_instance()->add_public_node(node);
		}
	}
}

void handleTransMsgReq(const std::shared_ptr<TransMsgReq> &transMsgReq, const MsgData &from)
{
	NodeInfo *nodeinfo = transMsgReq->mutable_dest();
	std::string nodeid = nodeinfo->node_id();
	std::string public_nodeid = nodeinfo->public_node_id();
	std::string msg = transMsgReq->data();

	{
		// 统计转发数据
		std::string data = transMsgReq->data();
		CommonMsg common_msg;
		std::string tmp(data.begin() + 4, data.end() - sizeof(uint32_t) * 3);
		int r = common_msg.ParseFromString(tmp);
		if (!r)
		{
			return;
		}
		std::string type = common_msg.type();
		global::reqCntMap[type].first += 1;
		global::reqCntMap[type].second += common_msg.data().size();
	}

	//获取自身节点信息
	auto self_node = Singleton<PeerNode>::get_instance()->get_self_node();
	//将自身节点id与目标节点public_node_id进行比较
	if (self_node.id == public_nodeid)
	{
		Node dest;
		bool find = Singleton<PeerNode>::get_instance()->find_node(nodeid, dest);
		if (find)
		{
			transMsgReq->set_priority(transMsgReq->priority() & 0xE); // 转发数据不能达到最高优先级
			net_com::send_one_message(dest, std::move(transMsgReq->data()), transMsgReq->priority());
		}
		else
		{
			return;
		}
	}
	else
	{
		Node to;
		bool find = Singleton<PeerNode>::get_instance()->find_node(public_nodeid, to);
		if (find)
		{
			TransMsgReq trans_Msg_Req;
			NodeInfo *destnode = trans_Msg_Req.mutable_dest();
			destnode->set_node_id(nodeid);
			destnode->set_public_node_id(public_nodeid);
			trans_Msg_Req.set_data(msg);
			trans_Msg_Req.set_priority(transMsgReq->priority());
			net_com::send_message(to, trans_Msg_Req);
		}
		else
		{
			// 尝试根据id找到节点后再次转发
			Node targetNode;
			if (!Singleton<PeerNode>::get_instance()->find_node(nodeid, targetNode))
			{
				return;
			}

			if (targetNode.public_node_id.empty() && targetNode.is_public_node == false)
			{
				return;
			}

			if (targetNode.public_node_id == self_node.id || (self_node.is_public_node && targetNode.is_public_node))
			{
				// 直接发送
				transMsgReq->set_priority(transMsgReq->priority() & 0xE); // 转发数据不能达到最高优先级
				net_com::send_one_message(targetNode, std::move(transMsgReq->data()), transMsgReq->priority());
			}
			else
			{
				// 找到负责的公网然后发送
				Node targetPublicNode;
				if (!Singleton<PeerNode>::get_instance()->find_node(targetNode.public_node_id, targetPublicNode))
				{
					return;
				}

				TransMsgReq trans_Msg_Req;
				NodeInfo *destnode = trans_Msg_Req.mutable_dest();
				destnode->set_node_id(nodeid);
				destnode->set_public_node_id(targetNode.public_node_id);
				trans_Msg_Req.set_data(msg);
				trans_Msg_Req.set_priority(transMsgReq->priority());
				net_com::send_message(targetPublicNode, trans_Msg_Req);
			}
		}
	}
}

void handleBroadcastMsgReq(const std::shared_ptr<BroadcaseMsgReq> &broadcaseMsgReq, const MsgData &from)
{
	// std::cout << "handleBroadcaseMsgReq" << std::endl;
	// 向自身节点发送处理
	std::string data = broadcaseMsgReq->data();
	CommonMsg common_msg;
	int r = common_msg.ParseFromString(data);
	if (!r)
	{
		return;
	}
	MsgData toSelfMsgData = from;
	toSelfMsgData.pack.data = data;
	toSelfMsgData.pack.flag = broadcaseMsgReq->priority();
	Singleton<ProtobufDispatcher>::get_instance()->handle(toSelfMsgData);

	// broadcast impl
	BroadcaseMsgReq req = *broadcaseMsgReq;

	const Node &selfNode = Singleton<PeerNode>::get_instance()->get_self_node();
	if (req.from().is_public_node())
	{
		// 来源是公网，向所属子节点发送
		const std::vector<Node> subNodeList = Singleton<PeerNode>::get_instance()->get_sub_nodelist(selfNode.id);
		for (auto &item : subNodeList)
		{
			if (req.from().node_id() != item.id && item.is_public_node == false)
			{
				net_com::send_message(item, req);
			}
		}
	}
	else
	{
		// 来源是子网节点，根据需要向其他公网节点转发
		std::string originNodeId = req.mutable_from()->node_id(); // 记下原先的节点
		req.mutable_from()->set_is_public_node(selfNode.is_public_node);
		req.mutable_from()->set_node_id(selfNode.id);

		const std::vector<Node> publicNodeList = Singleton<PeerNode>::get_instance()->get_nodelist(NODE_PUBLIC);
		for (auto &item : publicNodeList)
		{
			if (req.from().node_id() != item.id)
			{
				net_com::send_message(item, req);
			}
		}

		/// 并且也要向其所属的子节点发送
		const std::vector<Node> subNodeList = Singleton<PeerNode>::get_instance()->get_sub_nodelist(selfNode.id);
		for (auto &item : subNodeList)
		{
			if (req.from().node_id() != item.id && originNodeId != item.id && item.is_public_node == false)
			{
				net_com::send_message(item, req);
			}
		}
	}
}

void handleNotifyConnectReq(const std::shared_ptr<NotifyConnectReq> &notifyConnectReq, const MsgData &from)
{
	NodeInfo *server_node = notifyConnectReq->mutable_server_node();
	NodeInfo *client_node = notifyConnectReq->mutable_client_node();

	if (client_node->node_id() == Singleton<PeerNode>::get_instance()->get_self_id())
	{
		Node node;
		node.id = server_node->node_id();
		node.local_ip = server_node->local_ip();
		node.local_port = server_node->local_port();
		node.public_ip = server_node->public_ip();
		node.public_port = server_node->public_port();
		node.mac_md5 = server_node->mac_md5();
		node.is_public_node = server_node->is_public_node();
		node.fee = server_node->fee();
		node.package_fee = server_node->package_fee();
		node.base58address = server_node->base58addr();
		node.chain_height = server_node->chain_height();
		node.public_node_id = server_node->public_node_id();

		Node tem_node;
		auto find = Singleton<PeerNode>::get_instance()->find_node(node.id, tem_node);
		if (!find)
		{
			Singleton<PeerNode>::get_instance()->add(node);
			if (node.is_public_node)
			{
				Singleton<PeerNode>::get_instance()->add_public_node(node);
			}
			Singleton<PeerNode>::get_instance()->conect_nodelist();
		}
		else
		{
			if (tem_node.fd < 0)
			{
				Singleton<PeerNode>::get_instance()->conect_nodelist();
			}
		}
	}
	else
	{
		Node node;
		bool find = Singleton<PeerNode>::get_instance()->find_node(client_node->node_id(), node);
		if (find && node.fd > 0)
		{
			NotifyConnectReq notifyConnectReq;

			NodeInfo *server_node1 = notifyConnectReq.mutable_server_node();
			server_node1->set_node_id(server_node->node_id());
			server_node1->set_local_ip(server_node->local_ip());
			server_node1->set_local_port(server_node->local_port());
			server_node1->set_public_ip(server_node->public_ip());
			server_node1->set_public_port(server_node->public_port());
			server_node1->set_is_public_node(server_node->is_public_node());
			server_node1->set_mac_md5(server_node->mac_md5());
			server_node1->set_fee(server_node->fee());
			server_node1->set_package_fee(server_node->package_fee());
			server_node1->set_base58addr(server_node->base58addr());
			server_node1->set_chain_height(server_node->chain_height());

			NodeInfo *client_node1 = notifyConnectReq.mutable_client_node();
			client_node1->set_node_id(client_node->node_id());

			CommonMsg msg;
			Pack::InitCommonMsg(msg, notifyConnectReq);

			net_pack pack;
			Pack::common_msg_to_pack(msg, 0, pack);
			net_com::send_one_message(node, pack);
		}
	}
}

void handlePingReq(const std::shared_ptr<PingReq> &pingReq, const MsgData &from)
{
	std::string id = pingReq->id();
	Node node;
	if (Singleton<PeerNode>::get_instance()->find_node(id, node))
	{
		node.ResetHeart();
		net_com::SendPongReq(node);
	}
}

void handlePongReq(const std::shared_ptr<PongReq> &pongReq, const MsgData &from)
{
	std::string id = pongReq->id();
	uint32 height = pongReq->chain_height();
	Node node;
	auto find = Singleton<PeerNode>::get_instance()->find_node(id, node);
	if (find)
	{
		node.ResetHeart();
		if (height > 0)
		{
			node.set_chain_height(height);
		}
		Singleton<PeerNode>::get_instance()->add_or_update(node);
	}
}

void handleSyncNodeReq(const std::shared_ptr<SyncNodeReq> &syncNodeReq, const MsgData &from)
{
	std::cout << "handleSyncNodeReq ======" << std::endl;
	auto id = syncNodeReq->ids(0);
	auto selfid = Singleton<PeerNode>::get_instance()->get_self_id();
	auto self_node = Singleton<PeerNode>::get_instance()->get_self_node();
	std::cout << "handleSyncNodeReq id:" << id << std::endl;
	//for(int i = 0; i < size; i++)
	//{
	//auto id = syncNodeReq->ids(i);
	// 	if(id != selfid)
	// 	{
	// 		ids.insert(std::move(id));
	// 		// std::cout << "id:" << syncNodeReq->ids(i) << std::endl;
	// 	}
	// }

	auto node_size = syncNodeReq->nodes_size();
	if (node_size > 0)
	{
		//删除public_node_id == ids的内网节点
		Singleton<PeerNode>::get_instance()->delete_node_by_public_node_id(id);
	}

	//将内网节点重新加入到自己k桶

	for (int i = 0; i < node_size; i++)
	{
		const NodeInfo &nodeinfo = syncNodeReq->nodes(i);
		Node node;
		node.id = nodeinfo.node_id();
		node.local_ip = nodeinfo.local_ip();
		node.local_port = nodeinfo.local_port();
		node.public_ip = nodeinfo.public_ip();
		node.public_port = nodeinfo.public_port();
		node.mac_md5 = nodeinfo.mac_md5();
		node.is_public_node = nodeinfo.is_public_node();
		node.fee = nodeinfo.fee();
		node.package_fee = nodeinfo.package_fee();
		node.base58address = nodeinfo.base58addr();
		node.chain_height = nodeinfo.chain_height();
		node.public_node_id = nodeinfo.public_node_id();
		if (self_node.id != node.id)
		{
			Singleton<PeerNode>::get_instance()->add(node);
			if (node.is_public_node)
			{
				Singleton<PeerNode>::get_instance()->add_public_node(node);
			}
		}
	}
	SyncNodeAck syncNodeAck;
	//获取连接自己的所有内网节点
	vector<Node> nodelist = Singleton<PeerNode>::get_instance()->get_sub_nodelist(selfid);
	if (nodelist.size() == 0)
	{
		return;
	}
	//将自己的id放入syncNodeAck->ids中
	syncNodeAck.add_ids(std::move(selfid));
	//将nodelist中的节点放入syncNodeAck->nodes中
	for (auto &node : nodelist)
	{
		if (node.is_public_node && node.fd < 0) //liuzg
		{
			continue;
		}

		if (g_testflag == 0 && node.is_public_node) //liuzg
		{
			u32 &localIp = node.local_ip;
			u32 &publicIp = node.public_ip;

			if (localIp != publicIp || IpPort::is_public_ip(localIp) == false)
			{
				continue;
			}
		}

		NodeInfo *nodeinfo = syncNodeAck.add_nodes();
		nodeinfo->set_node_id(node.id);
		nodeinfo->set_local_ip(node.local_ip);
		nodeinfo->set_local_port(node.local_port);
		nodeinfo->set_public_ip(node.public_ip);
		nodeinfo->set_public_port(node.public_port);
		nodeinfo->set_is_public_node(node.is_public_node);
		nodeinfo->set_mac_md5(std::move(node.mac_md5));
		nodeinfo->set_fee(node.fee);
		nodeinfo->set_package_fee(node.package_fee);
		nodeinfo->set_base58addr(node.base58address);
		nodeinfo->set_chain_height(node.chain_height);
		nodeinfo->set_public_node_id(node.public_node_id);
	}
	//公网节点有的,矿机节点没有的
	// std::unordered_set<std::string> my_ids;
	// for(auto& node:nodelist)
	// {
	// 	auto result = ids.find(node.id);
	// 	if(result == ids.end())
	// 	{
	// 		NodeInfo* nodeinfo = syncNodeAck.add_nodes();
	// 		nodeinfo->set_node_id(node.id);
	// 		nodeinfo->set_local_ip( node.local_ip);
	// 		nodeinfo->set_local_port( node.local_port);
	// 		nodeinfo->set_public_ip( node.public_ip);
	// 		nodeinfo->set_public_port( node.public_port);
	// 		nodeinfo->set_is_public_node(node.is_public_node);
	// 		nodeinfo->set_mac_md5(std::move(node.mac_md5));
	// 		nodeinfo->set_fee(node.fee);
	// 		nodeinfo->set_package_fee(node.package_fee);
	// 		nodeinfo->set_base58addr(node.base58address);
	// 		nodeinfo->set_chain_height(node.chain_height);
	// 		nodeinfo->set_public_node_id(node.public_node_id);
	// 	}
	// 	std::cout << "handleSyncNodeReq node.id:" << node.id << std::endl;
	// 	my_ids.insert(std::move(node.id));
	// }

	// 矿机节点有的，公网节点没有的
	// for(auto& id:ids)
	// {
	// 	auto result = my_ids.find(id);
	// 	if(result == my_ids.end())
	// 	{
	// 		std::cout << "handleSyncNodeReq result == my_ids.end() node.id:" << id << std::endl;
	// 		syncNodeAck.add_ids(std::move(id));
	// 	}
	// }
	net_com::send_message(from, syncNodeAck, net_com::Compress::kCompress_True, net_com::Encrypt::kEncrypt_False, net_com::Priority::kPriority_High_2);
}

void handleSyncNodeAck(const std::shared_ptr<SyncNodeAck> &syncNodeAck, const MsgData &from)
{
	std::cout << "handleSyncNodeAck ======" << std::endl;
	auto self_id = Singleton<PeerNode>::get_instance()->get_self_id();
	//从syncNodeAck中获取id数量
	//auto id_size = syncNodeAck->ids_size();
	//从syncNodeAck中获取id
	auto id = syncNodeAck->ids(0);
	std::cout << "handleSyncNodeAck id 661:" << id << std::endl;
	int nodeSize = syncNodeAck->nodes_size();
	if (nodeSize > 0)
	{
		//将自身节点public_node_id == id的节点删除
		Singleton<PeerNode>::get_instance()->delete_node_by_public_node_id(id);
	}

	//将syncNodeAck中的节点信息重新加入到k桶中
	for (int i = 0; i < syncNodeAck->nodes_size(); i++)
	{
		const NodeInfo &nodeinfo = syncNodeAck->nodes(i);
		if (nodeinfo.node_id().size() > 0)
		{
			Node node;
			node.id = nodeinfo.node_id();
			node.local_ip = nodeinfo.local_ip();
			node.local_port = nodeinfo.local_port();
			node.public_ip = nodeinfo.public_ip();
			node.public_port = nodeinfo.public_port();
			node.mac_md5 = nodeinfo.mac_md5();
			node.is_public_node = nodeinfo.is_public_node();
			node.fee = nodeinfo.fee();
			node.package_fee = nodeinfo.package_fee();
			node.base58address = nodeinfo.base58addr();
			node.chain_height = nodeinfo.chain_height();
			node.public_node_id = nodeinfo.public_node_id();
			/*
            if(g_testflag == 0 && node.is_public_node)   //liuzg
		    {
			    u32 & localIp = node.local_ip;
			    u32 & publicIp = node.public_ip;

			    if (localIp != publicIp || IpPort::is_public_ip(localIp) == false)
			    {
				    continue;
			    }
		    }		
              */
			//根据node.id查找节点
			Node temp_node;
			bool find_result = Singleton<PeerNode>::get_instance()->find_node(node.id, temp_node);
			//std::cout << "handleSyncNodeAck node.id:" << node.id << std::endl;

			// if(find_result && temp_node.fd < 0)
			// {
			// 	std::cout << "handleSyncNodeAck find_result && temp_node.fd < 0 node.id:" << node.id << std::endl;
			// 	Singleton<PeerNode>::get_instance()->delete_node(node.id);
			// }

			if (!find_result && (node.id != self_id))
			{
				{
					Singleton<PeerNode>::get_instance()->add(node);
				}
				if (node.is_public_node)
				{
					Singleton<PeerNode>::get_instance()->add_public_node(node);
				}
			}
		}
	}
	Singleton<PeerNode>::get_instance()->conect_nodelist();
}

void handleEchoReq(const std::shared_ptr<EchoReq> &echoReq, const MsgData &from)
{
	EchoAck echoAck;
	echoAck.set_id(Singleton<PeerNode>::get_instance()->get_self_id());
	net_com::send_message(echoReq->id(), echoAck, net_com::Compress::kCompress_False, net_com::Encrypt::kEncrypt_False, net_com::Priority::kPriority_Low_0);
}

void handleEchoAck(const std::shared_ptr<EchoAck> &echoAck, const MsgData &from)
{
	std::cout << "echo from id:" << echoAck->id() << endl;
}

void handleUpdateFeeReq(const std::shared_ptr<UpdateFeeReq> &updateFeeReq, const MsgData &from)
{
	Singleton<PeerNode>::get_instance()->update_fee_by_id(updateFeeReq->id(), updateFeeReq->fee());
}

void handleUpdatePackageFeeReq(const std::shared_ptr<UpdatePackageFeeReq> &updatePackageFeeReq, const MsgData &from)
{
	Singleton<PeerNode>::get_instance()->update_package_fee_by_id(updatePackageFeeReq->id(), updatePackageFeeReq->package_fee());
}

void handleGetHeightReq(const std::shared_ptr<GetHeightReq> &heightReq, const MsgData &from)
{
	vector<Node> nodelist = Singleton<PeerNode>::get_instance()->get_nodelist();

	std::random_device device;
	std::mt19937 engine(device());
	std::shuffle(nodelist.begin(), nodelist.end(), engine);

	GetHeightAck heightAck;
	static const int MAX_NODE_SIZE = 100;
	for (size_t i = 0; i < nodelist.size() && i < MAX_NODE_SIZE; i++)
	{
		NodeHeight *nodeHeight = heightAck.add_nodes_height();
		nodeHeight->set_id(nodelist[i].id);
		nodeHeight->set_height(nodelist[i].chain_height);
		nodeHeight->set_base58addr(nodelist[i].base58address);
		nodeHeight->set_fee(nodelist[i].fee);
	}

	// Is fetch the public node.
	if (heightReq->is_fetch_public())
	{
		vector<Node> publicnodes = Singleton<PeerNode>::get_instance()->get_public_node();
		for (auto &node : publicnodes)
		{
			NodeInfo *nodeinfo = heightAck.add_public_nodes();
			nodeinfo->set_node_id(node.id);
			nodeinfo->set_local_ip(node.local_ip);
			nodeinfo->set_local_port(node.local_port);
			nodeinfo->set_public_ip(node.public_ip);
			nodeinfo->set_public_port(node.public_port);
			nodeinfo->set_is_public_node(node.is_public_node);
			nodeinfo->set_mac_md5(node.mac_md5);
			nodeinfo->set_fee(node.fee);
			nodeinfo->set_package_fee(node.package_fee);
			nodeinfo->set_base58addr(node.base58address);
			nodeinfo->set_chain_height(node.chain_height);
			nodeinfo->set_public_node_id(node.public_node_id);
		}
	}

	net_com::send_message(heightReq->id(), heightAck);
}

void handleGetHeightAck(const std::shared_ptr<GetHeightAck> &heightAck, const MsgData &from)
{
	vector<Node> nodelist;

	for (int i = 0; i < heightAck->nodes_height_size(); i++)
	{
		const NodeHeight &nodeHeight = heightAck->nodes_height(i);

		Node node;
		bool find = Singleton<PeerNode>::get_instance()->find_node(nodeHeight.id(), node);
		if (find)
		{
			node.chain_height = nodeHeight.height();
			node.base58address = nodeHeight.base58addr();
			node.fee = nodeHeight.fee();
			nodelist.push_back(node);
		}
		else
		{
			node.id = nodeHeight.id();
			node.chain_height = nodeHeight.height();
			node.base58address = nodeHeight.base58addr();
			node.fee = nodeHeight.fee();
			nodelist.push_back(node);
		}
	}
	if (nodelist.empty())
	{
		std::cout << "In GetHeightAck, list is empty!" << std::endl;
		return;
	}

	std::cout << "Get height ack: " << nodelist.size() << std::endl;
	Singleton<NodeCache>::get_instance()->add(nodelist);

	// Add public node
	if (heightAck->public_nodes_size() > 0)
	{
		std::vector<Node> newNodeList;
		for (int i = 0; i < heightAck->public_nodes_size(); i++)
		{
			const NodeInfo &nodeinfo = heightAck->public_nodes(i);
			Node node;
			node.id = nodeinfo.node_id();
			node.local_ip = nodeinfo.local_ip();
			node.local_port = nodeinfo.local_port();
			node.public_ip = nodeinfo.public_ip();
			node.public_port = nodeinfo.public_port();
			node.mac_md5 = nodeinfo.mac_md5();
			node.is_public_node = nodeinfo.is_public_node();
			node.fee = nodeinfo.fee();
			node.package_fee = nodeinfo.package_fee();
			node.base58address = nodeinfo.base58addr();
			node.chain_height = nodeinfo.chain_height();
			node.public_node_id = nodeinfo.public_node_id();

			newNodeList.push_back(node);
		}

		const Node &selfNode = Singleton<PeerNode>::get_instance()->get_self_node();
		std::vector<Node> oldNodeList = Singleton<PeerNode>::get_instance()->get_public_node();

		// 添加自身所连接公网
		for (auto &node : oldNodeList)
		{
			if (selfNode.public_node_id == node.id)
			{
				newNodeList.push_back(node);
			}
		}

		// 清除原有公网节点
		for (auto &node : oldNodeList)
		{
			Singleton<PeerNode>::get_instance()->delete_public_node(node);
		}

		// 更新公网节点
		for (auto &node : newNodeList)
		{
			Singleton<PeerNode>::get_instance()->add_public_node(node);
		}
	}
}
void handleGetTransInfoReq(const std::shared_ptr<GetTransInfoReq> &transInfoReq, const MsgData &from)
{

	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction *txn = pRocksDb->TransactionInit();
	if (txn == NULL)
	{
		std::cout << "TransactionInit failed !" << std::endl;
		return;
	}

	ON_SCOPE_EXIT
	{
		pRocksDb->TransactionDelete(txn, false);
	};
	std::string hash = transInfoReq->txid();

	std::string strTxRaw;
	if (pRocksDb->GetTransactionByHash(txn, hash, strTxRaw) != 0)
	{
		std::cout << "GetTransactionByHash fail" << std::endl;
		return;
	}

	string blockhash;
	unsigned height;
	int stat;
	stat = pRocksDb->GetBlockHashByTransactionHash(txn, hash, blockhash);
	if (stat != 0)
	{
		std::cout << "GetBlockHashByTransactionHash fail" << std::endl;
		return;
	}
	stat = pRocksDb->GetBlockHeightByBlockHash(txn, blockhash, height);
	if (stat != 0)
	{
		std::cout << "GetBlockHeightByBlockHash fail" << std::endl;
		return;
	}
	GetTransInfoAck transInfoAck;
	transInfoAck.set_height(height);
	CTransaction * utxoTx = new CTransaction();
	utxoTx->ParseFromString(strTxRaw);
	transInfoAck.set_allocated_trans(utxoTx);

	net_com::send_message(transInfoReq->nodeid(), transInfoAck);
	std::cout << "handleGetTransInfoReq send_message" << std::endl;
}
void handleGetTransInfoAck(const std::shared_ptr<GetTransInfoAck> &transInfoAck, const MsgData &from)
{
	std::lock_guard<std::mutex> lock(global::g_mutex_transinfo);
	global::g_is_utxo_empty = false;
	global::g_trans_info = *transInfoAck;
}