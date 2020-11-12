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

#include "ca_device.h"
#include "ca_header.h"
#include "ca_clientinfo.h"
#include "../utils/string_util.h"
#include "../utils//util.h"
#include "../utils/time_util.h"

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


uint64_t getAvgFee();
}

void MuiltipleApi();


#endif 