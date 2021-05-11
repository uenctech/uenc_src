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

// 带回执地址发送信息，T类型是protobuf协议的类型
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

// 广播信息
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


//获取自己节点ID 成功返回0
std::string net_get_self_node_id();

//获取节点信息
Node net_get_self_node();

//设置矿费
void net_set_self_fee(uint64_t fee);

//设置自己的base58地址
void net_set_self_base58_address(string address);

//更新矿费并进行全网广播
void net_update_fee_and_broadcast(uint64_t fee);

//设置打包费
void net_set_self_package_fee(uint64_t package_fee);

//更新打包费并进行全网广播
void net_update_package_fee_and_broadcast(uint64_t package_fee);

//返回所有节点ID
std::vector<std::string>  net_get_node_ids();

//返回公网节点
std::vector<Node> net_get_public_node();

//返回所有公网节点，不管是否连接
std::vector<Node> net_get_all_public_node();

//返回所有节点ID和fee
std::map<std::string, uint64_t> net_get_node_ids_and_fees();

//返回所有节点ID和base58address
std::map<std::string, string> net_get_node_ids_and_base58address();

//通过ip查找ID
std::string net_get_ID_by_ip(std::string ip);

//返回连接上节点的百分比
double net_get_connected_percent();


template <typename T>
void net_register_callback(std::function<void( const std::shared_ptr<T>& msg, const MsgData& from)> cb)
{
    Singleton<ProtobufDispatcher>::get_instance()->registerCallback<T>(cb);
}

#endif

