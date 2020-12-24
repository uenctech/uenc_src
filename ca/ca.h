#ifndef EBPC_CA_H
#define EBPC_CA_H

#include <iostream>
#include <thread>

#include "../proto/ca_protomsg.pb.h"
#include "../net/msg_queue.h"

/**
 * @description: ca 初始化log
 */
void ca_initConfig();
/**
 * @description: ca 初始化
 * @param 无
 * @return: 成功返回ture，失败返回false
 */
bool ca_init();

/**
 * @description: ca 清理函数
 * @param 无
 * @return: 无
 */
void ca_cleanup();

bool id_isvalid(const std::string &id);

/**
 * @description: ca 版本
 * @param 无
 * @return: 返回字符串格式版本号
 */
const char * ca_version();


/**
 * @description: 获取某个节点的最高块的高度和hash
 * @param id 要获取的节点的id
 * @return: 无
 */
void SendDevInfoReq(const std::string id);


/* ====================================================================================  
 # @description:  处理接收到的节点信息
 # @param msg  : 接收的协议数据
 # @param msgdata : 网络传输所需的数据
 ==================================================================================== */
void HandleGetDevInfoAck( const std::shared_ptr<GetDevInfoAck>& msg, const MsgData& msgdata );


/* ====================================================================================  
 # @description: 处理接收到的获取节点信息请求
 # @param msg  : 接收的协议数据
 # @param msgdata : 网络传输所需的数据
 ==================================================================================== */
void HandleGetDevInfoReq( const std::shared_ptr<GetDevInfoReq>& msg, const MsgData& msgdata );


/**
 * @description: 主菜单使用的相关实现函数
 * @create: 20201104   LiuMingLiang
 */
void ca_print_basic_info();
int set_device_signature_fee(uint64_t fee);
int set_device_package_fee(uint64_t fee);
int get_device_package_fee(uint64_t& packageFee);

bool isPublicIp(const string& ip);
int UpdatePublicNodeToConfigFile();

#endif
