#include <iostream>
#include <thread>
#include <unistd.h>
#include <utility>
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
#include "../net/peer_node.h"
#include "../net/node_cache.h"
#include "../utils/base64.h"


/* Set the highest block information obtained from other nodes  */ 
bool Sync::SetPotentialNodes(const std::string &id, 
							const int64_t &height, 
							const std::string &hash, 
							const std::string & forwardHash, 
							const std::string & backwardHash)
{
	std::lock_guard<std::mutex> lck(mu_potential_nodes);
    if(id.size() == 0)
    {
        return false;
    }

    // Traverse existing data to prevent duplicate data writing 
    uint64_t size = this->potential_nodes.size();
    for(uint64_t i = 0; i < size; i++)
    {
        if(0 == id.compare(potential_nodes[i].id))
        {
            return false;
        }
    }
    struct SyncNode data = {id, height, hash, forwardHash, backwardHash};
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


int Sync::GetSyncInfo(std::vector<std::string> & reliables, int64_t & syncHeight)
{
	if (this->potential_nodes.size() == 0)
	{
		return -1;
	}

	std::map<int64_t, std::map<std::string, std::vector<std::string>>> height_hash_id;
	for (auto & node : potential_nodes)
	{
		std::vector<std::string> checkHashs;
		StringUtil::SplitString(node.backwardHash, checkHashs, "_");

		for (auto & hash : checkHashs)
		{
			CheckHash checkHash;
			checkHash.ParseFromString(base64Decode(hash));

			auto idIter = height_hash_id.find(checkHash.end());
			if (height_hash_id.end() != idIter)
			{
				auto hashIter = idIter->second.find(checkHash.hash());
				if (idIter->second.end() != hashIter)
				{
					hashIter->second.push_back(node.id);
				}
				else
				{
					std::map<std::string, std::vector<std::string>> & hash_id = height_hash_id[checkHash.end()];
					std::vector<std::string> ids = {node.id};
					hash_id.insert(make_pair(checkHash.hash(), ids));
				}
			}
			else
			{
				std::vector<std::string> ids = {node.id};
				std::map<std::string, std::vector<std::string>> hash_id;
				hash_id.insert(std::make_pair(checkHash.hash(), ids));
				height_hash_id.insert(make_pair(checkHash.end(), hash_id));
			}
		}
	}

	int most = SYNCNUM * 0.6;
	for (auto & i : height_hash_id)
	{
		auto hash_id = i.second;
		int64_t tmpHeight = i.first;
		if (tmpHeight > syncHeight)
		{
			int mostHashCount = 0;
			std::string mostHash;
			std::vector<std::string> ids;
			for(auto & j : hash_id)
			{
				if (j.second.size() > (size_t)mostHashCount && j.second.size() >= (size_t)most)
				{
					mostHashCount = j.second.size();
					mostHash = j.first;
					ids = j.second;
				}
			}

			if ((size_t)mostHashCount >= reliables.size())
			{
				syncHeight = tmpHeight;
				reliables = ids;
			}
		}
	}
	return 0;
}

int Sync::SyncDataFromPubNode()
{
	std::vector<Node> nodes = net_get_public_node();
	if (nodes.size() == 0)
	{
		return -1;
	}

	srand(time(NULL));
	int i = rand() % nodes.size();

	int syncNum = Singleton<Config>::get_instance()->GetSyncDataCount();
	DataSynch(nodes[i].id, syncNum);

	return 0;
}


// Sync start 
void Sync::Process()
{   
	if(sync_adding)
	{
		std::cout << "sync_adding..." << std::endl;
		return;
	}               
    std::cout << "\n======Sync block begin====== " << std::endl;
    potential_nodes.clear();
	verifying_node.id.clear();

	// Failed to find reliable nodes for 3 consecutive times, directly request synchronization from public network nodes 
	if (reliableCount >= 3 || rollbackCount >= 3)
	{
		SyncDataFromPubNode();
		if ( reliableCount != 0)
		{
			std::lock_guard<std::mutex> lock(reliableLock);
			reliableCount = 0;
		}

		if ( rollbackCount != 0 )
		{
			std::lock_guard<std::mutex> lock(rollbackLock);
			rollbackCount = 0;
		}
		return ;
	}

    // === 1.Looking for potential reliable nodes  ===
	std::vector<Node> nodeInfos;
	if (Singleton<PeerNode>::get_instance()->get_self_node().is_public_node)
	{
		nodeInfos = Singleton<PeerNode>::get_instance()->get_nodelist();
		cout<<"public node size() = "<<nodeInfos.size()<<endl;
	}
	else
	{
		nodeInfos = Singleton<NodeCache>::get_instance()->get_nodelist();
		cout<<"normal node size() = "<<nodeInfos.size()<<endl;
	}
   
   std::cout << "nodeInfos size() = " << nodeInfos.size() << std::endl;

	std::vector<std::string> nodes;
	for (const auto & nodeInfo : nodeInfos)
	{
		nodes.push_back(nodeInfo.id);
	}
    if(nodes.size() == 0)
    {
        error("sync block not have node!!");
		return;
    }
    int nodesum = std::min(SYNCNUM, (int)nodes.size());

    /* Randomly select nodes to ensure fairness */
    std::vector<std::string> sendid = randomNode(nodesum);
   	std::cout << "sync send is size :" << sendid.size() << std::endl;

	string proxyid = Singleton<Config>::get_instance()->GetProxyID();
	Type type = Singleton<Config>::get_instance()->GetProxyTypeStatus();
	bool ispublicid = Singleton<Config>::get_instance()->GetIsPublicNode();
	if((type == kMANUAL) && (!proxyid.empty()) &&(!ispublicid))
	{
		SendSyncGetnodeInfoReq(proxyid);
	}
	else if((type == kAUTOMUTIC)  && (!ispublicid))
	{
		g_localnode.clear();
		vector<Node> allnode = Singleton<PeerNode>::get_instance()->get_nodelist();//The holes need to be removed 
		string self_id = Singleton<Config>::get_instance()->GetKID().c_str();
		vector<Node> localnode;
		vector<Node> idsequence;
		
		for(auto &item :allnode)
		{
			if(item.conn_kind != BYHOLE && (!item.is_public_node ))
			{
				localnode.push_back(item);
			}
		}

		Node self = Singleton<PeerNode>::get_instance()->get_self_node();	
		if(!self.is_public_node)
		{
			localnode.push_back(self);
		}

		std::sort(localnode.begin(),localnode.end(),Compare(true));
		localnode.erase(unique(localnode.begin(), localnode.end()), localnode.end());

		uint64_t height = localnode[0].chain_height;
		for(auto &item:localnode)
		{
			if(item.chain_height == height)
			{
				idsequence.push_back(item);
			}
		}
		std::sort(idsequence.begin(),idsequence.end(),Compare(false));

		if(self_id.compare(idsequence[0].id))
		{
			g_localnode.push_back(idsequence[0]);
			SendSyncGetnodeInfoReq(g_localnode[0].id);
			cout<<"potential g_localnode[0].ip = "<<IpPort::ipsz(g_localnode[0].local_ip)<<"g_localnode[0].height = "<<g_localnode[0].chain_height<<endl;
		}	
	}
	
	for(auto& id : sendid)
	{ 
		SendSyncGetnodeInfoReq(id); //Random node node information request 
	}
	
    sleep(3);
    if(potential_nodes.size() == 0)
    {
        error("potential_nodes == 0");
		return;
    }

	std::vector<std::string> reliables;
	int64_t syncHeight = 0;
	if ( 0 != GetSyncInfo(reliables, syncHeight) )
	{
		return;
	}
	
	if (reliables.size() <= 1)
	{
		std::lock_guard<std::mutex> lock(reliableLock);
		reliableCount++;
		return ;
	}
	else
	{
		std::lock_guard<std::mutex> lock(reliableLock);
		reliableCount = 0;
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

	unsigned int top = 0;
	if (pRocksDb->GetBlockTop(txn, top))
	{
		return ;
	}
	int sync_cnt = Singleton<Config>::get_instance()->GetSyncDataCount();
	int syncNum = std::min(syncHeight - top, (int64_t)sync_cnt);
	
	for(auto const &i :reliables)
	{
		if((type == kMANUAL) && (!proxyid.empty()) && (!ispublicid))
		{
			if(!proxyid.compare(i))
			{
				ca_console infoColor(kConsoleColor_Red, kConsoleColor_Black, true);
				DataSynch(proxyid, syncNum);
				cout<<"proxyid ="<<proxyid<<endl;
				cout << infoColor.reset();
				return;
			}
		}
		else if ((type == kAUTOMUTIC)  && (!ispublicid))
		{
			if(g_localnode.size() == 0)
			{
				break;
			}
			string autoid = g_localnode[0].id;
			if (!autoid.compare(i))
			{
				ca_console infoColor(kConsoleColor_Red, kConsoleColor_Black, true);
				cout<<"autoid.ip = "<<IpPort::ipsz(g_localnode[0].local_ip)<<" "<<"autoid.height ="<<g_localnode[0].chain_height<<endl;
				cout << infoColor.reset();
				DataSynch(autoid, syncNum);
				return;
			}
		}
	}

	srand(time(NULL));
	int i = rand() % reliables.size();
	DataSynch(reliables[i], syncNum);
}


/* Initiate a synchronization request */
bool Sync::DataSynch(std::string id, const int syncNum)
{
    if(0 == id.size())
    {
		error("DataSynch:id is empty!!");
        return false;
    }
    SendSyncBlockInfoReq(id, syncNum);
    return true;
}

//============Block synchronization interaction protocol ================

void SendSyncGetnodeInfoReq(std::string id)
{
	if(id.size() == 0)
	{
		return;
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

	unsigned int top = 0;
	if (0 != pRocksDb->GetBlockTop(txn, top))
	{
		error("(HandleSyncVerifyPledgeNodeReq) GetBlockTop failed !");
		return;
	}

    SyncGetnodeInfoReq getBestchainInfoReq;
    SetSyncHeaderMsg<SyncGetnodeInfoReq>(getBestchainInfoReq);
	getBestchainInfoReq.set_height(top);
	getBestchainInfoReq.set_syncnum( Singleton<Config>::get_instance()->GetSyncDataCount() );
	net_send_message<SyncGetnodeInfoReq>(id, getBestchainInfoReq, net_com::Compress::kCompress_True);
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
		net_send_message<SyncVerifyPledgeNodeReq>(id, syncVerifyPledgeNodeReq, net_com::Compress::kCompress_True);
	}

	return 0;
}

void HandleSyncVerifyPledgeNodeReq( const std::shared_ptr<SyncVerifyPledgeNodeReq>& msg, const MsgData& msgdata )
{
	// Determine whether the version is compatible 
	SyncHeaderMsg * HeaderMsg= msg->mutable_syncheadermsg();
	if(0 != Util::IsVersionCompatible(HeaderMsg->version()))
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
	net_send_message<SyncVerifyPledgeNodeAck>(headerMsg->id(), syncVerifyPledgeNodeAck, net_com::Compress::kCompress_True);
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
	net_send_message<SyncGetPledgeNodeReq>(id, getBestchainInfoReq, net_com::Compress::kCompress_True);
}

void HandleSyncGetPledgeNodeReq( const std::shared_ptr<SyncGetPledgeNodeReq>& msg, const MsgData& msgdata )
{
	// Determine whether the version is compatible 
	SyncHeaderMsg * HeaderMsg= msg->mutable_syncheadermsg();
	if( 0 != Util::IsVersionCompatible( HeaderMsg->version() ) )
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
	net_send_message<SyncGetPledgeNodeAck>(headerMsg->id(), syncGetPledgeNodeAck, net_com::Compress::kCompress_True);
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

// Validate potentially reliable node requests 
void SendVerifyReliableNodeReq(std::string id, int64_t height)
{
	if(id.size() == 0)
	{
		return;
	}

    VerifyReliableNodeReq verifyReliableNodeReq;
	SetSyncHeaderMsg<VerifyReliableNodeReq>(verifyReliableNodeReq);

    verifyReliableNodeReq.set_height(height);

	net_send_message<VerifyReliableNodeReq>(id, verifyReliableNodeReq, net_com::Compress::kCompress_True);
}

int get_check_hash(const uint32_t begin, const uint32_t end, const uint32_t end_height, std::vector<CheckHash> & vCheckHash)
{
	if (begin > end)
	{
		return -1;
	}

	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if( txn == NULL )
	{
		return -2;
	}

	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, false);
	};

	std::vector<std::tuple<int, int>> check_range; 
	for(int i = begin; i < (int)end; i++)
	{
		check_range.push_back(std::make_tuple(i*CHECK_HEIGHT, (i+1)*CHECK_HEIGHT));
	}
	check_range.push_back(std::make_tuple(end*CHECK_HEIGHT, end_height));

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
		vCheckHash.push_back(checkhash);
    }
	return 0;
}

std::vector<CheckHash> get_check_hash_backward(const int height, const int num)
{
	std::vector<CheckHash> v_checkhash;

	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if ( txn == NULL )
	{
		return v_checkhash;
	}

	bool bRollback = true;
	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, bRollback);
	};

	unsigned int top = 0;
	if (pRocksDb->GetBlockTop(txn, top))
	{
		return v_checkhash;
	}

	int check_begin = height / CHECK_HEIGHT;
	int end_height = std::min((height + num), (int)top);
	int check_end = end_height / CHECK_HEIGHT;

	get_check_hash(check_begin, check_end, end_height, v_checkhash);
	return v_checkhash;
}

std::vector<CheckHash> get_check_hash_forward(int height)
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

	get_check_hash(check_begin, check_end, height, v_checkhash);
	return v_checkhash;
}

void  SendSyncBlockInfoReq(std::string id, const int syncNum)
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
	syncBlockInfoReq.set_max_height(blockHeight + syncNum);
	
	std::vector<CheckHash> v_checkhash = get_check_hash_forward(blockHeight);
	for(auto i:v_checkhash)
	{
		CheckHash *checkhash = syncBlockInfoReq.add_checkhash();
		checkhash->set_begin(i.begin());
		checkhash->set_end(i.end());
		checkhash->set_hash(i.hash());
	}
	net_send_message<SyncBlockInfoReq>(id, syncBlockInfoReq, net_com::Compress::kCompress_True);
}


std::vector<std::string> CheckHashToString(const std::vector<CheckHash> & v)
{
	std::vector<std::string> vRet;
	for (const auto & i : v)
	{
		vRet.push_back(base64Encode(i.SerializeAsString()));
	}

	return vRet;
}


void HandleSyncGetnodeInfoReq( const std::shared_ptr<SyncGetnodeInfoReq>& msg, const MsgData& msgdata )
{
	// Determine whether the version is compatible 
	SyncHeaderMsg * HeaderMsg= msg->mutable_syncheadermsg();
	if( 0 != Util::IsVersionCompatible( HeaderMsg->version() ) )
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

	std::vector<CheckHash> vCheckHashForward = get_check_hash_forward(msg->height());
	std::vector<CheckHash> vCheckHashbackward = get_check_hash_backward(msg->height(), msg->syncnum());
	std::vector<std::string> vStrForward = CheckHashToString(vCheckHashForward);
	std::vector<std::string> vStrBackward = CheckHashToString(vCheckHashbackward);

	syncGetnodeInfoAck.set_height(blockHeight);
	syncGetnodeInfoAck.set_hash(blockHash.substr(0,HASH_LEN));
	syncGetnodeInfoAck.set_checkhashforward( StringUtil::concat(vStrForward, "_") );
	syncGetnodeInfoAck.set_checkhashbackward( StringUtil::concat(vStrBackward, "_") );

	SyncHeaderMsg * headerMsg = msg->mutable_syncheadermsg();
	net_send_message<SyncGetnodeInfoAck>(headerMsg->id(), syncGetnodeInfoAck, net_com::Compress::kCompress_True);
}

void HandleSyncGetnodeInfoAck( const std::shared_ptr<SyncGetnodeInfoAck>& msg, const MsgData& msgdata )
{
	// Determine whether the version is compatible 
	SyncHeaderMsg * pSyncHeaderMsg= msg->mutable_syncheadermsg();
	if( 0 != Util::IsVersionCompatible( pSyncHeaderMsg->version() ) )
	{
		error("HandleSyncGetnodeInfoAck IsVersionCompatible");
		return ;
	}

	g_synch->SetPotentialNodes( pSyncHeaderMsg->id(), msg->height(), msg->hash(), msg->checkhashforward(), msg->checkhashbackward() );
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
	net_send_message<VerifyReliableNodeAck>(headerMsg->id(), verifyReliableNodeAck, net_com::Compress::kCompress_True);
}


void HandleSyncBlockInfoReq( const std::shared_ptr<SyncBlockInfoReq>& msg, const MsgData& msgdata )
{

	SyncHeaderMsg * pSyncHeaderMsg = msg->mutable_syncheadermsg();
	
	// Determine whether the version is compatible 
	if( 0 != Util::IsVersionCompatible( pSyncHeaderMsg->version() ) )
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

	unsigned int max_height = msg->max_height();           // The maximum height of the request node to be synchronized 
	int64_t end = std::min(ownblockHeight, max_height);    // Sync end height 
	
	if(ownblockHeight < msg->height())
	{
		std::cout<<"request height is over me,not sync." << std::endl;
		return ;	
	}

	SyncBlockInfoAck syncBlockInfoAck;
	SetSyncHeaderMsg<SyncBlockInfoAck>(syncBlockInfoAck);

	std::vector<CheckHash> v_checkhash = get_check_hash_forward(other_height);
	if((int)v_checkhash.size() == (int)msg->checkhash_size())
	{
		std::vector<CheckHash> v_invalid_checkhash;
		for (int i = 0; i < msg->checkhash_size(); i++) 
		{
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
	}
	else
	{
		error("checkhash.size not equal!! form:%d me:%d",(int)msg->checkhash_size(), (int)v_checkhash.size());
	}

	int beginHeight = 0;
	if (syncBlockInfoAck.invalid_checkhash_size() != 0)
	{
		beginHeight = syncBlockInfoAck.invalid_checkhash(0).begin();
	}
	else
	{
		beginHeight = msg->height();
	}
	
	if(ownblockHeight > msg->height() )
	{
		std::string blocks = get_blkinfo_ser(beginHeight, end, msg->max_num());
		syncBlockInfoAck.set_blocks(blocks);
		syncBlockInfoAck.set_poolblocks(MagicSingleton<BlockPoll>::GetInstance()->GetBlock());
	}

	SyncHeaderMsg * headerMsg = msg->mutable_syncheadermsg();
	net_send_message<SyncBlockInfoAck>(headerMsg->id(), syncBlockInfoAck, net_com::Compress::kCompress_True);	
}



void HandleSyncBlockInfoAck( const std::shared_ptr<SyncBlockInfoAck>& msg, const MsgData& msgdata )
{
	SyncHeaderMsg * pSyncHeaderMsg = msg->mutable_syncheadermsg();
	string id = pSyncHeaderMsg->id();
	cout<<"id = "<<id <<endl;
	Node node;
	Singleton<PeerNode>::get_instance()->find_node(id, node);
	cout<<"HandleSyncBlockInfoAck ip = "<<IpPort::ipsz(node.local_ip)<<"syncBlockInfoReq height =" <<node.chain_height<<endl;
	// Determine whether the version is compatible 
	if( 0 != Util::IsVersionCompatible( pSyncHeaderMsg->version() ) )
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
	for (int i = 0; i < msg->invalid_checkhash_size(); i++) 
	{
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
		net_send_message<SyncLoseBlockReq>(headerMsg->id(), syncLoseBlockReq, net_com::Compress::kCompress_True);	
		break;
	}

	//Add block 
	std::string blocks = msg->blocks();
	std::string poolblocks = msg->poolblocks();
	std::vector<std::string> v_blocks;
	std::vector<std::string> v_poolblocks;
	SplitString(blocks, v_blocks, "_");
	SplitString(poolblocks, v_poolblocks, "_");

	if(msg->invalid_checkhash_size() > 0 )
	{
		int begin = msg->invalid_checkhash(0).begin() == 0 ? 1 : msg->invalid_checkhash(0).begin();
		g_synch->conflict_height = begin;
		// sleep(5);
	}
	// if(msg->invalid_checkhash_size() == 0)
	// {
	// 	g_synch->conflict_height = -1;
	// }


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
	sleep(1);
	g_synch->conflict_height = -1;
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

	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, false);
	};

	std::vector<std::string> ser_block;
	for(auto i = begin; i <= end; i++)
	{
		std::vector<std::string> vBlockHashs;
		pRocksDb->GetBlockHashsByBlockHeight(txn, i, vBlockHashs); //i height
		for(auto hash:vBlockHashs)
		{
			// auto res = std::find(std::begin(v_hashs), std::end(v_hashs), hash.substr(0,HASH_LEN));
			// if (res == std::end(v_hashs)) {
			// 	std::cout << "HandleSyncLoseBlockReq hash:" << hash.substr(0,HASH_LEN) << std::endl;
			// 	string strHeader;
			// 	pRocksDb->GetBlockByBlockHash(txn, hash, strHeader);
			// 	ser_block.push_back(Str2Hex(strHeader));
			// }
			string strBlock;
			if ( 0 != pRocksDb->GetBlockByBlockHash(txn, hash, strBlock) ) 
			{
				return ;
			}
			ser_block.push_back(Str2Hex(strBlock));
		}
	}
	SyncLoseBlockAck syncLoseBlockAck;
	syncLoseBlockAck.set_blocks(StringUtil::concat(ser_block, "_"));
	SyncHeaderMsg * headerMsg = msg->mutable_syncheadermsg();
	net_send_message<SyncLoseBlockAck>(headerMsg->id(), syncLoseBlockAck, net_com::Compress::kCompress_True);	
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
	for (auto i = begin; i <= end; i++) 
	{
		if(block_num >= max_num)
		{
			break;
		}
        std::vector<std::string> vBlockHashs;
        pRocksDb->GetBlockHashsByBlockHeight(txn, i, vBlockHashs);
		block_num += vBlockHashs.size();
        for (auto hash : vBlockHashs) 
		{
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
	for (auto i = begin; i <= end; i++) 
	{
		if(block_num >= max_num)
		{
			break;
		}
        std::vector<std::string> vBlockHashs;
        pRocksDb->GetBlockHashsByBlockHeight(txn, i, vBlockHashs);
		block_num += vBlockHashs.size();
        for (auto hash : vBlockHashs) 
		{
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
