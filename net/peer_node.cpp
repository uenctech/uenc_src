#include "./peer_node.h"
#include "../include/logging.h"
#include "./ip_port.h"
#include "./net_api.h"
#include <chrono>
#include "./epoll_mode.h"
#include <sys/time.h>
#include <bitset>
#include "./global.h"

std::string parse_kind(int kind)
{
	switch (kind)
	{
		case 0:
			return "no connect";		
		case 1:
			return "inner-inner derict";
		case 2:
			return "inner-outer derict";
		case 3:
			return "outer-inner derict";
		case 4:
			return "outer-outer derict";
		case 5:
			return "inner-inner holing";
		case 6:
			return "inner-inner hole";
		case 7:
			return "transpond";			
		case 8:
			return "passive accept";			
		default:
			return "xx";
	}
}

bool PeerNode::add(const Node& _node)
{
	std::lock_guard<std::mutex> lck(mutex_for_nodes_);
	if(_node.id.size() == 0)
	{
		return false;
	} 
	auto itr = node_map_.find(_node.id);
	if (itr != node_map_.end())
	{
		// error("the node is exist!!!! node_id : %s", _node.id.c_str());
		return false;
	}
	this->node_map_[_node.id] = _node;

	return true;
}

bool PeerNode::add_public_node(const Node & _node)
{
	std::lock_guard<std::mutex> lck(mutex_for_public_nodes_);
	if(_node.id.size() == 0)
	{
		return false;
	} 

	if (!_node.is_public_node)
	{
		return false;
	}

	auto itr = pub_node_map_.find(_node.id);
	if (itr != pub_node_map_.end())
	{
		return false;
	}
	this->pub_node_map_[_node.id] = _node;

	return true;
}

bool PeerNode::delete_public_node(const Node & _node)
{
	std::lock_guard<std::mutex> lck(mutex_for_public_nodes_);
	if(_node.id.size() == 0)
	{
		return false;
	} 

	if (!_node.is_public_node)
	{
		return false;
	}

	auto itr = pub_node_map_.find(_node.id);
	if (itr == pub_node_map_.end())
	{
		return false;
	}

	pub_node_map_.erase(itr);

	return true;
}

bool PeerNode::update(const Node & _node)
{
	{
		std::lock_guard<std::mutex> lck(mutex_for_nodes_);

		auto itr = this->node_map_.find(_node.id);
		if (itr == this->node_map_.end())
		{
			// error("node_id is not exist please add it node_id : %s", _node.id.c_str());
			return false;
		}
		this->node_map_[_node.id] = _node;
	}

	return true;
}

bool PeerNode::update_public_node(const Node & _node)
{
	std::lock_guard<std::mutex> lck(mutex_for_nodes_);

	if (!_node.is_public_node)
	{
		return false;
	}

	auto itr = this->pub_node_map_.find(_node.id);
	if (itr == this->pub_node_map_.end())
	{
		return false;
	}
	this->pub_node_map_[_node.id] = _node;

	return true;
}

bool PeerNode::add_or_update(Node _node)
{
	std::lock_guard<std::mutex> lck(mutex_for_nodes_);
	this->node_map_[_node.id] = _node;
	
	return true;
}

void PeerNode::delete_node(id_type _id)
{
	std::lock_guard<std::mutex> lck(mutex_for_nodes_);

	auto nodeIt = node_map_.find(_id);
	auto pubNodeIt = pub_node_map_.find(_id);
	
	if (nodeIt != node_map_.end())
	{
		int fd = nodeIt->second.fd;
		if(fd > 0)
		{
			Singleton<EpollMode>::get_instance()->delete_epoll_event(fd);
			close(fd);
		}	
		u32 ip = nodeIt->second.public_ip;
		u16 port = nodeIt->second.public_port;
		Singleton<BufferCrol>::get_instance()->delete_buffer(ip, port);				
		nodeIt = node_map_.erase(nodeIt);
	}
	
	if (pubNodeIt != pub_node_map_.end())
	{
		int fd = pubNodeIt->second.fd;
		if(fd > 0)
		{
			Singleton<EpollMode>::get_instance()->delete_epoll_event(fd);
			close(fd);
		}	
		u32 ip = pubNodeIt->second.public_ip;
		u16 port = pubNodeIt->second.public_port;
		Singleton<BufferCrol>::get_instance()->delete_buffer(ip, port);	
		pubNodeIt = pub_node_map_.erase(pubNodeIt);
	}
}

/**
 * @brief Delete nodes based on public_node_id 
 * 
 * @param public_node_id 
 */
void PeerNode::delete_node_by_public_node_id(id_type public_node_id)
{
	std::lock_guard<std::mutex> lck(mutex_for_nodes_);
	auto nodeIter = node_map_.begin();
	for(; nodeIter != node_map_.end(); nodeIter++)
	{
        if(nodeIter->second.public_node_id == public_node_id && !nodeIter->second.is_public_node)
		{
			nodeIter = node_map_.erase(nodeIter);
		}	
    }

}

void PeerNode::delete_by_fd(int fd)
{
	std::lock_guard<std::mutex> lck(mutex_for_nodes_);
	auto nodeIt = node_map_.begin();
	for(; nodeIt != node_map_.end(); ++nodeIt)
	{
        if(nodeIt->second.fd == fd)
		{
			break;
		}	
    }
	auto publicNodeIt = pub_node_map_.begin();
	for (; publicNodeIt != pub_node_map_.end(); ++ publicNodeIt)
	{
		if(publicNodeIt->second.fd == fd)
		{
			break;
		}
	}

	if (nodeIt != node_map_.end())
	{
		if(fd > 0)
		{
			Singleton<EpollMode>::get_instance()->delete_epoll_event(fd);
			close(fd);
		}	
		u32 ip = nodeIt->second.public_ip;
		u16 port = nodeIt->second.public_port;
		Singleton<BufferCrol>::get_instance()->delete_buffer(ip, port);				
		nodeIt = node_map_.erase(nodeIt);
	}
	
	if (publicNodeIt != pub_node_map_.end())
	{
		if(fd > 0)
		{
			Singleton<EpollMode>::get_instance()->delete_epoll_event(fd);
			close(fd);
		}	
		u32 ip = publicNodeIt->second.public_ip;
		u16 port = publicNodeIt->second.public_port;
		Singleton<BufferCrol>::get_instance()->delete_buffer(ip, port);	
		publicNodeIt = pub_node_map_.erase(publicNodeIt);
	}	
}

std::string PeerNode::nodeid2str(std::bitset<K_ID_LEN> id)
{
	std::string idstr = id.to_string();
	std::string ret;

	for (size_t i = 0; i < idstr.size() / 8; i++)
	{
		std::string tmp(idstr.begin() + i * 8, idstr.begin() + i * 8 + 8);
		unsigned int slice = std::stol(tmp, 0, 2);
		char buff[20] = {0};
		sprintf(buff, "%02x", slice);
		ret += buff;
	}
	return ret;
}

void PeerNode::print(std::vector<Node> & nodelist)
{
	std::cout << std::endl;
	std::cout << "------------------------------------------------------------------------------------------------------------" << std::endl;
	for (auto& i : nodelist)
	{
		i.print();
	}
	std::cout << "------------------------------------------------------------------------------------------------------------" << std::endl;
	std::cout << "PeerNode size is: " << nodelist.size() << std::endl;
}

void PeerNode::print(const Node & node)
{
	std::cout << "---------------------------------- node info --------------------------------------------------------------------------" << std::endl;
	
	std::cout << "id: " << node.id << std::endl;
	std::cout << "public_ip: " << IpPort::ipsz(node.public_ip) << std::endl;
	std::cout << "local_ip: " << IpPort::ipsz(node.local_ip) << std::endl;
	std::cout << "public_port: " << node.public_port << std::endl;
	std::cout << "local_port: " << node.local_port << std::endl;
	std::cout << "conn_kind: " << node.conn_kind << std::endl;
	std::cout << "fd: " << node.fd << std::endl;
	std::cout << "heart_time: " << node.heart_time << std::endl;
	std::cout << "heart_probes: " << node.heart_probes << std::endl;
	std::cout << "is_public_node: " << node.is_public_node << std::endl;
	std::cout << "mac_md5: " << node.mac_md5 << std::endl;
	std::cout << "fee: " << node.fee << std::endl;
	std::cout << "package_fee: " << node.package_fee << std::endl;
	std::cout << "base58address: " << node.base58address << std::endl;
	std::cout << "chain_height: " << node.chain_height << std::endl;
	std::cout << "public_node_id: " << node.public_node_id << std::endl;

	std::cout << "---------------------------------- end --------------------------------------------------------------------------" << std::endl;
}

std::string PeerNode::nodelist_info(std::vector<Node> & nodelist)
{
	std::ostringstream oss;	
	oss << std::endl;
	oss << "------------------------------------------------------------------------------------------------------------" << std::endl;
	for (auto& i : nodelist)
	{
		oss << i.info_str();
	}
	oss << "------------------------------------------------------------------------------------------------------------" << std::endl;
	oss << "PeerNode size is: " << nodelist.size() << std::endl;
	return oss.str();
}



bool PeerNode::find_node_by_fd(int fd, Node &node_)
{
	std::lock_guard<std::mutex> lck(mutex_for_nodes_);
	for (auto node : node_map_)
	{
		if (node.second.fd == fd)
		{
			node_ = node.second;
			return true;
		}
	}
	return false;
}


//check id exists
bool PeerNode::is_id_exists(id_type const &id)
{
	std::lock_guard<std::mutex> lck(mutex_for_nodes_);
	string str_id = id;
	auto it = node_map_.find(str_id);
	return it != node_map_.end();
}


// find node
bool PeerNode::find_node(id_type const &id, Node &x)
{
	std::lock_guard<std::mutex> lck(mutex_for_nodes_);

	string str_id = id;
	auto it = node_map_.find(str_id);
	if (it != node_map_.end())
	{
		x = it->second;
		return true;
	}
	return false;
}

std::vector<Node> PeerNode::get_public_node()
{
	std::lock_guard<std::mutex> lck(mutex_for_public_nodes_);
	vector<Node> rst;
	for (const auto & node : pub_node_map_)
	{
		rst.push_back(node.second);
	}

	return rst;
}


vector<Node> PeerNode::get_nodelist(NodeType type, bool mustAlive)
{
	std::lock_guard<std::mutex> lck(mutex_for_nodes_);
	vector<Node> rst;
	auto cb = node_map_.cbegin(), ce = node_map_.cend();
	for (; cb != ce; ++cb)
	{
		if(type == NODE_ALL || (type == NODE_PUBLIC && cb->second.is_public_node))
		{
			if(mustAlive)
			{
				if(cb->second.is_connected())
				{
					rst.push_back(cb->second);
				}
			}else{
				rst.push_back(cb->second);
			}
		}
	}
	return rst;
}

std::vector<Node> PeerNode::get_sub_nodelist(id_type const & id)
{
	std::lock_guard<std::mutex> lck(mutex_for_nodes_);

	std::vector<Node> ret;

	for (auto & item : node_map_)
	{
		if (item.second.public_node_id == id)
		{
			ret.push_back(item.second);
		}
	}

	return ret;
}


// Refresh thread 
extern atomic<int> nodelist_refresh_time;
bool PeerNode::nodelist_refresh_thread_init()
{
	refresh_thread = std::thread(std::bind(&PeerNode::nodelist_refresh_thread_fun, this));
	refresh_thread.detach();
	return true;
}

// Thread function 
void PeerNode::nodelist_refresh_thread_fun()
{
	std::lock_guard<std::mutex> lck(global::mutex_listen_thread);
	while(!global::listen_thread_inited)
	{
		// std::cout << "cond_listen_thread:wait" << std::endl;   
		global::cond_listen_thread.wait(global::mutex_listen_thread);
	}
	// std::cout << "cond_listen_thread:ok!!" << std::endl;   

	std::lock_guard<std::mutex> lck1(global::mutex_set_fee);
	while(global::fee_inited < 2)
	{
		// std::cout << "fee_inited:wait" << std::endl;   
		global::cond_fee_is_set.wait(global::mutex_set_fee);
	}

	//Initialize get node 
	net_com::InitRegisterNode();

	//Get nodes synchronously 
	vector<Node> nodelist;
	//Get own node information 
	bool is_public_node_value = Singleton<Config>::get_instance()->GetIsPublicNode();
	do
	{
		sleep(global::nodelist_refresh_time);

		nodelist = get_public_node();
		if (nodelist.empty())
		{
			std::cout << "nodelist_refresh_thread_fun continue ======" << std::endl;
			net_com::InitRegisterNode();
			continue;
		}

		std::random_shuffle(nodelist.begin(), nodelist.end());

		if (is_public_node_value)
		{
			// Itself is the public network 
			// Node & node = nodelist[0];
			// if (node.fd > 0)
			// {
			// 	net_com::SendSyncNodeReq(node);
			// }
			// else
			// {
			// 	net_com::SendRegisterNodeReq(node, true);
			// }
			for (auto & node : nodelist)
			{
				if (node.fd > 0)
				{
					net_com::SendSyncNodeReq(node);
				}
				else
				{
					net_com::SendRegisterNodeReq(node, true);
				}
			}
		}
		else
		{
			// Itself is a subnet node 
			Node node;
			for (auto & item : nodelist)
			{
				if (item.fd > 0)
				{
					node = item;
					break;
				}
			}

			if (node.id.empty())
			{
				// No connected node 
				node = nodelist[0];
				net_com::SendRegisterNodeReq(node, true);
			}
		}

	} while (true);
}

void PeerNode::conect_nodelist()
{
	vector<Node> nodelist = get_nodelist();
	for (auto &node : nodelist)
	{	
		if (node.fd == -1)
		{
			net_com::parse_conn_kind(node);
			uint32_t u32_ip;
			uint16_t port;
			if(node.conn_kind == DRTI2I)  //Direct connection 
			{
				u32_ip = node.local_ip;
				port = node.local_port;
				node.public_ip = u32_ip;
				node.public_port = port;
			}
			else if( node.conn_kind == DRTI2O) //Direct connection inside and outside 
			{
				u32_ip = node.public_ip;
				port = node.public_port;
			}
			else if( node.conn_kind == DRTO2I)  //Outside to inside 
			{
				net_com::SendNotifyConnectReq(node);
				continue;
			}
			else if( node.conn_kind == DRTO2O)  //Direct connection 
			{
				u32_ip = node.public_ip;
				port = node.public_port;
			}
			else if( node.conn_kind == BYSERV )  //Forward 
			{
				node.fd = -2;
				node.conn_kind = BYSERV;
				update(node);
				update_public_node(node);
				// net_com::SendConnectNodeReq(node);
				continue;   
			}

			//Connect only if it is directly connected internally 
			if(node.conn_kind == DRTI2I || node.conn_kind == DRTO2O)
			{
				//std::cout << "node.conn_kind :" << node.conn_kind << std::endl;
				int cfd = net_com::connect_init(u32_ip, port);
				if (cfd <= 0)
				{
					continue;
				}
				node.fd = cfd;
				Singleton<BufferCrol>::get_instance()->add_buffer(u32_ip, port, cfd);
				Singleton<EpollMode>::get_instance()->add_epoll_event(cfd, EPOLLIN | EPOLLET | EPOLLOUT);
				update(node);
				if(node.is_public_node)
				{
					update_public_node(node);
				}
				net_com::SendConnectNodeReq(node);
			}
		}	
	}
}

// Get ID
const id_type PeerNode::get_self_id()
{
	std::lock_guard<std::mutex> lck(mutex_for_curr_);
	return curr_node_.id;
}

// Set ID
void PeerNode::set_self_id(const id_type &id)
{
	std::lock_guard<std::mutex> lck(mutex_for_curr_);
	curr_node_.id = id;
}

// Set IP
void PeerNode::set_self_ip_p(const u32 public_ip)
{
	std::lock_guard<std::mutex> lck(mutex_for_curr_);
	curr_node_.public_ip = public_ip;
}

// Set IP
void PeerNode::set_self_ip_l(const u32 local_ip)
{
	std::lock_guard<std::mutex> lck(mutex_for_curr_);
	curr_node_.local_ip = local_ip;
}

// Set port 
void PeerNode::set_self_port_p(const u16 port_p)
{
	std::lock_guard<std::mutex> lck(mutex_for_curr_);
	curr_node_.public_port = port_p;
}

// Set port 
void PeerNode::set_self_port_l(const u16 port_l)
{
	std::lock_guard<std::mutex> lck(mutex_for_curr_);
	curr_node_.local_port = port_l;
}

void PeerNode::set_self_public_node(bool bl)
{
	std::lock_guard<std::mutex> lck(mutex_for_curr_);
	curr_node_.is_public_node = bl;
}

void PeerNode::set_self_fee(uint64_t fee)
{	
	std::lock_guard<std::mutex> lck(mutex_for_curr_);
	curr_node_.fee = fee;
}

void PeerNode::set_self_package_fee(uint64_t package_fee)
{	
	std::lock_guard<std::mutex> lck(mutex_for_curr_);
	curr_node_.package_fee = package_fee;
}

void PeerNode::set_self_mac_md5(string mac_md5)
{
	std::lock_guard<std::mutex> lck(mutex_for_curr_);
	curr_node_.mac_md5 = mac_md5;
}

void PeerNode::set_self_base58_address(string address)
{
	std::lock_guard<std::mutex> lck(mutex_for_curr_);
	curr_node_.base58address = address;
}

void PeerNode::set_self_public_node_id(string public_node_id)
{
	std::lock_guard<std::mutex> lck(mutex_for_curr_);
	curr_node_.public_node_id = public_node_id; 
}

void PeerNode::set_self_chain_height(u32 height)
{
	std::lock_guard<std::mutex> lck(mutex_for_curr_);
	curr_node_.set_chain_height(height);
}

void PeerNode::set_self_chain_height()
{
	if (net_callback::chain_height_callback)
	{
		u32 height = net_callback::chain_height_callback();
		if (height > 0)
		{
			set_self_chain_height(height);
		}
	}
	else
	{
		//std::cout << "Set self chain height: callback empty!!!" << std::endl;
	}
}

u32 PeerNode::get_self_chain_height_newest()
{
	// Update to newest chain height
	set_self_chain_height();
	// std::cout << "Get self chain height: " << curr_node_.chain_height << std::endl;
	
	return curr_node_.chain_height;
}

// Own node 
const Node PeerNode::get_self_node()
{
	std::lock_guard<std::mutex> lck(mutex_for_curr_);
	return curr_node_;
}

// Generate ID 
bool PeerNode::make_rand_id()
{
	std::bitset<K_ID_LEN> bit;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	static std::default_random_engine e(tv.tv_sec + tv.tv_usec + getpid());
	static std::uniform_int_distribution<unsigned> u(0, 1);

	std::lock_guard<std::mutex> lck(mutex_for_curr_);
	for (int i = 0; i < K_ID_LEN; i++)
	{
		bit[i] = u(e);
	}
	curr_node_.id = nodeid2str(bit);
	return true;
}

//Determine whether the string type id is legal 
bool PeerNode::is_id_valid(const string &id)
{
	if (id.size() != K_ID_LEN / 4)
	{
		return false;
	}
	return true;
}

bool PeerNode::update_fee_by_id(const string &id, uint64_t fee)
{
	std::lock_guard<std::mutex> lck(mutex_for_nodes_);

	auto itr = node_map_.find(id);
	if (itr == node_map_.end())
	{
		error("the node is not exist!!!! update_fee failed! node_id : %s", id.c_str());
		return false;
	}
	itr->second.fee = fee;

	return true;
}

bool PeerNode::update_package_fee_by_id(const string &id, uint64_t package_fee)
{
	std::lock_guard<std::mutex> lck(mutex_for_nodes_);

	auto itr = node_map_.find(id);
	if (itr == node_map_.end())
	{
		error("the node is not exist!!!! update_package_fee failed! node_id : %s", id.c_str());
		return false;
	}
	itr->second.package_fee = package_fee;
	return true;
}
