#include <fstream>
#include "handle_event.h"
#include "../include/logging.h"
#include "./pack.h"
#include "./ip_port.h"
#include "peer_node.h"
#include "../include/net_interface.h"
#include "./global.h"
#include "net.pb.h"
#include "common.pb.h"
#include "dispatcher.h"
#include "../common/config.h"
#include "socket_buf.h"
#include <unordered_set>
#include <utility>

static int PrintMsNum = 0;
void handlePrintMsgReq(const std::shared_ptr<PrintMsgReq>& printMsgReq, const MsgData& from)
{
	int type = printMsgReq->type();
	if(type == 0)
	{
		std::cout 	<< ++PrintMsNum << " times data:" << printMsgReq->data() << std::endl; 
	}else{
		ofstream file("bigdata.txt", fstream::out);
		file <<printMsgReq->data();
		file.close();
		cout << "write bigdata.txt success!!!" << endl;
	}
}

void handleRegisterNodeReq(const std::shared_ptr<RegisterNodeReq>& registerNode, const MsgData& from)
{	
    NodeInfo * nodeinfo = registerNode->mutable_mynode();

	if (!Singleton<Config>::get_instance()->GetIsPublicNode())
	{
		return;
	}
	std::string destid = nodeinfo->node_id();
	Node node;
	node.fd             = from.fd;
	node.id             = destid;
	node.local_ip       = nodeinfo->local_ip();
	node.local_port     = nodeinfo->local_port();
	node.public_ip      = from.ip;
	node.public_port    = from.port;	
	node.mac_md5        = nodeinfo->mac_md5();
	node.is_public_node = nodeinfo->is_public_node();
	node.fee			= nodeinfo->fee();
	node.package_fee	= nodeinfo->package_fee();
	node.base58address  = nodeinfo->base58addr();
	Node tem_node;
	auto find = Singleton<PeerNode>::get_instance()->find_node(node.id, tem_node);
	if((find && tem_node.conn_kind == NOTYET) || !find){
		node.conn_kind      = PASSIV;
	}
	else
	{
		node.conn_kind 		= tem_node.conn_kind;
	}
	

	if (find)
	{
		if(tem_node.fd != from.fd )
		{
			close(tem_node.fd);
			Singleton<BufferCrol>::get_instance()->delete_buffer(tem_node.public_ip, tem_node.public_port);
			Singleton<BufferCrol>::get_instance()->add_buffer(from.fd, from.port, from.fd);
		}
		Singleton<PeerNode>::get_instance()->update(node);
	}
	else
	{
		Singleton<PeerNode>::get_instance()->add(node);
	}

	
	RegisterNodeAck registerNodeAck;
	std::vector<Node> nodelist;
	nodelist.push_back(Singleton<PeerNode>::get_instance()->get_self_node());
	bool is_get_nodelist = registerNode->is_get_nodelist();
	if(is_get_nodelist)
	{
		std::vector<Node> tmp	= Singleton<PeerNode>::get_instance()->get_nodelist();
		nodelist.insert(nodelist.end(),tmp.begin(),tmp.end());
	}
	for(auto &node:nodelist)
	{
		NodeInfo* nodeinfo = registerNodeAck.add_nodes();
		nodeinfo->set_node_id(node.id);
		nodeinfo->set_local_ip( node.local_ip);
		nodeinfo->set_local_port( node.local_port);
		nodeinfo->set_public_ip( node.public_ip);
		nodeinfo->set_public_port( node.public_port);			
		nodeinfo->set_is_public_node(node.is_public_node);
		nodeinfo->set_mac_md5(node.mac_md5);
		nodeinfo->set_fee(node.fee);
		nodeinfo->set_package_fee(node.package_fee);
		nodeinfo->set_base58addr(node.base58address);
	}

	net_com::send_message(destid, registerNodeAck, true);
}

void handleRegisterNodeAck(const std::shared_ptr<RegisterNodeAck>& registerNodeAck, const MsgData& from)
{	
	for (int i = 0; i < registerNodeAck->nodes_size(); i++) {
		const NodeInfo& nodeinfo = registerNodeAck->nodes(i);
		Node node;
		node.id             = nodeinfo.node_id();
		node.local_ip       = nodeinfo.local_ip();
		node.local_port     = nodeinfo.local_port();
		node.public_ip      = nodeinfo.public_ip();
		node.public_port    = nodeinfo.public_port();	
		node.mac_md5        = nodeinfo.mac_md5();
		node.is_public_node = nodeinfo.is_public_node();
		node.fee			= nodeinfo.fee();
		node.package_fee	= nodeinfo.package_fee();
		node.base58address  = nodeinfo.base58addr();
		
		if(from.ip == node.public_ip && from.port == node.public_port)
		{	
			node.fd = from.fd;
			net_com::parse_conn_kind(node);
			Singleton<PeerNode>::get_instance()->update(node);
			Node tmp;
			bool find = Singleton<PeerNode>::get_instance()->find_node_by_fd(node.fd, tmp);
			{
				if(find && tmp.id == "")
				{
					Singleton<PeerNode>::get_instance()->delete_by_fd_not_close(node.fd);
				}
			}
		}
		if(node.id != Singleton<PeerNode>::get_instance()->get_self_id() && node.id.size() > 0)
		{
			Singleton<PeerNode>::get_instance()->add(node);
		}
		else if (node.id == Singleton<PeerNode>::get_instance()->get_self_id() && !Singleton<Config>::get_instance()->GetIsPublicNode())
		{
			Singleton<PeerNode>::get_instance()->set_self_ip_p(node.public_ip);
			Singleton<PeerNode>::get_instance()->set_self_port_p(node.public_port);
		}
		
	}
	Singleton<PeerNode>::get_instance()->conect_nodelist();
}

void handleConnectNodeReq(const std::shared_ptr<ConnectNodeReq>& connectNodeReq, const MsgData& from)
{
    NodeInfo * nodeinfo = connectNodeReq->mutable_mynode();

	Node node;
	node.fd             = from.fd;
	node.id             = nodeinfo->node_id();
	node.local_ip       = nodeinfo->local_ip();
	node.local_port     = nodeinfo->local_port();
	node.mac_md5        = nodeinfo->mac_md5();
	node.is_public_node = nodeinfo->is_public_node();
	node.fee			= nodeinfo->fee();
	node.package_fee	= nodeinfo->package_fee();
	node.base58address  = nodeinfo->base58addr();
	if(nodeinfo->conn_kind() != BYSERV)
	{
		node.conn_kind      = PASSIV;
		node.public_ip      = from.ip;
		node.public_port    = from.port;			
	}else
	{
		node.conn_kind      = BYSERV;
		node.fd = -2;
		
		
	}
	
	Node tem_node;
	auto find = Singleton<PeerNode>::get_instance()->find_node(node.id, tem_node);
	if (find)
	{
		if(tem_node.fd != from.fd && node.conn_kind != BYSERV)
		{
			close(tem_node.fd);
			Singleton<BufferCrol>::get_instance()->delete_buffer(tem_node.public_ip, tem_node.public_port);
			Singleton<BufferCrol>::get_instance()->add_buffer(from.fd, from.port, from.fd);
		}
		else if(node.conn_kind == BYSERV && tem_node.fd > 0)
		{
			return;
		}
		Singleton<PeerNode>::get_instance()->update(node);
	}
	else
	{
		Singleton<PeerNode>::get_instance()->add(node);
	}
}

void handleTransMsgReq(const std::shared_ptr<TransMsgReq>& transMsgReq, const MsgData& from)
{
    NodeInfo * nodeinfo = transMsgReq->mutable_dest();
	std::string nodeid = nodeinfo->node_id();

	/*
	std::string data = transMsgReq->data();
	CommonMsg common_msg;
	std::string tmp(data.begin() + 4, data.end() - 8);
	int r = common_msg.ParseFromString(tmp);
	if (!r) {
		std::cout << "parse CommonMsg error111" << std::endl;;
		return;
	}
	std::string type = common_msg.type();
	info("handleTransMsgReq--sub type:%s", type.c_str());
	*/

	Node dest;
	bool find = Singleton<PeerNode>::get_instance()->find_node(nodeid,dest);
	if(find)
	{
		net_com::send_one_message(dest, std::move(transMsgReq->data()));	
	}
	
	
	
}


void handleNotifyConnectReq(const std::shared_ptr<NotifyConnectReq>& notifyConnectReq, const MsgData& from)
{
	NodeInfo * server_node = notifyConnectReq->mutable_server_node(); 
	NodeInfo * client_node = notifyConnectReq->mutable_client_node();

	if(client_node->node_id() == Singleton<PeerNode>::get_instance()->get_self_id())
	{
		Node node;
		node.id             = server_node->node_id();
		node.local_ip       = server_node->local_ip();
		node.local_port     = server_node->local_port();
		node.public_ip      = server_node->public_ip();
		node.public_port    = server_node->public_port();	
		node.mac_md5        = server_node->mac_md5();
		node.is_public_node = server_node->is_public_node();
		node.fee			= server_node->fee();
		node.package_fee	= server_node->package_fee();
		node.base58address  = server_node->base58addr();

		Node tem_node;
		auto find = Singleton<PeerNode>::get_instance()->find_node(node.id, tem_node);
		if (!find)
		{
			Singleton<PeerNode>::get_instance()->add(node);
			Singleton<PeerNode>::get_instance()->conect_nodelist();
		}
		else
		{
			if(tem_node.fd < 0)
			{	
				Singleton<PeerNode>::get_instance()->conect_nodelist();
			}
		}
	}else{  
		Node node;
		bool find = Singleton<PeerNode>::get_instance()->find_node(client_node->node_id() ,node);
		if(find && node.fd > 0)
		{
			NotifyConnectReq notifyConnectReq;

			NodeInfo* server_node1 = notifyConnectReq.mutable_server_node();
			server_node1->set_node_id(server_node->node_id());
			server_node1->set_local_ip( server_node->local_ip());
			server_node1->set_local_port( server_node->local_port());
			server_node1->set_public_ip( server_node->public_ip());
			server_node1->set_public_port( server_node->public_port());	
			server_node1->set_is_public_node(server_node->is_public_node());
			server_node1->set_mac_md5(server_node->mac_md5());
			server_node1->set_fee(server_node->fee());
			server_node1->set_package_fee(server_node->package_fee());
			server_node1->set_base58addr(server_node->base58addr());

			NodeInfo* client_node1 = notifyConnectReq.mutable_client_node();
			client_node1->set_node_id(client_node->node_id());

			CommonMsg msg;
			Pack::InitCommonMsg(msg, notifyConnectReq);

			net_pack pack;
			Pack::common_msg_to_pack(msg, pack);
			net_com::send_one_message(node, pack);
		}
	}
}


void handlePingReq(const std::shared_ptr<PingReq>& pingReq, const MsgData& from)
{
	std::string id = pingReq->id();
	Node node;
	auto find = Singleton<PeerNode>::get_instance()->find_node(id, node);
	if(find)
	{
		node.ResetHeart();
		Singleton<PeerNode>::get_instance()->update(node);
		net_com::SendPongReq(node);
	}
}


void handlePongReq(const std::shared_ptr<PongReq>& pongReq, const MsgData& from)
{
	std::string id = pongReq->id();
	Node node;
	auto find = Singleton<PeerNode>::get_instance()->find_node(id, node);
	if(find)
	{
		node.ResetHeart();
		Singleton<PeerNode>::get_instance()->update(node);
	}
}

void handleSyncNodeReq(const std::shared_ptr<SyncNodeReq>& syncNodeReq, const MsgData& from)
{
	
	std::unordered_set<std::string> ids;
	auto size = syncNodeReq->ids_size();
	auto selfid = Singleton<PeerNode>::get_instance()->get_self_id();
	for(int i = 0; i < size; i++)
	{
		auto id = syncNodeReq->ids(i);
		if(id != selfid)
		{
			ids.insert(std::move(id));
			
		}
	}

	SyncNodeAck syncNodeAck;
	vector<Node> nodelist = Singleton<PeerNode>::get_instance()->get_nodelist();

	
	std::unordered_set<std::string> my_ids;
	for(auto& node:nodelist)
	{
		auto result = ids.find(node.id); 
		if(result == ids.end())
		{
			NodeInfo* nodeinfo = syncNodeAck.add_nodes();
			nodeinfo->set_node_id(node.id);
			nodeinfo->set_local_ip( node.local_ip);
			nodeinfo->set_local_port( node.local_port);
			nodeinfo->set_public_ip( node.public_ip);
			nodeinfo->set_public_port( node.public_port);			
			nodeinfo->set_is_public_node(node.is_public_node);
			nodeinfo->set_mac_md5(std::move(node.mac_md5));
			nodeinfo->set_fee(node.fee);
			nodeinfo->set_package_fee(node.package_fee);	
			nodeinfo->set_base58addr(node.base58address);		
		}
		my_ids.insert(std::move(node.id));
	}

	
	for(auto& id:ids)
	{
		auto result = my_ids.find(id); 
		if(result == my_ids.end())
		{
			syncNodeAck.add_ids(std::move(id));
		}
	}

	net_com::send_message(from, syncNodeAck);
}



void handleSyncNodeAck(const std::shared_ptr<SyncNodeAck>& syncNodeAck, const MsgData& from)
{
	
	if(syncNodeAck->ids_size() < 100)
	{
		for(int i = 0; i < syncNodeAck->ids_size(); i++)
		{
			Singleton<PeerNode>::get_instance()->delete_node(syncNodeAck->ids(i));
		}	
	}

	for (int i = 0; i < syncNodeAck->nodes_size(); i++) {
		const NodeInfo& nodeinfo = syncNodeAck->nodes(i);
		if(nodeinfo.node_id().size() > 0)
		{
			Node node;
			node.id             = nodeinfo.node_id();
			node.local_ip       = nodeinfo.local_ip();
			node.local_port     = nodeinfo.local_port();
			node.public_ip      = nodeinfo.public_ip();
			node.public_port    = nodeinfo.public_port();	
			node.mac_md5        = nodeinfo.mac_md5();
			node.is_public_node = nodeinfo.is_public_node();
			node.fee			= nodeinfo.fee();
			node.package_fee	= nodeinfo.package_fee();
			node.base58address  = nodeinfo.base58addr();

			if(node.id != Singleton<PeerNode>::get_instance()->get_self_id())
			{				
				Singleton<PeerNode>::get_instance()->add(node);
			}
		}
	}
	Singleton<PeerNode>::get_instance()->conect_nodelist();
}


void handleEchoReq(const std::shared_ptr<EchoReq>& echoReq, const MsgData& from)
{
	EchoAck echoAck;
	echoAck.set_id(Singleton<PeerNode>::get_instance()->get_self_id());
	net_com::send_message(echoReq->id(), echoAck);	
}


void handleEchoAck(const std::shared_ptr<EchoAck>& echoAck, const MsgData& from)
{
	std::cout <<"echo from id:" << echoAck->id() << endl;
}

void handleUpdateFeeReq(const std::shared_ptr<UpdateFeeReq>& updateFeeReq, const MsgData& from)
{
	Singleton<PeerNode>::get_instance()->update_fee_by_id(updateFeeReq->id(), updateFeeReq->fee());
}

void handleUpdatePackageFeeReq(const std::shared_ptr<UpdatePackageFeeReq>& updatePackageFeeReq, const MsgData& from)
{
	Singleton<PeerNode>::get_instance()->update_package_fee_by_id(updatePackageFeeReq->id(), updatePackageFeeReq->package_fee());
}