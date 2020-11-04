
#ifndef _HANDLE_EVENT_H_
#define _HANDLE_EVENT_H_

#include <memory>
#include "net.pb.h"
#include "./msg_queue.h"

void handlePrintMsgReq(const std::shared_ptr<PrintMsgReq>& printMsgReq, const MsgData& from);
void handleRegisterNodeReq(const std::shared_ptr<RegisterNodeReq>& registerNode, const MsgData& from);
void handleRegisterNodeAck(const std::shared_ptr<RegisterNodeAck>& registerNodeAck, const MsgData& from);
void handleConnectNodeReq(const std::shared_ptr<ConnectNodeReq>& connectNodeReq, const MsgData& from);
void handleTransMsgReq(const std::shared_ptr<TransMsgReq>& transMsgReq, const MsgData& from);
void handleNotifyConnectReq(const std::shared_ptr<NotifyConnectReq>& transMsgReq, const MsgData& from);

void handlePingReq(const std::shared_ptr<PingReq>& pingReq, const MsgData& from);
void handlePongReq(const std::shared_ptr<PongReq>& pongReq, const MsgData& from);
void handleSyncNodeReq(const std::shared_ptr<SyncNodeReq>& syncNodeReq, const MsgData& from);
void handleSyncNodeAck(const std::shared_ptr<SyncNodeAck>& syncNodeAck, const MsgData& from);
void handleEchoReq(const std::shared_ptr<EchoReq>& echoReq, const MsgData& from);
void handleEchoAck(const std::shared_ptr<EchoAck>& echoAck, const MsgData& from);

void handleUpdateFeeReq(const std::shared_ptr<UpdateFeeReq>& updateFeeReq, const MsgData& from);
void handleUpdatePackageFeeReq(const std::shared_ptr<UpdatePackageFeeReq>& updatePackageFeeReq, const MsgData& from);

#endif