#ifndef __CA_SYNCHRONIZATION_H_
#define __CA_SYNCHRONIZATION_H_

#include <vector>
#include <string>
#include <mutex>
#include "../proto/ca_protomsg.pb.h"
#include "net/msg_queue.h"
#include "../include/net_interface.h"
#include "ca_global.h"

const int SYNCNUM = 5;      
const int CHECKNUM = 10;    
const int CHECK_HEIGHT = 100;  
const int HASH_LEN = 6;       
const int SYNC_NUM_LIMIT = 500;   


struct SyncNode
{
    std::string id = "";          
    int64_t height = 0;          
    std::string hash = "";        
    
    SyncNode(){
    }
    SyncNode(std::string id_, int64_t height_, std::string hash_):id(id_),height(height_),hash(hash_){

    }
    bool operator <(const SyncNode& other)
    {
        if(height < other.height)
        {
            return true;
        }
        return false;
    }

};

class Sync
{
public:
    Sync() = default;
    ~Sync()  = default;

    bool SetPotentialNodes(const std::string &id, const int64_t &height, const std::string &hash);

    void Process();
    bool DataSynch(std::string id);

    SyncNode verifying_node;                  
    std::vector<SyncNode> verifying_result;   
    std::vector<SyncNode> potential_nodes;    
    bool is_sync = false;
    std::mutex mu_potential_nodes;
    std::mutex mu_verifying_result;
    int conflict_height = -1;
    bool sync_adding = false;
};



void SendSyncGetnodeInfoReq(std::string id);
void SendVerifyReliableNodeReq(std::string id, int64_t height);
void SendSyncBlockInfoReq(std::string id);

void HandleSyncGetnodeInfoReq( const std::shared_ptr<SyncGetnodeInfoReq>& msg, const MsgData& msgdata );
void HandleSyncGetnodeInfoAck( const std::shared_ptr<SyncGetnodeInfoAck>& msg, const MsgData& msgdata );

void HandleVerifyReliableNodeReq( const std::shared_ptr<VerifyReliableNodeReq>& msg, const MsgData& msgdata );
void HandleVerifyReliableNodeAck( const std::shared_ptr<VerifyReliableNodeAck>& msg, const MsgData& msgdata );

void HandleSyncBlockInfoReq( const std::shared_ptr<SyncBlockInfoReq>& msg, const MsgData& msgdata );
void HandleSyncBlockInfoAck( const std::shared_ptr<SyncBlockInfoAck>& msg, const MsgData& msgdata );

void HandleSyncLoseBlockReq( const std::shared_ptr<SyncLoseBlockReq>& msg, const MsgData& msgdata );
void HandleSyncLoseBlockAck( const std::shared_ptr<SyncLoseBlockAck>& msg, const MsgData& msgdata );

bool IsOverIt(int64_t height);
std::string get_blkinfo_ser(int64_t begin, int64_t end, int64_t max_num);
std::vector<std::string> get_blkinfo(int64_t begin, int64_t end, int64_t max_num);
int SyncData(std::string &headerstr, bool isSync = false);
template <typename T>
void SetSyncHeaderMsg(T& msg);

template <typename T>
void SetSyncHeaderMsg(T& msg)
{
	SyncHeaderMsg * pSyncHeaderMsg = msg.mutable_syncheadermsg();
    pSyncHeaderMsg->set_version(getVersion());
	pSyncHeaderMsg->set_id(net_get_self_node_id());
}


#endif