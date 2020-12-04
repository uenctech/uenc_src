
#include "ca_blockpool.h"
#include "ca_console.h"
#include "../include/logging.h"
#include "ca_transaction.h"
#include <algorithm>
#include <iterator>
#include <pthread.h>
#include "ca_global.h"
#include "ca_hexcode.h"
#include "ca_rocksdb.h"
#include "MagicSingleton.h"
#include "../include/ScopeGuard.h"
#include "ca_synchronization.h"
#include "ca_base58.h"

bool BlockPoll::CheckConflict(const CBlock& block1, const CBlock& block2)
{
	std::vector<std::string> v1;
	{
		CTransaction tx = block1.txs(0);
		for(int j = 0; j < tx.vin_size(); j++)
		{
			CTxin vin = tx.vin(j);
			std::string hash = vin.prevout().hash();
			if(hash.size() > 0)
			{
				v1.push_back(hash + "_" + GetBase58Addr(vin.scriptsig().pub()));
			}
		}
	}

	std::vector<std::string> v2;
	{
		CTransaction tx = block2.txs(0);
		for(int j = 0; j < tx.vin_size(); j++)
		{
			CTxin vin = tx.vin(j);
			std::string hash = vin.prevout().hash();
			if(hash.size() > 0)
			{
				v2.push_back(hash + "_" + GetBase58Addr(vin.scriptsig().pub()));
			}
		}
	}

    std::sort(v1.begin(), v1.end());
    std::sort(v2.begin(), v2.end());
 
    std::vector<std::string> v_intersection;
 
    std::set_intersection(v1.begin(), v1.end(),
                          v2.begin(), v2.end(),
                          std::back_inserter(v_intersection));


	/*
	if(v_intersection.size() > 0)
	{
		std::cout << "======CheckConflict======" << std::endl;
		std::cout << "v1: " << v1.size() << std::endl;
		for(auto n : v1)
		{
			std::cout << n << std::endl;
		}

		std::cout << "v2: "  << v2.size() << std::endl; 
		for(auto n : v2){
			std::cout << n << std::endl;	
		}

		std::cout << "intersection: " << v1.size() << std::endl;
		for(auto n : v_intersection)
		{
			std::cout << n << std::endl;
		}
		std::cout << "=========end==========" << std::endl;
	}
	*/
	return v_intersection.size() > 0 ;
}


bool BlockPoll::CheckConflict(const CBlock& block)
{
	for (auto it = blocks_.begin(); it != blocks_.end(); ++it) 
	{
		auto &curr_block = *it;
		bool ret = CheckConflict(curr_block.blockheader_, block);
		if(ret)   //有冲突
		{
			return true;
		}
	}
	return false;
}

bool BlockPoll::Add(const Block& block)
{
	std::lock_guard<std::mutex> lck(mu_block_);

	if(block.isSync_)
	{
		std::cout << "BlockPoll::Add sync=====height:" << block.blockheader_.height() << " hash:" << block.blockheader_.hash().substr(0,HASH_LEN) << std::endl;

		sync_blocks_.push_back(block);
		return false;
	}
	CBlock blockheader = block.blockheader_;

	if(!VerifyHeight(blockheader))
	{
		error("BlockPoll:VerifyHeight fail!!");
		return false;
	}

	//检查utxo是否有冲突
	for (auto it = blocks_.begin(); it != blocks_.end(); ++it) 
	{
		auto &curr_block = *it;
		bool ret = CheckConflict(curr_block.blockheader_, blockheader);
		if(ret)   //有冲突
		{
			error("BlockPoll::Add====has conflict");
			if(curr_block.blockheader_.time() < blockheader.time())   //预留块中的早
			{
				return false;
			}else{     //预留块中的晚
				it = blocks_.erase(it);
				blocks_.push_back(block);
				return true;
			}
		}
	}
	std::cout << "BlockPoll::Add=====height:" << blockheader.height() << " hash:" << blockheader.hash().substr(0,HASH_LEN) << std::endl;
	info("BlockPoll::Add:%s", block.blockheader_.hash().c_str());
	blocks_.push_back(block);
	return true;
}



void BlockPoll::Process()
{
	if(processing_)
	{
		std::cout << "BlockPoll::Process is processing_" << std::endl;
		return;
	}
	processing_ = true;
	std::lock_guard<std::mutex> lck(mu_block_);
	int success_num = 0;
	std::string frist_hash;
	int frist_hash_num = 0;
	if(sync_blocks_.size() > 0)
	{
		frist_hash = sync_blocks_.front().blockheader_.prevhash();
	}
	bool sync_add_success = false; 
	g_synch->sync_adding = false;

    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
    Transaction* txn = pRocksDb->TransactionInit();
	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, true);
	};

    for (auto it = sync_blocks_.begin(); it != sync_blocks_.end(); ++it) {
		g_synch->sync_adding = true;
        auto &block = *it;
		if(frist_hash == block.blockheader_.prevhash())
		{
			frist_hash_num++;
		}
		if(VerifyBlockHeader(block.blockheader_))
		{
			bool ret = AddBlock(block.blockheader_);
			if(ret)
			{
                /* 交易统计增加 */
                // 交易
                uint64_t counts{0};
                pRocksDb->GetTxCount(txn, counts);
                counts++;
                pRocksDb->SetTxCount(txn, counts);
                // 燃料费
                counts = 0;
                pRocksDb->GetGasCount(txn, counts);
                counts++;
                pRocksDb->SetGasCount(txn, counts);
                // 额外奖励
                counts = 0;
                pRocksDb->GetAwardCount(txn, counts);
                counts++;
                pRocksDb->SetAwardCount(txn, counts);

				sync_add_success = true;
				success_num++;
				ca_console successColor(kConsoleColor_Green, kConsoleColor_Black, true);
				printf("%sSync block: %s is successful! %s\n", successColor.color().c_str(), block.blockheader_.hash().c_str(), successColor.reset().c_str());
			}
		}
    }
	g_synch->sync_adding = false;
	if(sync_blocks_.size() > 0)
	{	
		std::cout << "frist_hash_num:" << frist_hash_num << std::endl;
	}
	int sync_cnt = Singleton<Config>::get_instance()->GetSyncDataCount();
	
	if((success_num + frist_hash_num) >= sync_cnt)
	{
		g_synch->DataSynch(g_synch->verifying_node.id);
		std::cout << "sync not voer, continue sync,id:" << g_synch->verifying_node.id << std::endl;
	}
	if(sync_blocks_.size() >= SYNC_ADD_FAIL_LIMIT && !sync_add_success)
	{
		sync_add_fail_times_++;
		std::cout << "sync_add_fail_times_:" << sync_add_fail_times_ << std::endl;
	}else if(sync_blocks_.size() > 0 && sync_add_success)
	{
		sync_add_fail_times_ = 0;
	}
	sync_blocks_.clear();
	if(sync_add_fail_times_ >= SYNC_ADD_FAIL_TIME)
	{
		error("BlockPoll::Process rollback");
		unsigned int top = 0;
		pRocksDb->GetBlockTop(txn, top);

		if(g_synch->conflict_height > -1 && (unsigned)g_synch->conflict_height < top)
		{
			RollbackToHeight(g_synch->conflict_height);
		}
		else
		{
			RollbackToHeight(top - ROLLBACK_HEIGHT);
		}
		sync_add_fail_times_ = 0;
	}
	
    for (auto it = blocks_.begin(); it != blocks_.end(); ) 
	{ 
        auto &block = *it;
		// std::cout << "stay time:" << time(NULL) - block.time_ << std::endl;
		if(time(NULL) - block.time_ >= PROCESS_TIME)
		{
			block.time_ = time(NULL);
			pending_block_.push_back(std::move(block));
			it = blocks_.erase(it);
		}else{
			++it; 
		}
    }

	if(pending_block_.size() == 0)
	{
		processing_ = false;
		return;
	}
	std::sort(pending_block_.begin(), pending_block_.end(), [](const Block & b1, const Block & b2){
		if (b1.blockheader_.height() < b2.blockheader_.height())
		{
			return true;
		}
		else
		{
			return false;
		}
	});
	// for(auto i:pending_block_)
	// {
	// 	//std::cout << "pending_block_:" << " height:" << i.blockheader_.height() << " hash:" << i.blockheader_.hash().substr(0,HASH_LEN)  << std::endl;
	// }

	// auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	// Transaction* txn = pRocksDb->TransactionInit();

    for (auto it = pending_block_.begin(); it != pending_block_.end(); ) 
	{
        auto &block = *it;
		std::string strTempHeader;
		pRocksDb->GetBlockByBlockHash(txn, block.blockheader_.hash(), strTempHeader);

		if (strTempHeader.size() != 0)
		{
			it = pending_block_.erase(it);
			continue;
		}

		std::string strPrevHeader;
		pRocksDb->GetBlockByBlockHash(txn, block.blockheader_.prevhash(), strPrevHeader);
		if (strPrevHeader.size() == 0)
		{
			// std::cout <<"process pending_block_ not have preblock" << std::endl;
			++it;
			continue;
		}
		//过期
		if(time(NULL) - block.time_ >= PENDING_EXPIRE_TIME)
		{
			std::cout <<"pending_block_  have expire" << std::endl;
			it = pending_block_.erase(it);
			continue;
		}

		if(VerifyBlockHeader(block.blockheader_))
		{
			bool ret = AddBlock(block.blockheader_);
			if(ret)
			{
                /* 交易统计增加 */
                // 交易
                uint64_t counts{0};
                pRocksDb->GetTxCount(txn, counts);
                counts++;
                pRocksDb->SetTxCount(txn, counts);
                // 燃料费
                counts = 0;
                pRocksDb->GetGasCount(txn, counts);
                counts++;
                pRocksDb->SetGasCount(txn, counts);
                // 额外奖励
                counts = 0;
                pRocksDb->GetAwardCount(txn, counts);
                counts++;
                pRocksDb->SetAwardCount(txn, counts);

				ca_console successColor(kConsoleColor_Green, kConsoleColor_Black, true);
				printf("%sAdd block: %s is successful! %s\n", successColor.color().c_str(), block.blockheader_.hash().c_str(), successColor.reset().c_str());
				info("Add block: %s is successful!", block.blockheader_.hash().c_str());
			}
		}
		it = pending_block_.erase(it);
    }

	processing_ = false;
}


std::string BlockPoll::GetBlock()
{
	std::lock_guard<std::mutex> lck(mu_block_);
	std::vector<std::string> ser_block;
	for (auto &block:blocks_) {
        ser_block.push_back(block.blockheader_.SerializeAsString());
	}
    
    size_t size = ser_block.size();
	std::string blocks;
    for(size_t i = 0; i < size; i++)
    {
        blocks += Str2Hex(ser_block[i]);
        blocks += "_";
    }
    return blocks;
}

bool BlockPoll::VerifyHeight(const CBlock& block)
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
	pRocksDb->GetBlockTop(txn, ownblockHeight);

	unsigned int preheight;
	db_status = pRocksDb->GetBlockHeightByBlockHash(txn, block.prevhash(), preheight);
    if (db_status == 0) {
		if(ownblockHeight > (preheight + 5) ){
			return false;
		}
    }

	return true;
}













