/*
 * @Author: your name
 * @Date: 2021-01-14 17:59:45
 * @LastEditTime: 2021-03-10 17:47:14
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \ebpc\net\handle_event.h
 */

#ifndef _HANDLE_EVENT_H_
#define _HANDLE_EVENT_H_

#include <memory>
#include "net.pb.h"
#include "./msg_queue.h"

void handlePrintMsgReq(const std::shared_ptr<PrintMsgReq> &printMsgReq, const MsgData &from);
void handleRegisterNodeReq(const std::shared_ptr<RegisterNodeReq> &registerNode, const MsgData &from);
void handleRegisterNodeAck(const std::shared_ptr<RegisterNodeAck> &registerNodeAck, const MsgData &from);
void handleConnectNodeReq(const std::shared_ptr<ConnectNodeReq> &connectNodeReq, const MsgData &from);
void handleBroadcastNodeReq(const std::shared_ptr<BroadcastNodeReq> &broadcastNodeReq, const MsgData &from);
void handleTransMsgReq(const std::shared_ptr<TransMsgReq> &transMsgReq, const MsgData &from);
void handleBroadcastMsgReq(const std::shared_ptr<BroadcaseMsgReq> &broadcaseMsgReq, const MsgData &from);
void handleNotifyConnectReq(const std::shared_ptr<NotifyConnectReq> &transMsgReq, const MsgData &from);

void handlePingReq(const std::shared_ptr<PingReq> &pingReq, const MsgData &from);
void handlePongReq(const std::shared_ptr<PongReq> &pongReq, const MsgData &from);
void handleSyncNodeReq(const std::shared_ptr<SyncNodeReq> &syncNodeReq, const MsgData &from);
void handleSyncNodeAck(const std::shared_ptr<SyncNodeAck> &syncNodeAck, const MsgData &from);
void handleEchoReq(const std::shared_ptr<EchoReq> &echoReq, const MsgData &from);
void handleEchoAck(const std::shared_ptr<EchoAck> &echoAck, const MsgData &from);

void handleUpdateFeeReq(const std::shared_ptr<UpdateFeeReq> &updateFeeReq, const MsgData &from);
void handleUpdatePackageFeeReq(const std::shared_ptr<UpdatePackageFeeReq> &updatePackageFeeReq, const MsgData &from);

void handleGetHeightReq(const std::shared_ptr<GetHeightReq> &heightReq, const MsgData &from);
void handleGetHeightAck(const std::shared_ptr<GetHeightAck> &heightAck, const MsgData &from);

void handleGetTransInfoReq(const std::shared_ptr<GetTransInfoReq> &transInfoReq, const MsgData &from);
void handleGetTransInfoAck(const std::shared_ptr<GetTransInfoAck> &transInfoAck, const MsgData &from);
#endif