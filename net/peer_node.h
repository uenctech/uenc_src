#ifndef _PEER_NODE_H_
#define _PEER_NODE_H_

#include <map>
#include <list>
#include <mutex>
#include <vector>
#include <string>
#include <thread>
#include <vector>
#include <bitset>
#include <iostream>
#include "./define.h"
#include "./ip_port.h"

using namespace std;

enum NodeType {NODE_ALL, NODE_PUBLIC};



enum ConnKind
{
	NOTYET,		//尚未连接
	DRTI2I,		//内内直连
	DRTI2O,		//内外直连
	DRTO2I,		//外内直连
	DRTO2O,		//外外直连
	HOLING,		//内内打洞
	BYHOLE,		//内内打洞
	BYSERV,		//服务中转
	PASSIV		//被动接受连接
};



std::string parse_kind(int kind);

using id_type = std::string;

class Node
{
public:	
	id_type					id			= "";
	u32						public_ip	= 0;
	u32						local_ip	= 0; 
	u16						public_port	= 0;
	u16						local_port	= 0;
	ConnKind				conn_kind	= NOTYET;
	int						fd			= -1;
	int                     heart_time  = HEART_TIME;
	int 					heart_probes = HEART_PROBES;
	bool					is_public_node = false;
	std::string             mac_md5     = "";
	uint64_t				fee			= 0;
	uint64_t				package_fee	= 0;
	std::string			    base58address = "";
	u32						chain_height = 0;
	id_type                 public_node_id = "";

	Node(){

	}
	Node(std::string node_id){
		id = node_id;
	}

	bool operator ==(const Node & obj) const
	{
	    return id == obj.id;
	}

	bool operator > (const Node &obj)//重载>操作符
	{
		return (*this).id > obj.id;
	}


	void ResetHeart()
	{
		heart_time  = HEART_TIME;
		heart_probes = HEART_PROBES;	
	}
	void print()
	{
		std::cout << info_str() << std::endl;
	}

	std::string info_str()
	{
		std::ostringstream oss;	
		oss
			<< "  ip(" << string(IpPort::ipsz(public_ip)) << ")"
			<< "  port(" << public_port << ")"
			<< "  ip_l(" << string(IpPort::ipsz(local_ip)) << ")"
			<< "  port_l(" << local_port << ")"
			<< "  kind(" << parse_kind(conn_kind) << ")"
			<< "  fd(" << fd << ")" 
			<< "  miner_fee(" << fee << ")"
			<< "  package_fee(" << package_fee << ")"
			<< "  base58(" << base58address << ")"
			<< "  heart_probes(" << heart_probes << ")"
			<< "  is_public(" << is_public_node << ")"
			<< "  id(" << id << ")"
			<< "  md5(" << mac_md5 << ")"
			<< "  height( " << chain_height << " )"
			<< "  public_node_id(" << public_node_id << ")"
			
			<< std::endl;
		return oss.str();		
	}
	bool is_connected() const
	{
		return fd > 0 || fd == -2;
	}
	void set_chain_height(u32 height)
	{
		// std::cout << "Set chain height: " << height << std::endl;
		this->chain_height = height;
	}
};
class Compare
{
public:
	Compare(bool f= true):flag(f){}
	bool operator()(const Node &s1,const Node &s2)
	{
		if(flag)
		{
			return s1.chain_height > s2.chain_height;
		}
		else 
		{
			return s1.id > s2.id;
		}
		
	}
protected:
	bool flag;
};
class PeerNode
{
public:
	PeerNode() = default;
	~PeerNode() = default;

public:
	bool is_id_exists(id_type const& id);
	bool find_node(id_type const& id, Node& x);
	bool find_node_by_fd(int fd, Node& node_);
	
	std::vector<Node> get_nodelist(NodeType type = NODE_ALL, bool mustAlive = false);
	std::vector<Node> get_public_node();
	std::vector<Node> get_sub_nodelist(id_type const & id);
	
	void delete_node(id_type node_id);
	void delete_by_fd(int fd);
	void delete_node_by_public_node_id(id_type public_node_id);
	
	bool add(const Node& _node);
	bool add_public_node(const Node & _node);
	bool delete_public_node(const Node & _node);
	bool update(const Node & _node);
	bool update_public_node(const Node & _node);
	bool add_or_update(Node _node);
	void print(std::vector<Node> & nodelist);
	void print(const Node & node);
	std::string nodelist_info(std::vector<Node> & nodelist);
	std::string nodeid2str(std::bitset<K_ID_LEN> id);


	// 刷新线程
	bool nodelist_refresh_thread_init();
	// 线程函数
	void nodelist_refresh_thread_fun();

	void conect_nodelist();

	// 获取 ID
	const id_type get_self_id();
	void set_self_id(const id_type & id);
	void set_self_ip_p(const u32 public_ip);
	void set_self_ip_l(const u32 local_ip);
	void set_self_port_p(const u16 port_p);
	void set_self_port_l(const u16 port_l);
	void set_self_public_node(bool bl);
	void set_self_fee(uint64_t fee);
	void set_self_package_fee(uint64_t package_fee);
	void set_self_mac_md5(string mac_md5);
	void set_self_base58_address(string address);
	void set_self_public_node_id(string public_node_id);
	void set_self_chain_height(u32 height);
	void set_self_chain_height();
	u32 get_self_chain_height_newest();
	const Node get_self_node();
	bool update_fee_by_id(const string &id, uint64_t fee);
	bool update_package_fee_by_id(const string &id, uint64_t fee);

	// 生成 ID
	bool make_rand_id();
	//判断string类型的id是否合法
	bool is_id_valid(const string &id);

	
private:
	std::mutex mutex_for_nodes_;
	std::map<std::string, Node> node_map_;

	// 公网节点列表
	std::mutex mutex_for_public_nodes_;
	std::map<std::string, Node> pub_node_map_;

	Node		curr_node_;
	std::mutex	mutex_for_curr_;
	std::thread    refresh_thread;

};


#endif