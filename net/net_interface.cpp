#include <string.h>
#include "../include/net_interface.h"
#include "../include/logging.h"
#include "../utils/singleton.h"
#include "./ip_port.h"

#include "./peer_node.h"
#include "./pack.h"
#include "./net_api.h"
#include "node_cache.h"

#define CA_SEND 1
#define CA_BROADCAST 1

//获取自己节点ID 成功返回0
std::string net_get_self_node_id()
{
	return Singleton<PeerNode>::get_instance()->get_self_id();
}

Node net_get_self_node() 
{
    return Singleton<PeerNode>::get_instance()->get_self_node();
}

//设置矿费
void net_set_self_fee(uint64_t fee)
{	
	Singleton<PeerNode>::get_instance()->set_self_fee(fee);
	global::fee_inited++;
	if(global::fee_inited >= 2)
	{
		global::cond_fee_is_set.notify_all();
		std::cout << "cond_fee_is_set:notify_all" << std::endl;
	}
    
}

//设置自己的base58地址
void net_set_self_base58_address(string address)
{
	Singleton<PeerNode>::get_instance()->set_self_base58_address(address);
}

//更新矿费并进行全网广播
void net_update_fee_and_broadcast(uint64_t fee)
{
	Singleton<PeerNode>::get_instance()->set_self_fee(fee);
	UpdateFeeReq updateFeeReq;
	updateFeeReq.set_id(Singleton<PeerNode>::get_instance()->get_self_id());
	updateFeeReq.set_fee(fee);
	net_com::broadcast_message(updateFeeReq, net_com::Compress::kCompress_False, net_com::Encrypt::kEncrypt_False, net_com::Priority::kPriority_Low_0);
}

//设置打包费
void net_set_self_package_fee(uint64_t package_fee)
{	
	Singleton<PeerNode>::get_instance()->set_self_package_fee(package_fee);
	global::fee_inited++;
    if(global::fee_inited >= 2)
	{
		global::cond_fee_is_set.notify_all();
   		std::cout << "cond_package_fee_is_set:notify_all" << std::endl;

	}
    
}

//更新打包费并进行全网广播
void net_update_package_fee_and_broadcast(uint64_t package_fee)
{
	Singleton<PeerNode>::get_instance()->set_self_package_fee(package_fee);
	UpdatePackageFeeReq updatePackageFeeReq;
	updatePackageFeeReq.set_id(Singleton<PeerNode>::get_instance()->get_self_id());
	updatePackageFeeReq.set_package_fee(package_fee);
	net_com::broadcast_message(updatePackageFeeReq, net_com::Compress::kCompress_False, net_com::Encrypt::kEncrypt_False, net_com::Priority::kPriority_Low_0);
}

//返回K桶所有ID
std::vector<std::string> net_get_node_ids()
{
	std::vector<std::string> ids;
	auto nodelist = Singleton<PeerNode>::get_instance()->get_nodelist();
	for(auto& node:nodelist)
	{
		ids.push_back(node.id);
	}
	return ids;
}

double net_get_connected_percent()
{
	auto nodelist = Singleton<PeerNode>::get_instance()->get_nodelist();
	int num = 0;
	for(auto& node:nodelist)
	{
		if(node.fd > 0 || node.fd == -2)
		{
			num++;
		}	
	}
	int total_num = nodelist.size();
	return total_num == 0 ? 0 : num/total_num;
}

//返回公网节点
std::vector<Node> net_get_public_node()
{
	std::vector<Node> vnodes;
	auto nodelist = Singleton<PeerNode>::get_instance()->get_nodelist(NODE_PUBLIC);
	for(auto& node: nodelist)
	{
		if(node.fd > 0)
		{
			vnodes.push_back(node);
		}
	}
	return vnodes;
}

// 返回所有公网节点，不管是否连接
std::vector<Node> net_get_all_public_node()
{
	std::vector<Node> vnodes = Singleton<PeerNode>::get_instance()->get_nodelist(NODE_PUBLIC, false);
	return vnodes;
}

//返回所有节点ID和fee
std::map<std::string, uint64_t> net_get_node_ids_and_fees()
{
	std::vector<Node> nodelist;
	if (Singleton<PeerNode>::get_instance()->get_self_node().is_public_node)
	{
		nodelist = Singleton<PeerNode>::get_instance()->get_nodelist();
	}
	else
	{
		nodelist = Singleton<NodeCache>::get_instance()->get_nodelist();
	}
	
	std::map<std::string, uint64_t> res;
	for(auto& node:nodelist)
	{
		res[node.id] = node.fee;
	}
	return res;
}

//返回所有节点ID和base58address
std::map<std::string, string> net_get_node_ids_and_base58address()
{
	std::vector<Node> nodelist;
	if (Singleton<PeerNode>::get_instance()->get_self_node().is_public_node)
	{
		nodelist = Singleton<PeerNode>::get_instance()->get_nodelist();
	}
	else
	{
		nodelist = Singleton<NodeCache>::get_instance()->get_nodelist();
	}

	std::map<std::string, string> res;
	for(auto& node:nodelist)
	{
		res[node.id] = node.base58address;
	}
	return res;
}

//通过ip查找ID
std::string net_get_ID_by_ip(std::string ip)
{
	auto nodelist = Singleton<PeerNode>::get_instance()->get_nodelist(NODE_PUBLIC);
	for(auto& node:nodelist)
	{	
		
		if(std::string(IpPort::ipsz(node.public_ip)) == ip)
		{
			return node.id;	
		}
	}

	return std::string();
}


