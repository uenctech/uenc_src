#ifndef _NET_INTERFACE_H_
#define _NET_INTERFACE_H_


#include <iostream>
#include <map>
#include <vector>
#include <unordered_map>
#include "../net/msg_queue.h"
#include "../net/net_api.h"
#include "../net/dispatcher.h"


template <typename T>
bool net_send_message(std::string id, T& msg, bool iscompress = false);


template <typename T>
bool net_broadcast_message(T& msg);


std::string net_get_self_node_id();


Node net_get_self_node();


void net_set_self_fee(uint64_t fee);


void net_set_self_base58_address(string address);


void net_update_fee_and_broadcast(uint64_t fee);


void net_set_self_package_fee(uint64_t package_fee);


void net_update_package_fee_and_broadcast(uint64_t package_fee);


std::vector<std::string>  net_get_node_ids();


std::vector<Node> net_get_public_node();


std::map<std::string, uint64_t> net_get_node_ids_and_fees();


std::map<std::string, string> net_get_node_ids_and_base58address();


std::string net_get_ID_by_ip(std::string ip);


double net_get_connected_percent();


template <typename T>
void net_register_callback(std::function<void( const std::shared_ptr<T>& msg, const MsgData& from)> cb);



template <typename T>
bool net_send_message(std::string id, T& msg, bool iscompress)
{
    return net_com::send_message(id, msg, iscompress);
}

template <typename T>
bool net_send_message(const MsgData& from, T& msg)
{
    return net_com::send_message(from, msg);
}

template <typename T>
bool net_broadcast_message(T& msg)
{
    return net_com::broadcast_message(msg);
}

template <typename T>
void net_register_callback(std::function<void( const std::shared_ptr<T>& msg, const MsgData& from)> cb)
{
    Singleton<ProtobufDispatcher>::get_instance()->registerCallback<T>(cb);
}

#endif

