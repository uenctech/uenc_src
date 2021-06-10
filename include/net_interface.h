#ifndef _NET_INTERFACE_H_
#define _NET_INTERFACE_H_


#include <iostream>
#include <map>
#include <vector>
#include <unordered_map>
#include "../net/msg_queue.h"
#include "../net/net_api.h"
#include "../net/dispatcher.h"

// 单点发送信息，T类型是protobuf协议的类型
template <typename T>
bool net_send_message(const std::string & id, 
                        const T & msg, 
                        const net_com::Compress isCompress, 
                        const net_com::Encrypt isEncrypt, 
                        const net_com::Priority priority)
{
    return net_com::send_message(id, msg, isCompress, isEncrypt, priority);
}

template <typename T>
bool net_send_message(const std::string & id, 
                        const T & msg)
{
    return net_com::send_message(id, msg, net_com::Compress::kCompress_False, net_com::Encrypt::kEncrypt_False, net_com::Priority::kPriority_Low_0);
}

template <typename T>
bool net_send_message(const std::string & id, 
                        const T & msg, 
                        const net_com::Compress isCompress)
{
    return net_com::send_message(id, msg, isCompress, net_com::Encrypt::kEncrypt_False, net_com::Priority::kPriority_Low_0);
}

template <typename T>
bool net_send_message(const std::string & id, 
                        const T & msg, 
                        const net_com::Encrypt isEncrypt)
{
    return net_com::send_message(id, msg, net_com::Compress::kCompress_False, isEncrypt, net_com::Priority::kPriority_Low_0);
}

template <typename T>
bool net_send_message(const std::string & id, 
                        const T & msg, 
                        const net_com::Priority priority)
{
    return net_com::send_message(id, msg, net_com::Compress::kCompress_False, net_com::Encrypt::kEncrypt_False, priority);
}

// Send a message with a return receipt address. T type is the type of the ProtoBuf protocol
template <typename T>
bool net_send_message(const MsgData & from, 
                        const T & msg, 
                        const net_com::Compress isCompress, 
                        const net_com::Encrypt isEncrypt, 
                        const net_com::Priority priority)
{
    return net_com::send_message(from, msg, isCompress, isEncrypt, priority);
}

template <typename T>
bool net_send_message(const MsgData & from, 
                        const T & msg)
{
    return net_com::send_message(from, msg, net_com::Compress::kCompress_False, net_com::Encrypt::kEncrypt_False, net_com::Priority::kPriority_Low_0);
}

template <typename T>
bool net_send_message(const MsgData & from, 
                        const T & msg, 
                        const net_com::Compress isCompress)
{
    return net_com::send_message(from, msg, isCompress, net_com::Encrypt::kEncrypt_False, net_com::Priority::kPriority_Low_0);
}

template <typename T>
bool net_send_message(const MsgData & from, 
                        const T & msg, 
                        const net_com::Encrypt isEncrypt)
{
    return net_com::send_message(from, msg, net_com::Compress::kCompress_False, isEncrypt, net_com::Priority::kPriority_Low_0);
}
template <typename T>
bool net_send_message(const MsgData & from, 
                        const T & msg,
                        const net_com::Priority priority)
{
    return net_com::send_message(from, msg, net_com::Compress::kCompress_False, net_com::Encrypt::kEncrypt_False, priority);
}

// Broadcast information
template <typename T>
bool net_broadcast_message(const T& msg, 
                            const net_com::Compress isCompress, 
                            const net_com::Encrypt isEncrypt, 
                            const net_com::Priority priority)
{
    return net_com::broadcast_message(msg, isCompress, isEncrypt, priority);
}

template <typename T>
bool net_broadcast_message(const T& msg)
{
    return net_com::broadcast_message(msg, net_com::Compress::kCompress_False, net_com::Encrypt::kEncrypt_False, net_com::Priority::kPriority_Low_0);
}

template <typename T>
bool net_broadcast_message(const T& msg, 
                            const net_com::Compress isCompress)
{
    return net_com::broadcast_message(msg, isCompress, net_com::Encrypt::kEncrypt_False, net_com::Priority::kPriority_Low_0);
}

template <typename T>
bool net_broadcast_message(const T& msg, 
                            const net_com::Encrypt isEncrypt)
{
    return net_com::broadcast_message(msg, net_com::Compress::kCompress_False, isEncrypt, net_com::Priority::kPriority_Low_0);
}

template <typename T>
bool net_broadcast_message(const T& msg, 
                            const net_com::Priority priority)
{
    return net_com::broadcast_message(msg, net_com::Compress::kCompress_False, net_com::Encrypt::kEncrypt_False, priority);
}


//Returns 0 on success of obtaining its own node ID
std::string net_get_self_node_id();

//Obtain node information
Node net_get_self_node();

//Set mine fee
void net_set_self_fee(uint64_t fee);

//Set your own base58 address
void net_set_self_base58_address(string address);

//Update mine fees and broadcast the whole network
void net_update_fee_and_broadcast(uint64_t fee);

//Set Packing Fee
void net_set_self_package_fee(uint64_t package_fee);

//Update the packaging fee and broadcast the whole network
void net_update_package_fee_and_broadcast(uint64_t package_fee);

//Returns all node IDs
std::vector<std::string>  net_get_node_ids();

//Returns the public network node
std::vector<Node> net_get_public_node();

//Returns all public network nodes, whether connected or not
std::vector<Node> net_get_all_public_node();

//Returns all node IDs and FEE
std::map<std::string, uint64_t> net_get_node_ids_and_fees();

//Returns all node IDs and Base58Address
std::map<std::string, string> net_get_node_ids_and_base58address();

//Look up the ID by IP
std::string net_get_ID_by_ip(std::string ip);

//Look up the ID by IP
double net_get_connected_percent();


template <typename T>
void net_register_callback(std::function<void( const std::shared_ptr<T>& msg, const MsgData& from)> cb)
{
    Singleton<ProtobufDispatcher>::get_instance()->registerCallback<T>(cb);
}

#endif

