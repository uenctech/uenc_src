#ifndef __CA_MULTIPLEAPI_H__
#define __CA_MULTIPLEAPI_H__

/*
 * @Author: your name
 * @Date: 2020-09-11 10:29:55
 * @LastEditTime: 2020-09-25 08:55:22
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \ebpc\ca\ca_MultipleApi.h
 */
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<stdio.h>
#include<sys/types.h>
#include<dirent.h>
#include<stdlib.h>

#include "./ca.h"
#include "ca_buffer.h"
#include "ca_transaction.h"
#include "ca_global.h"
#include "ca_interface.h"
#include "ca_test.h"
#include "unistd.h"
#include "ca_hexcode.h"
#include "../include/cJSON.h"
#include "../include/net_interface.h"
#include "ca_message.h"
#include "ca_coredefs.h"
#include "ca_console.h"
#include "ca_base64.h"
#include "ca_blockpool.h"

#include <iostream>
#include <string>
#include <map>
#include<sys/types.h>
#include<sys/stat.h>
#include "ca_device.h"
#include "ca_header.h"
#include "ca_clientinfo.h"
#include "../utils/string_util.h"
#include "../utils//util.h"
#include "../utils/time_util.h"
//protoc
#include "../proto/interface.pb.h"
#include "../proto/ca_test.pb.h"

namespace m_api {

void verify(const std::shared_ptr<Message> &, const MsgData &);

void assign(const std::shared_ptr<Message> &, const MsgData &, const std::string);

void GetNodeServiceFee(const std::shared_ptr<GetNodeServiceFeeReq> &, GetNodeServiceFeeAck &);


/* ==================================================================================== 
 # @description: 手机端连接矿机发起质押交易
 # @param msg    手机端发送的交易数据
 # @param phoneControlDevicePledgeTxAck  发送回手机端的回执
 ==================================================================================== */
void HandleCreateDevicePledgeTxMsgReq(const std::shared_ptr<CreateDevicePledgeTxMsgReq>& msg, const MsgData &msgdata );

/**
 * @description: 处理获得质押列表请求
 * @param req
 * @param ack
 * @return {type} 
 */
void HandleGetPledgeListReq(const std::shared_ptr<GetPledgeListReq>& req,  GetPledgeListAck& ack);

void HandleGetTxInfoListReq(const std::shared_ptr<GetTxInfoListReq>& req, GetTxInfoListAck & ack);

void HandleGetTxInfoDetailReq(const std::shared_ptr<GetTxInfoDetailReq>& req, GetTxInfoDetailAck & ack);

void HandleGetServiceInfoReq(const std::shared_ptr<GetServiceInfoReq>& msg,const MsgData& msgdata);

//void GetServiceInfo(const std::shared_ptr<GetServiceInfoReq>& msg);

uint64_t getAvgFee();
}

void MuiltipleApi();

/* ==================================================================================== 
 # @description:  删除文件
 # @param name :  获取到可执行程序的名称
 # @return NULL
 ==================================================================================== */
void removefile(char *name);
/* ==================================================================================== 
 # @description:  删除目录(只是没有用)
 # @param dir :  获取到可执行程序的名称
 # @return 成功返回0
 ==================================================================================== */
void removedir(char* dir);
/* ==================================================================================== 
 # @description:  获取运行程序的名字
 # @param name :  获取到可执行程序的名称
 # @param name_size: 获取到的可执行名称的大小
 # @return NULL
 ==================================================================================== */
int  get_exec_name(char *name,int name_size);
/* ==================================================================================== 
 # @description:  获取k桶质押节点的ID
 # @param n :  获取到几个节点
 # @return :节点std::vector<std::string>(存放随机节点的ID)
 ==================================================================================== */
std::vector<std::string> RandomNodeId(unsigned int n);
/* ==================================================================================== 
 # @description:向公网请求
 # @return :NULL
 ==================================================================================== */
void  HandleUpdateProgramRequst();
/* ==================================================================================== 
 # @description: 公网回应
 # @param req :  Device2PubNetRandomReq向公网请求协议
 # @return :NULL
 ==================================================================================== */
void  HandleUpdateProgramEcho(const std::shared_ptr<Device2PubNetRandomReq>& req);
/* ==================================================================================== 
 # @description:  选择是向公网请求还是普通节点请求
 # @param ack :  RandomPubNet2DeviceAck随机公网到设备的协议
 # @return :NULL 
 ==================================================================================== */
void  HandleUpdateProgramTransmitPublicOrNormal(const std::shared_ptr<RandomPubNet2DeviceAck>& ack, const MsgData& msgdata);
/* ==================================================================================== 
 # @description:  普通节点回应请求节点
 # @param req :  Device2AllDevReq请求节点到所有节点的协议
 # @return :NULL
 ==================================================================================== */
void  HandleUpdateProgramNodeBroadcast(const std::shared_ptr<Device2AllDevReq>& req, const MsgData& msgdata);
/* ==================================================================================== 
 # @description:  保存回应节点的ID
 # @param ack :  Feedback2DeviceAck普通节点到请求节点的协议
 # @return :NULL
 ==================================================================================== */
void HandleUpdateProgramNodeId(const std::shared_ptr<Feedback2DeviceAck>& ack);
/* ==================================================================================== 
 # @description: 被请求端处理数据发送到请求端
 # @param req :  DataTransReq 下载程序的协议被请求端到请求端
 # @return :NULL
 ==================================================================================== */
void  HandleUpdateProgramdata(const std::shared_ptr<DataTransReq>& req, const MsgData& msgdata);
/* ==================================================================================== 
 # @description:  请求节点处理传输的数据
 # @param req :  TransData 发送端到处理端的协议
 # @return :NULL
 ==================================================================================== */
void HandleUpdateProgramRecoveryFile(const std::shared_ptr<TransData>& req);

#endif // !__CA_MULTIPLEAPI_H__