#include <iostream>
#include <thread>
#include <unistd.h>
#include <map>
#include "ca_global.h"
#include "ca_message.h"
#include "ca_coredefs.h"
#include "ca_serialize.h"
#include "ca_transaction.h"
#include "ca_synchronization.h"
#include "ca_test.h"
#include "ca_blockpool.h"
#include "ca_hexcode.h"
#include "../include/net_interface.h"
#include "ca_rocksdb.h"
#include "MagicSingleton.h"
#include "../common/config.h"
#include "../include/logging.h"
#include "../include/ScopeGuard.h"
#include "ca_console.h"
#include "../proto/ca_protomsg.pb.h"
#include <algorithm>
#include <tuple>
#include "../utils/string_util.h"


/* 设置获取到的其他节点的最高块信息 */ 
bool Sync::SetPotentialNodes(const std::string &id, const int64_t &height, const std::string &hash)
{
	std::lock_guard<std::mutex> lck(mu_potential_nodes);
    if(id.size() == 0)
    {
        return false;
    }

    // 遍历现有数据，防止重复数据写入
    uint64_t size = this->potential_nodes.size();
    for(uint64_t i = 0; i < size; i++)
    {
        if(0 == id.compare(potential_nodes[i].id))
        {
            return false;
        }
    }
    struct SyncNode data = {id, height, hash};
    this->potential_nodes.push_back(data);
    return true;
}


void Sync::SetPledgeNodes(const std::vector<std::string> & ids)
{
	std::lock_guard<std::mutex> lck(mu_get_pledge);
	std::vector<std::string> pledgeNodes = this->pledgeNodes;
	this->pledgeNodes.clear();

	std::vector<std::string> v_diff;
	std::set_difference(ids.begin(),ids.end(),pledgeNodes.begin(),pledgeNodes.end(),std::back_inserter(v_diff));

	this->pledgeNodes.assign(v_diff.begin(), v_diff.end());
}



// 同步开始
void Sync::Process()
{   
	if(sync_adding)
	{
		std::cout << "sync_adding..." << std::endl;
		return;
	}               
    // std::cout << "\n======Sync block begin====== " << std::endl;
    potential_nodes.clear();
	verifying_node.id.clear();

    // === 1.寻找潜在可靠节点 ===
    std::vector<std::string> nodes = net_get_node_ids();
    if(nodes.size() == 0)
    {
        error("sync block not have node!!");
		return;
    }
    int nodesum = std::min(SYNCNUM, (int)nodes.size());

    /* 随机选择节点，保证公平*/
    std::vector<std::string> sendid = randomNode(nodesum);
   	// std::cout << "sync send is size :" << sendid.size() << std::endl;
    for(auto& id : sendid)
    {
        SendSyncGetnodeInfoReq(id);
    }
    sleep(3);
    if(potential_nodes.size() == 0)
    {
        error("potential_nodes == 0");
		return;
    }
    // std::cout << "potential_nodes list:" << std::endl;
    std::sort(potential_nodes.begin(), potential_nodes.end());
    // for(auto i : potential_nodes)
    // {
    //     printf("id: %s ", i.id.c_str());
    //     printf("height: %ld ", i.height);
    //     printf("hash: %s\n", i.hash.c_str());
    // }

	if(IsOverIt(potential_nodes.back().height))
	{
		// std::cout << "sync is over other,not sync." << std::endl;
		return;
	}
    // === 2.验证潜在可靠节点 ===
    while(potential_nodes.size() > 0)
    {
		verifying_result.clear();
        verifying_node = potential_nodes.back();

        sendid = randomNode(nodesum);
		for (auto it = sendid.begin(); it != sendid.end(); ++it) 
		{
			if(*it == verifying_node.id)
			{
				it = sendid.erase(it);
				break;
			}
		}

        for(auto& id : sendid)
        {
            SendVerifyReliableNodeReq(id, verifying_node.height);
        }
        sleep(3); 
		if(verifying_result.size() == 0)
		{
			error("verifying_result == 0");
			potential_nodes.pop_back();
			continue;
		}		
        //检查verifying_result
        // std::cout << "verifying_node id:"<< verifying_node.id 
        //     << " height:" << verifying_node.height
        //     << " hash:" << verifying_node.hash << endl;

		bool is_alone = true;
		for(auto& other: verifying_result)
		{
            // std::cout << "other id:"<< other.id 
            //     << " height:" << other.height
            //     << " hash:" << other.hash << endl;  			
			if(other.height >= verifying_node.height)
			{
				is_alone = false;
			}
		}
		if(is_alone)
		{
			// error("sync verifying_node height is alone!!!");
			potential_nodes.pop_back();
			continue;
		}

        bool isok = false;
        for(auto& other: verifying_result)
        {      
            if(other.height >= verifying_node.height && other.hash ==  verifying_node.hash)
            {
                isok = true;
            }
        }
        if(isok)
        {
            break;
        }else{
            potential_nodes.pop_back();
        }
    }
	if(potential_nodes.size() == 0){
		error("over potential_nodes.size() == 0");
		return;
	}
    DataSynch(verifying_node.id);
}

/* 发起同步请求*/
bool Sync::DataSynch(std::string id)
{
    if(0 == id.size())
    {
		error("DataSynch:id is empty!!");
        return false;
    }
    SendSyncBlockInfoReq(id);   
    return true;
}



//============区块同步交互协议================

void SendSyncGetnodeInfoReq(std::string id)
{
	if(id.size() == 0)
	{
		return;
	}

    SyncGetnodeInfoReq getBestchainInfoReq;
    SetSyncHeaderMsg<SyncGetnodeInfoReq>(getBestchainInfoReq);
	net_send_message<SyncGetnodeInfoReq>(id, getBestchainInfoReq, true);
}

int SendVerifyPledgeNodeReq(std::vector<std::string> ids)
{
	if (ids.size() == 0)
	{
		return -1;
	}

	SyncVerifyPledgeNodeReq syncVerifyPledgeNodeReq;
	SetSyncHeaderMsg<SyncVerifyPledgeNodeReq>(syncVerifyPledgeNodeReq);

	for (auto id : ids)
	{
		syncVerifyPledgeNodeReq.add_ids(id);
	}

	for (auto id : ids)
	{
		net_send_message<SyncVerifyPledgeNodeReq>(id, syncVerifyPledgeNodeReq, true);
	}

	return 0;
}

void HandleSyncVerifyPledgeNodeReq( const std::shared_ptr<SyncVerifyPledgeNodeReq>& msg, const MsgData& msgdata )
{
	// 判断版本是否兼容
	SyncHeaderMsg * HeaderMsg= msg->mutable_syncheadermsg();
	if( 0 != IsVersionCompatible( HeaderMsg->version() ) )
	{
		error("HandleSyncGetnodeInfoReq IsVersionCompatible");
		return ;
	}
	
	if (msg->ids_size() == 0)
	{
		error("(HandleSyncVerifyPledgeNodeReq) msg->ids_size() == 0");
		return ;
	}

	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if (txn == NULL)
	{
		error("(HandleSyncVerifyPledgeNodeReq) TransactionInit failed !");
		return ;
	}

	bool bRollback = true;
	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, bRollback);
	};

	std::vector<std::string> addrs;
	if ( 0 != pRocksDb->GetPledgeAddress(txn, addrs) )
	{
		error("(HandleSyncVerifyPledgeNodeReq) GetPledgeAddress failed!");
		return ;
	}

	SyncVerifyPledgeNodeAck syncVerifyPledgeNodeAck;

	std::map<std::string, std::string> idBase58s = net_get_node_ids_and_base58address();
	for (auto & idBase58 : idBase58s)
	{
		auto iter = find(addrs.begin(), addrs.end(), idBase58.second);
		if (iter != addrs.end())
		{
			uint64_t amount = 0;
			SearchPledge(idBase58.second, amount);

			if (amount >= g_TxNeedPledgeAmt)
			{
				syncVerifyPledgeNodeAck.add_ids(idBase58.first);
			}
		}
	}

	if (syncVerifyPledgeNodeAck.ids_size() == 0)
	{
		error("(HandleSyncVerifyPledgeNodeReq) ids == 0!");
		return ;
	}

	SyncHeaderMsg * headerMsg = msg->mutable_syncheadermsg();
	net_send_message<SyncVerifyPledgeNodeAck>(headerMsg->id(), syncVerifyPledgeNodeAck, true);
}

void HandleSyncVerifyPledgeNodeAck( const std::shared_ptr<SyncVerifyPledgeNodeAck>& msg, const MsgData& msgdata )
{
	if (msg->ids_size() == 0)
	{
		return ;
	}

	std::vector<std::string> ids;
	for (int i = 0; i < msg->ids_size(); ++i)
	{
		ids.push_back(msg->ids(i));
	}

	g_synch->SetPledgeNodes(ids);
}

void SendSyncGetPledgeNodeReq(std::string id)
{
	if(id.size() == 0)
	{
		return;
	}

    SyncGetPledgeNodeReq getBestchainInfoReq;
    SetSyncHeaderMsg<SyncGetPledgeNodeReq>(getBestchainInfoReq);
	net_send_message<SyncGetPledgeNodeReq>(id, getBestchainInfoReq, true);
}

void HandleSyncGetPledgeNodeReq( const std::shared_ptr<SyncGetPledgeNodeReq>& msg, const MsgData& msgdata )
{
	// 判断版本是否兼容
	SyncHeaderMsg * HeaderMsg= msg->mutable_syncheadermsg();
	if( 0 != IsVersionCompatible( HeaderMsg->version() ) )
	{
		error("HandleSyncGetnodeInfoReq IsVersionCompatible");
		return ;
	}
	
	SyncGetPledgeNodeAck syncGetPledgeNodeAck;

	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if( txn == NULL )
	{
		return ;
	}

	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, false);
	};

	std::vector<string> addresses;
	pRocksDb->GetPledgeAddress(txn, addresses);

	std::map<std::string, std::string> idBase58s = net_get_node_ids_and_base58address();
	for (auto idBase58 : idBase58s)
	{
		auto iter = find(addresses.begin(), addresses.end(), idBase58.second);
		if (iter != addresses.end())
		{
			uint64_t amount = 0;
			SearchPledge(idBase58.second, amount);

			if (amount >= g_TxNeedPledgeAmt)
			{
				syncGetPledgeNodeAck.add_ids(idBase58.first);
			}
		}
	}
	SyncHeaderMsg * headerMsg = msg->mutable_syncheadermsg();
	net_send_message<SyncGetPledgeNodeAck>(headerMsg->id(), syncGetPledgeNodeAck, true);
}

void HandleSyncGetPledgeNodeAck( const std::shared_ptr<SyncGetPledgeNodeAck>& msg, const MsgData& msgdata )
{
	std::vector<std::string> ids;
	for (int i = 0; i < msg->ids_size(); ++i)
	{
		ids.push_back(msg->ids(i));
	}

	g_synch->SetPledgeNodes(ids);
}

// 验证潜在可靠节点请求
void SendVerifyReliableNodeReq(std::string id, int64_t height)
{
	if(id.size() == 0)
	{
		return;
	}

    VerifyReliableNodeReq verifyReliableNodeReq;
	SetSyncHeaderMsg<VerifyReliableNodeReq>(verifyReliableNodeReq);

    verifyReliableNodeReq.set_height(height);

	net_send_message<VerifyReliableNodeReq>(id, verifyReliableNodeReq, true);
}

std::vector<CheckHash> get_check_hash(int height)
{
	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();

	bool bRollback = true;
	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, bRollback);
	};

	std::vector<CheckHash> v_checkhash;
	if(height < 1) 
	{
		return v_checkhash;
	}
	int check_end = (height / CHECK_HEIGHT);        // 5
	int check_num = std::min(check_end, (CHECKNUM-1) ); //5
	int check_begin = check_end - check_num; //0
	std::vector<std::tuple<int, int>> check_range; 
	for(int i = check_begin; i < check_end; i++)
	{
		check_range.push_back(std::make_tuple(i*100, (i+1)*100 - 1));
	}
	check_range.push_back(std::make_tuple(check_end*100, height - 1));

    for(auto i: check_range)
    {
		int begin = std::get<0>(i);
		int end = std::get<1>(i);
		std::string str;

		std::vector<std::string> hashs;
		for(auto j = begin; j <= end; j++)
		{
			std::vector<std::string> vBlockHashs;
			pRocksDb->GetBlockHashsByBlockHeight(txn, j, vBlockHashs); //j height
			std::sort(vBlockHashs.begin(), vBlockHashs.end());
			hashs.push_back(StringUtil::concat(vBlockHashs, "_"));
		}
		CheckHash checkhash;
		checkhash.set_begin(begin);
		checkhash.set_end(end);
		if(hashs.size() > 0)
		{
			std::string all_hash = getsha256hash(StringUtil::concat(hashs, "_"));
			checkhash.set_hash(all_hash.substr(0,HASH_LEN));
		}
		v_checkhash.push_back(checkhash);
		// std::cout << "begin:" << begin << std::endl;
		// std::cout << "end:" << end << std::endl;
		// std::cout << "all_hash:" << all_hash.substr(0,HASH_LEN) << std::endl;
    }
	return v_checkhash;
}


void SendSyncBlockInfoReq(std::string id)
{
	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if( txn == NULL )
	{
		return ;
	}

	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, false);
	};

	unsigned int blockHeight;
	pRocksDb->GetBlockTop(txn, blockHeight);


	SyncBlockInfoReq syncBlockInfoReq;
	SetSyncHeaderMsg<SyncBlockInfoReq>(syncBlockInfoReq);

	syncBlockInfoReq.set_height(blockHeight);
	int sync_cnt = Singleton<Config>::get_instance()->GetSyncDataCount();
	syncBlockInfoReq.set_max_num(sync_cnt);
	
	std::vector<CheckHash> v_checkhash = get_check_hash(blockHeight);
	for(auto i:v_checkhash)
	{
		CheckHash *checkhash = syncBlockInfoReq.add_checkhash();
		checkhash->set_begin(i.begin());
		checkhash->set_end(i.end());
		checkhash->set_hash(i.hash());
	}
	net_send_message<SyncBlockInfoReq>(id, syncBlockInfoReq, true);
}


void HandleSyncGetnodeInfoReq( const std::shared_ptr<SyncGetnodeInfoReq>& msg, const MsgData& msgdata )
{
	// 判断版本是否兼容
	SyncHeaderMsg * HeaderMsg= msg->mutable_syncheadermsg();
	if( 0 != IsVersionCompatible( HeaderMsg->version() ) )
	{
		error("HandleSyncGetnodeInfoReq IsVersionCompatible");
		return ;
	}

	int db_status = 0;
	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if( txn == NULL )
	{
		error("HandleSyncGetnodeInfoReq TransactionInit");
		return;
	}

	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, false);
	};

	unsigned int blockHeight;
	std::string blockHash;
	db_status = pRocksDb->GetBlockTop(txn, blockHeight);
	if (db_status != 0) 
	{
		error("HandleSyncGetnodeInfoReq GetBlockTop");
		return;
	}

	db_status = pRocksDb->GetBestChainHash(txn, blockHash);
	if (db_status != 0) 
	{
		error("HandleSyncGetnodeInfoReq GetBestChainHash");
		return;
	}

	SyncGetnodeInfoAck syncGetnodeInfoAck;
	SetSyncHeaderMsg<SyncGetnodeInfoAck>(syncGetnodeInfoAck);

	syncGetnodeInfoAck.set_height(blockHeight);
	syncGetnodeInfoAck.set_hash(blockHash.substr(0,HASH_LEN));

	SyncHeaderMsg * headerMsg = msg->mutable_syncheadermsg();

	net_send_message<SyncGetnodeInfoAck>(headerMsg->id(), syncGetnodeInfoAck, true);
}

void HandleSyncGetnodeInfoAck( const std::shared_ptr<SyncGetnodeInfoAck>& msg, const MsgData& msgdata )
{
	// 判断版本是否兼容
	SyncHeaderMsg * pSyncHeaderMsg= msg->mutable_syncheadermsg();
	if( 0 != IsVersionCompatible( pSyncHeaderMsg->version() ) )
	{
		error("HandleSyncGetnodeInfoAck IsVersionCompatible");
		return ;
	}

	g_synch->SetPotentialNodes( pSyncHeaderMsg->id(), msg->height(), msg->hash() );
}

void HandleVerifyReliableNodeReq( const std::shared_ptr<VerifyReliableNodeReq>& msg, const MsgData& msgdata )
{
	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if( txn == NULL )
	{
		return;
	}

	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, false);
	};

	std::string ownid = net_get_self_node_id();

	VerifyReliableNodeAck verifyReliableNodeAck;

	verifyReliableNodeAck.set_id(ownid);
	verifyReliableNodeAck.set_height(msg->height());

    std::string hash;
    pRocksDb->GetBlockHashByBlockHeight(txn, msg->height(), hash);
    if(hash.size() == 0)
    {
        unsigned int top = 0;
        pRocksDb->GetBlockTop(txn, top);
        verifyReliableNodeAck.set_height(top);
    }
	verifyReliableNodeAck.set_hash(hash.substr(0,HASH_LEN));
	SyncHeaderMsg * headerMsg = msg->mutable_syncheadermsg();
	net_send_message<VerifyReliableNodeAck>(headerMsg->id(), verifyReliableNodeAck, true);
}


void HandleVerifyReliableNodeAck( const std::shared_ptr<VerifyReliableNodeAck>& msg, const MsgData& msgdata )
{
	std::lock_guard<std::mutex> lck(g_synch->mu_verifying_result);
    SyncNode data(msg->id(), msg->height(), msg->hash());
    g_synch->verifying_result.push_back(data);
}


void HandleSyncBlockInfoReq( const std::shared_ptr<SyncBlockInfoReq>& msg, const MsgData& msgdata )
{

	SyncHeaderMsg * pSyncHeaderMsg = msg->mutable_syncheadermsg();

	// 判断版本是否兼容
	if( 0 != IsVersionCompatible( pSyncHeaderMsg->version() ) )
	{
		error("HandleSyncBlockInfoReq:IsVersionCompatible");
		return ;
	}

	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, true);
	};

	int64_t other_height = msg->height();
	unsigned int ownblockHeight;
	pRocksDb->GetBlockTop(txn, ownblockHeight);
	int64_t end = ownblockHeight;
	if(ownblockHeight < msg->height())
	{
		std::cout<<"request height is over me,not sync." << std::endl;
		return ;	
	}


	SyncBlockInfoAck syncBlockInfoAck;
	SetSyncHeaderMsg<SyncBlockInfoAck>(syncBlockInfoAck);


	if(ownblockHeight > msg->height() )
	{
		std::string blocks = get_blkinfo_ser(other_height, end, msg->max_num());
		syncBlockInfoAck.set_blocks(blocks);
		syncBlockInfoAck.set_poolblocks(MagicSingleton<BlockPoll>::GetInstance()->GetBlock());
	}

	std::vector<CheckHash> v_checkhash = get_check_hash(other_height);
	if((int)v_checkhash.size() == (int)msg->checkhash_size())
	{
		std::vector<CheckHash> v_invalid_checkhash;
		for (int i = 0; i < msg->checkhash_size(); i++) {
			const CheckHash& checkhash = msg->checkhash(i);
			if(checkhash.begin() == v_checkhash[i].begin() 
				&& checkhash.end() == v_checkhash[i].end() 
				&& checkhash.hash() != v_checkhash[i].hash()
			)
			{
				v_invalid_checkhash.push_back(checkhash);
				CheckHash *invalid_checkhash = syncBlockInfoAck.add_invalid_checkhash();
				invalid_checkhash->set_begin(checkhash.begin());
				invalid_checkhash->set_end(checkhash.end());
			}
		}
		// if(v_invalid_checkhash.size() > 0 )
		// {
		// 	std::cout << "HandleSyncBlockInfoReq v_invalid_checkhash.size > 0" << std::endl;
		// 	for(auto i:v_invalid_checkhash)
		// 	{
		// 		std::cout << "begin:" << i.begin() << std::endl;
		// 		std::cout << "end:" << i.end() << std::endl;
		// 	}
		// }
	}else{
		error("checkhash.size not equal!! form:%d me:%d",(int)msg->checkhash_size(), (int)v_checkhash.size());
	}

	SyncHeaderMsg * headerMsg = msg->mutable_syncheadermsg();
	net_send_message<SyncBlockInfoAck>(headerMsg->id(), syncBlockInfoAck, true);	
}



void HandleSyncBlockInfoAck( const std::shared_ptr<SyncBlockInfoAck>& msg, const MsgData& msgdata )
{
	SyncHeaderMsg * pSyncHeaderMsg = msg->mutable_syncheadermsg();

	// 判断版本是否兼容
	if( 0 != IsVersionCompatible( pSyncHeaderMsg->version() ) )
	{
		error("HandleSyncBlockInfoAck:IsOverIt");
		return ;
	}

	if(MagicSingleton<BlockPoll>::GetInstance()->GetSyncBlocks().size() > 0)
	{
		error("HandleSyncBlockInfoAck:SyncBlocks not empty!"); 
		return;
	}


	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();

	bool bRollback = true;
	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, bRollback);
	};

	SyncHeaderMsg * headerMsg = msg->mutable_syncheadermsg();
	for (int i = 0; i < msg->invalid_checkhash_size(); i++) {
		const CheckHash& checkhash = msg->invalid_checkhash(i);
		std::cout << "invalid_checkhash==================:" << std::endl;
		std::cout << "begin:" << checkhash.begin() << std::endl;
		std::cout << "end:" << checkhash.end() << std::endl;
		std::cout << "invalid_checkhash===============end" << std::endl;

		SyncLoseBlockReq syncLoseBlockReq;
		SetSyncHeaderMsg<SyncLoseBlockReq>(syncLoseBlockReq);
		syncLoseBlockReq.set_begin(checkhash.begin());
		syncLoseBlockReq.set_end(checkhash.end());

		std::vector<std::string> hashs;
		for(auto i = checkhash.begin(); i <= checkhash.end(); i++)
		{
			std::vector<std::string> vBlockHashs;
			pRocksDb->GetBlockHashsByBlockHeight(txn, i, vBlockHashs); //i height
			std::for_each(vBlockHashs.begin(), vBlockHashs.end(),
				 [](std::string &s){ s = s.substr(0,HASH_LEN);}
			);

			hashs.push_back(StringUtil::concat(vBlockHashs, "_"));
		}
		syncLoseBlockReq.set_all_hash(StringUtil::concat(hashs, "_"));
		net_send_message<SyncLoseBlockReq>(headerMsg->id(), syncLoseBlockReq, true);	
		break;
	}

	//加块
	std::string blocks = msg->blocks();
	std::string poolblocks = msg->poolblocks();
	std::vector<std::string> v_blocks;
	std::vector<std::string> v_poolblocks;
	SplitString(blocks, v_blocks, "_");
	SplitString(poolblocks, v_poolblocks, "_");

	if(msg->invalid_checkhash_size() > 0 && v_blocks.size() > 0)
	{
		g_synch->conflict_height = msg->invalid_checkhash(0).begin();
		sleep(5);
	}
	if(msg->invalid_checkhash_size() == 0)
	{
		g_synch->conflict_height = -1;
	}


	for(size_t i = 0; i < v_blocks.size(); i++)
	{
		if(0 != SyncData(v_blocks[i],true))
		{
			return ;
		}
	}	
	for(size_t i = 0; i < v_poolblocks.size(); i++)
	{
		if(0 != SyncData(v_poolblocks[i]))
		{
			return ;
		}
	}	
}


void HandleSyncLoseBlockReq( const std::shared_ptr<SyncLoseBlockReq>& msg, const MsgData& msgdata )
{
	uint64_t begin = msg->begin();
	uint64_t end = msg->end();
	std::string all_hash = msg->all_hash();
	// std::cout << "begin:" << begin << std::endl;
	// std::cout << "end:" << end << std::endl;
	// std::cout << "all_hash:" << all_hash << std::endl;
	std::vector<std::string> v_hashs;
	SplitString(all_hash, v_hashs, "_");
	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();

	std::vector<std::string> ser_block;
	for(auto i = begin; i <= end; i++)
	{
		std::vector<std::string> vBlockHashs;
		pRocksDb->GetBlockHashsByBlockHeight(txn, i, vBlockHashs); //i height
		for(auto hash:vBlockHashs)
		{
			auto res = std::find(std::begin(v_hashs), std::end(v_hashs), hash.substr(0,HASH_LEN));
			if (res == std::end(v_hashs)) {
				std::cout << "HandleSyncLoseBlockReq hash:" << hash.substr(0,HASH_LEN) << std::endl;
				string strHeader;
				pRocksDb->GetBlockByBlockHash(txn, hash, strHeader);
				ser_block.push_back(Str2Hex(strHeader));
			}
		}
	}
	SyncLoseBlockAck syncLoseBlockAck;
	syncLoseBlockAck.set_blocks(StringUtil::concat(ser_block, "_"));
	SyncHeaderMsg * headerMsg = msg->mutable_syncheadermsg();
	net_send_message<SyncLoseBlockAck>(headerMsg->id(), syncLoseBlockAck, true);	
}

void HandleSyncLoseBlockAck( const std::shared_ptr<SyncLoseBlockAck>& msg, const MsgData& msgdata )
{
	if(MagicSingleton<BlockPoll>::GetInstance()->GetSyncBlocks().size() > 0)
	{
		return;
	}

	std::string blocks = msg->blocks();
	std::vector<std::string> v_blocks;
	SplitString(blocks, v_blocks, "_");

	for(size_t i = 0; i < v_blocks.size(); i++)
	{
		if(0 != SyncData(v_blocks[i],true))
		{
			return ;
		}
	}		
}

bool IsOverIt(int64_t height)
{
	int db_status = 0;
	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if( txn == NULL )
	{
		std::cout << "(ReqBlock) TransactionInit failed !" << std::endl;
		return false;
	}

	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, false);
	};

	unsigned int ownblockHeight;
	db_status = pRocksDb->GetBlockTop(txn, ownblockHeight);
    if (db_status != 0) 
	{
        return false;
    }
	if(ownblockHeight > height)
	{
		return true;
	}
	return false;
}

std::string get_blkinfo_ser(int64_t begin, int64_t end, int64_t max_num)
{
	max_num = max_num > SYNC_NUM_LIMIT ? SYNC_NUM_LIMIT : max_num;
    std::string bestblockHash;
	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if( txn == NULL )
	{
		std::cout << "(get_blkinfo_ser) TransactionInit failed !" << std::endl;
	}

	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, false);
	};
	begin = begin < 0 ? 0 : begin;
	end = end < 0 ? 0 : end;
	unsigned int top;
	pRocksDb->GetBlockTop(txn, top);	
	begin = begin > top ? top : begin;
	end = end > top ? top : end;

	std::vector<std::string> ser_block;
	int block_num = 0;
	for (auto i = begin; i <= end; i++) {
		if(block_num >= max_num)
		{
			break;
		}
        std::vector<std::string> vBlockHashs;
        pRocksDb->GetBlockHashsByBlockHeight(txn, i, vBlockHashs);
		block_num += vBlockHashs.size();
        for (auto hash : vBlockHashs) {
            string strHeader;
            pRocksDb->GetBlockByBlockHash(txn, hash, strHeader);
            ser_block.push_back(Str2Hex(strHeader));
		}
	}
    
    return StringUtil::concat(ser_block, "_");
}

std::vector<std::string> get_blkinfo(int64_t begin, int64_t end, int64_t max_num)
{
	max_num = max_num > SYNC_NUM_LIMIT ? SYNC_NUM_LIMIT : max_num;
    std::string bestblockHash;
	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if( txn == NULL )
	{
		std::cout << "(get_blkinfo_ser) TransactionInit failed !" << std::endl;
	}

	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, false);
	};
	begin = begin < 0 ? 0 : begin;
	end = end < 0 ? 0 : end;
	unsigned int top;
	pRocksDb->GetBlockTop(txn, top);	
	begin = begin > top ? top : begin;
	end = end > top ? top : end;

	std::vector<std::string> v_blocks;
	int block_num = 0;
	for (auto i = begin; i <= end; i++) {
		if(block_num >= max_num)
		{
			break;
		}
        std::vector<std::string> vBlockHashs;
        pRocksDb->GetBlockHashsByBlockHeight(txn, i, vBlockHashs);
		block_num += vBlockHashs.size();
        for (auto hash : vBlockHashs) {
            string strHeader;
            pRocksDb->GetBlockByBlockHash(txn, hash, strHeader);
            v_blocks.push_back(strHeader);
		}
	}
    
    return v_blocks;
}



int SyncData(std::string &headerstr, bool isSync)
{
	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if( txn == NULL )
	{
		std::cout << "(SyncData) TransactionInit failed !" << std::endl;
		return -1;
	}

	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, false);
	};

	headerstr = Hex2Str(headerstr);
	CBlock cblock;
	cblock.ParseFromString(headerstr);

	MagicSingleton<BlockPoll>::GetInstance()->Add(Block(cblock,isSync));
	return 0;
}