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

	auto itr = node_map_.find(_node.id);
	if (itr != node_map_.end())
	{
		error("the node is exist!!!! node_id : %s", _node.id.c_str());
		return false;
	}
	this->node_map_[_node.id] = _node;

	return true;
}

bool PeerNode::update(const Node & _node)
{
	{
		std::lock_guard<std::mutex> lck(mutex_for_nodes_);

		auto itr = this->node_map_.find(_node.id);
		if (itr == this->node_map_.end())
		{
			
			return false;
		}
		this->node_map_[_node.id] = _node;
	}

	return true;
}

bool PeerNode::add_or_update(Node _node)
{
	std::lock_guard<std::mutex> lck(mutex_for_nodes_);
	this->node_map_[_node.id] = _node;
	
	return true;
}

bool PeerNode::delete_node(id_type _id)
{
	std::lock_guard<std::mutex> lck(mutex_for_nodes_);

	auto it = this->node_map_.find(_id);
	if (it != this->node_map_.end())
	{
		int fd = it->second.fd;
		if(fd > 0)
		{
			Singleton<EpollMode>::get_instance()->delete_epoll_event(fd);
			close(fd);
		}

		u32 ip = it->second.public_ip;
		u16 port = it->second.public_port;
		Singleton<BufferCrol>::get_instance()->delete_buffer(ip, port);
		this->node_map_.erase(it);
		return true;
	}
	return false;
}

bool PeerNode::delete_by_fd(int fd)
{
	std::lock_guard<std::mutex> lck(mutex_for_nodes_);
	auto it = node_map_.begin();
    for(; it != node_map_.end(); ++it) {
        if(it->second.fd == fd)
		{
			if(fd > 0)
			{
				Singleton<EpollMode>::get_instance()->delete_epoll_event(fd);
				close(fd);
			}	
			u32 ip = it->second.public_ip;
			u16 port = it->second.public_port;
			Singleton<BufferCrol>::get_instance()->delete_buffer(ip, port);				
            it = node_map_.erase(it);
			break;
		}	
    }
	if (it == node_map_.end())
	{
		return false;
	}
	else
	{
		return true;
	}
	

}

void PeerNode::delete_by_fd_not_close(int fd)
{
	std::lock_guard<std::mutex> lck(mutex_for_nodes_);
    for(auto it = node_map_.begin(); it != node_map_.end(); ) {
        if(it->second.fd == fd)
		{			
            it = node_map_.erase(it);
			break;
		}
        else
            ++it;
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

void PeerNode::print(std::vector<Node> nodelist)
{
	std::cout << std::endl;
	std::cout << "------------------------------------------------------------------------------------------------------------" << std::endl;
	for (auto i : nodelist)
	{
		i.print();
	}
	std::cout << "------------------------------------------------------------------------------------------------------------" << std::endl;
	std::cout << "PeerNode size is: " << nodelist.size() << std::endl;
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



bool PeerNode::is_id_exists(id_type const &id)
{
	std::lock_guard<std::mutex> lck(mutex_for_nodes_);
	string str_id = id;
	auto it = node_map_.find(str_id);
	return it != node_map_.end();
}



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


vector<Node> PeerNode::get_nodelist(NodeType type)
{
	std::lock_guard<std::mutex> lck(mutex_for_nodes_);
	vector<Node> rst;
	auto cb = node_map_.cbegin(), ce = node_map_.cend();
	for (; cb != ce; ++cb)
	{
		if(type == NODE_ALL || (type == NODE_PUBLIC && cb->second.is_public_node))
		{
			rst.push_back(cb->second);
		}
	}
	return rst;
}



extern atomic<int> nodelist_refresh_time;
bool PeerNode::nodelist_refresh_thread_init()
{
	refresh_thread = std::thread(std::bind(&PeerNode::nodelist_refresh_thread_fun, this));
	refresh_thread.detach();
	return true;
}


void PeerNode::nodelist_refresh_thread_fun()
{
	std::lock_guard<std::mutex> lck(global::mutex_listen_thread);
	while(!global::listen_thread_inited)
	{
		
		global::cond_listen_thread.wait(global::mutex_listen_thread);
	}
	

	std::lock_guard<std::mutex> lck1(global::mutex_set_fee);
	while(global::fee_inited < 2)
	{
		
		global::cond_fee_is_set.wait(global::mutex_set_fee);
	}

	
	net_com::InitRegisterNode();

	
	vector<Node> nodelist;
	do
	{
		sleep(global::nodelist_refresh_time);
		nodelist = PeerNode::get_nodelist(NODE_PUBLIC);
		if (nodelist.size() > 0)
		{
			std::random_shuffle(nodelist.begin(), nodelist.end());
			bool res = net_com::SendSyncNodeReq(nodelist[0]);
			if(!res)
			{
				net_com::InitRegisterNode();
			}
		}else
		{	
			net_com::InitRegisterNode();
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
	
			if(node.conn_kind == DRTI2I)  
			{
				u32_ip = node.local_ip;
				port = node.local_port;
				node.public_ip = u32_ip;
				node.public_port = port;
			}else if( node.conn_kind == DRTI2O) 
			{
				u32_ip = node.public_ip;
				port = node.public_port;
			}
			else if( node.conn_kind == DRTO2I)  
			{
				net_com::SendNotifyConnectReq(node);
				continue;
			}
			else if( node.conn_kind == DRTO2O)  
			{
				u32_ip = node.public_ip;
				port = node.public_port;
			}
			else if( node.conn_kind == BYSERV )  
			{
				node.fd = -2;
				node.conn_kind = BYSERV;
				update(node);
				
				continue;   
			}

			int cfd = net_com::connect_init(u32_ip, port);
			if (cfd <= 0)
			{
				continue;
			}
			node.fd = cfd;
			Singleton<BufferCrol>::get_instance()->add_buffer(u32_ip, port, cfd);
			Singleton<EpollMode>::get_instance()->add_epoll_event(cfd, EPOLLIN | EPOLLET | EPOLLOUT);
			update(node);
			net_com::SendConnectNodeReq(node);
		}	
	}
}


const id_type PeerNode::get_self_id()
{
	std::lock_guard<std::mutex> lck(mutex_for_curr_);
	return curr_node_.id;
}


void PeerNode::set_self_id(const id_type &id)
{
	std::lock_guard<std::mutex> lck(mutex_for_curr_);
	curr_node_.id = id;
}


void PeerNode::set_self_ip_p(const u32 public_ip)
{
	std::lock_guard<std::mutex> lck(mutex_for_curr_);
	curr_node_.public_ip = public_ip;
}


void PeerNode::set_self_ip_l(const u32 local_ip)
{
	std::lock_guard<std::mutex> lck(mutex_for_curr_);
	curr_node_.local_ip = local_ip;
}


void PeerNode::set_self_port_p(const u16 port_p)
{
	std::lock_guard<std::mutex> lck(mutex_for_curr_);
	curr_node_.public_port = port_p;
}


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


const Node PeerNode::get_self_node()
{
	std::lock_guard<std::mutex> lck(mutex_for_curr_);
	return curr_node_;
}


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
