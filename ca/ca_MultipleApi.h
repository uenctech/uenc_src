#ifndef __CA_MULTIPLEAPI_H__
#define __CA_MULTIPLEAPI_H__

/*
 * @Author: your name
 * @Date: 2020-09-11 10:29:55
 * @LastEditTime: 2021-04-26 09:48:49
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
#include "../include/net_interface.h"
#include "ca_message.h"
#include "ca_coredefs.h"
#include "ca_console.h"
#include "ca_base64.h"
#include "ca_blockpool.h"
#include "../common/devicepwd.h"
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

int64_t getAvgFee();
}

void MuiltipleApi();

/* ==================================================================================== 
 # @description: 请求获取挂起的交易列表
 # @param req :  GetTxPendingListReq 获取挂起交易的协议
 # @return :NULL
 ==================================================================================== */
void HandleGetTxPendingListReq(const std::shared_ptr<GetTxPendingListReq>& req, const MsgData& msgdata);

/* ==================================================================================== 
 # @description: 请求获取失败的交易列表
 # @param req :  GetTxFailureListReq 获取失败交易的协议
 # @return :NULL
 ==================================================================================== */
void HandleGetTxFailureListReq(const std::shared_ptr<GetTxFailureListReq>& req, GetTxFailureListAck& ack);

void HandleGetTxByHashReq(const std::shared_ptr<GetTxByHashReq>& req, const MsgData& msgdata);
bool  GetTxByTxHashFromRocksdb(vector<string>txhash,vector<CTransaction> & outTx);

void handleCheckNodeHeightReq(const std::shared_ptr<CheckNodeHeightReq>& req, const MsgData& msgdata);

#endif // !__CA_MULTIPLEAPI_H__