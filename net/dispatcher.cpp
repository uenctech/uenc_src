#include "dispatcher.h"
#include <string>
#include "../include/logging.h"
#include "net.pb.h"
#include "common.pb.h"
#include "handle_event.h"
#include "./peer_node.h"
#include "../utils/singleton.h"
#include "utils/compress.h"
#include <utility>

using namespace std;
using namespace google::protobuf;

void ProtobufDispatcher:: handle(const MsgData& data ) {

    CommonMsg common_msg;
    int r = common_msg.ParseFromString(data.pack.data);
    if (!r) {
        error("parse CommonMsg error");
        return;
    }

    std::string type = common_msg.type();
    info("ProtobufDispatcher:: handle--%s",type.c_str());
    if(type.size() == 0)
    {
        error("handle type is empty");
        return;
    }

    const Descriptor *des = DescriptorPool::generated_pool()->FindMessageTypeByName(type);
    if(!des)
    {
        error("cannot create Descriptor for %s", type.c_str());
        return;
    }

    const Message *proto = MessageFactory::generated_factory()->GetPrototype(des);
    if (!proto) 
    {
        error("cannot create Message for %s", type.c_str());
        return;
    }

    string sub_serialize_msg;
    if (common_msg.compress())
    {   
        Compress uncpr(std::move(common_msg.data()), common_msg.data().size() * 10);
        sub_serialize_msg = uncpr.m_raw_data;
    }
    else
    {   
        sub_serialize_msg = std::move(common_msg.data());
    }
    

    MessagePtr sub_msg(proto->New());
    r = sub_msg->ParseFromString(sub_serialize_msg);
    if (!r) {
        error("bad msg for protobuf for %s", type.c_str());
        return;
    }

    std::string name = sub_msg->GetDescriptor()->name();
    auto p = protocbs_.find(name);
    if (p != protocbs_.end()) {
        
        Node node;
        auto find = Singleton<PeerNode>::get_instance()->find_node_by_fd(data.fd, node);
        if (find){
            
            node.ResetHeart();
            Singleton<PeerNode>::get_instance()->update(node);
        }
        p->second( sub_msg, data );
    } else {
        
        return;
    }
}

void ProtobufDispatcher::registerAll()
{
    registerCallback<RegisterNodeReq>(handleRegisterNodeReq);
    registerCallback<RegisterNodeAck>(handleRegisterNodeAck);
    registerCallback<PrintMsgReq>(handlePrintMsgReq);
    registerCallback<ConnectNodeReq>(handleConnectNodeReq);
    registerCallback<TransMsgReq>(handleTransMsgReq);
    registerCallback<NotifyConnectReq>(handleNotifyConnectReq);
    registerCallback<PingReq>(handlePingReq);
    registerCallback<PongReq>(handlePongReq);
    registerCallback<SyncNodeReq>(handleSyncNodeReq);
    registerCallback<SyncNodeAck>(handleSyncNodeAck);
    registerCallback<EchoReq>(handleEchoReq);
    registerCallback<EchoAck>(handleEchoAck);
    registerCallback<UpdateFeeReq>(handleUpdateFeeReq);
    registerCallback<UpdatePackageFeeReq>(handleUpdatePackageFeeReq);

}