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
	NOTYET,		
	DRTI2I,		
	DRTI2O,		
	DRTO2I,		
	DRTO2O,		
	HOLING,		
	BYHOLE,		
	BYSERV,		
	PASSIV		
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
	std::string					base58address = "";

	Node(){

	}
	Node(std::string node_id){
		id = node_id;
	}
	bool operator ==(const Node & obj) const
	{
	    return id == obj.id;
	}
	void ResetHeart()
	{
		heart_time  = HEART_TIME;
		heart_probes = HEART_PROBES;	
	}
	void print()
	{
		std::cout
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
			<< std::endl;
	}
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
	std::vector<Node> get_nodelist(NodeType type = NODE_ALL);
	bool delete_node(id_type node_id);
	bool delete_by_fd(int fd);
	void delete_by_fd_not_close(int fd);
	bool add(const Node& _node);
	bool update(const Node & _node);
	bool add_or_update(Node _node);
	void print(std::vector<Node> nodelist);

	std::string nodeid2str(std::bitset<K_ID_LEN> id);



	
	bool nodelist_refresh_thread_init();
	
	void nodelist_refresh_thread_fun();

	void conect_nodelist();

	
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
	const Node get_self_node();
	bool update_fee_by_id(const string &id, uint64_t fee);
	bool update_package_fee_by_id(const string &id, uint64_t fee);

	
	bool make_rand_id();
	
	bool is_id_valid(const string &id);

	
private:
	std::mutex mutex_for_nodes_;
	std::map<std::string, Node> node_map_;

	Node		curr_node_;
	std::mutex	mutex_for_curr_;
	std::thread    refresh_thread;

};


#endif