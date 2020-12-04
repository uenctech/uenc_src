/*
 * @Author: your name
 * @Date: 2020-09-18 18:01:27
 * @LastEditTime: 2020-10-13 20:33:03
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \ebpc\ca\ca_phonetx.h
 */
#ifndef __CA_PHONETX_H__
#define __CA_PHONETX_H__

// net头文件
#include "../net/msg_queue.h"

// proto头文件
#include "../proto/ca_protomsg.pb.h"
#include "../proto/transaction.pb.h"
#include "../proto/interface.pb.h"

#endif // !__CA_PHONETX_H__

enum ERRCODE 
{
    PHONE_TX_SUCCESS          = 0, // 交易成功
    PHONE_TX_VERSION_ERROR    = -1, // 版本错误
    PHONE_TX_PARAM_ERROR      = -2,  // 参数错误
    PHONE_TX_CREATE_ERROR  = -3, // 创建错误
    PHONE_TX_TRANSACTION_ERROR = -4, // 交易获取错误
    PHONE_TX_ADDR_ERROR = -5, // 地址错误
    PHONE_TX_NO_AMOUNT = -6, // 余额不足
    PHONE_TX_TXN_ERROR = -7, // 余额不足
    PHONE_TX_GET_PACK_FEE_ERROR = -8, // 余额不足
    PHONE_TX_PASSWORD_ERROR = -9, // 密码错误
};



/* ==================================================================================== 
 # @description:  处理手机端发送的多重交易请求
 # @param msg     手机端发送的消息体
 # @param msgdata 网络通信所必须的信息
 ==================================================================================== */
void HandleCreateMultiTxReq( const std::shared_ptr<CreateMultiTxMsgReq>& msg, const MsgData& msgdata );


/* ==================================================================================== 
 # @description:  处理接受到的手机端交易
 # @param msg     手机端发送的消息体
 # @param msgdata 网络通信所必须的信息
 ==================================================================================== */
void HandlePreMultiTxRaw( const std::shared_ptr<MultiTxMsgReq>& msg, const MsgData& msgdata );

/* ==================================================================================== 
 # @description:  手机端连接矿机发起多重交易
 # @param msg     手机端发送的消息体
 # @param msgdata 网络通信所必须的信息
 ==================================================================================== */
void HandleCreateDeviceMultiTxMsgReq(const std::shared_ptr<CreateDeviceMultiTxMsgReq>& msg, const MsgData& msgdata);