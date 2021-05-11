#include <iostream>
#include <memory>
#include <sys/time.h>

#include "ca_rollback.h"
#include "ca_console.h"
#include "ca_txhelper.h"
#include "ca_transaction.h"

#include "MagicSingleton.h"
#include "../include/ScopeGuard.h"
#include "../utils/json.hpp"

#include "block.pb.h"



int Rollback::RollbackRedeemTx(std::shared_ptr<Rocksdb> pRocksDb, Transaction* txn, CTransaction &tx)
{
	ca_console ResBlockColor(kConsoleColor_Green, kConsoleColor_Black, true);

	nlohmann::json txExtra = nlohmann::json::parse(tx.extra());
	nlohmann::json txInfo = txExtra["TransactionInfo"].get<nlohmann::json>();
	std::string redempUtxoStr = txInfo["RedeemptionUTXO"].get<std::string>();

	// 取出交易发起方，解质押交易只有一个发起方和接收方
	std::vector<std::string> owner_addrs = TxHelper::GetTxOwner(tx);
	std::string txOwner = owner_addrs[0];

	if(CheckTransactionType(tx) == kTransactionType_Tx)
	{
		uint64_t pledgeValue = 0;

		std::string txRaw;
		if (0 != pRocksDb->GetTransactionByHash(txn, redempUtxoStr, txRaw) )
		{
			printf("%sGetTransactionByHash failed !!! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
			return -1;
		}

		CTransaction utxoTx;
		utxoTx.ParseFromString(txRaw);

		for (int j = 0; j < utxoTx.vout_size(); j++)
		{
			CTxout txOut = utxoTx.vout(j);
			if (txOut.scriptpubkey() == VIRTUAL_ACCOUNT_PLEDGE)
			{
				pledgeValue += txOut.value();
			}
		}

		// 回滚vin
		std::vector<std::string> vinUtxos;
		uint64_t vinAmountTotal = 0;
		uint64_t voutAmountTotal = 0;
		for (auto & txin : tx.vin())
		{
			vinUtxos.push_back(txin.prevout().hash());
		}

		auto iter = find(vinUtxos.begin(), vinUtxos.end(), redempUtxoStr);
		if (iter != vinUtxos.end())
		{
			vinUtxos.erase(iter);
		}
		else
		{
			printf("%s Find redempUtxoStr in vinUtxos failed ! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
			return -2;
		}
		
		for (auto & vinUtxo : vinUtxos)
		{
			vinAmountTotal += TxHelper::GetUtxoAmount(vinUtxo, txOwner);
		}
		
		for (auto & txout : tx.vout())
		{
			voutAmountTotal += txout.value();
		}

		// 判断解质押交易的vin中是否有质押产生的正常utxo部分
		nlohmann::json extra = nlohmann::json::parse(tx.extra());
		uint64_t signFee = extra["SignFee"].get<int>();
		uint64_t NeedVerifyPreHashCount = extra["NeedVerifyPreHashCount"].get<int>();
		uint64_t packageFee = extra["PackageFee"].get<int>();

		voutAmountTotal += signFee * (NeedVerifyPreHashCount - 1);
		voutAmountTotal += packageFee;

		bool bIsUnused = true;
		if (voutAmountTotal != vinAmountTotal)
		{
			uint64_t usable = TxHelper::GetUtxoAmount(redempUtxoStr, txOwner);
			if (voutAmountTotal == vinAmountTotal - usable)
			{
				// 本交易未使用质押utxo的正常部分
				bIsUnused = false;
			}
		}

		for (auto & txin : tx.vin())
		{
			if (txin.prevout().hash() == redempUtxoStr && bIsUnused)
			{
				continue;
			}
			
			if ( 0 != pRocksDb->SetUtxoHashsByAddress(txn, txOwner, txin.prevout().hash()) )
			{
				std::string txRaw;
				if ( 0 != pRocksDb->GetTransactionByHash(txn, txin.prevout().hash(), txRaw) )
				{
					printf("%sGetTransactionByHash failed !!! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
					return -2;
				}

				CTransaction vinUtxoTx;
				vinUtxoTx.ParseFromString(txRaw);

				nlohmann::json extra = nlohmann::json::parse(vinUtxoTx.extra());
				std::string txType = extra["TransactionType"].get<std::string>();
				if (txType != TXTYPE_REDEEM)
				{
					printf("%sSetUtxoHashsByAddress failed !!! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
					return -3;
				}
			}
		}

		int64_t value = 0;
		if ( 0 != pRocksDb->GetBalanceByAddress(txn, txOwner, value) )
		{
			printf("%sGetBalanceByAddress failed !!! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
			return -2;
		}

		int64_t amount = 0;
		amount = value - pledgeValue;

		// 回滚余额
		if ( 0 != pRocksDb->SetBalanceByAddress(txn, txOwner, amount) )
		{
			printf("%sSetBalanceByAddress failed !!! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
			return -3;
		}

		// 删除交易记录
		if ( 0 != pRocksDb->RemoveAllTransactionByAddress(txn, txOwner, tx.hash()) )
		{
			printf("%sRemoveAllTransactionByAddress failed !!! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
			return -4;
		}

		// 放回解质押的utxo
		if (0 != pRocksDb->SetPledgeAddressUtxo(txn, txOwner, redempUtxoStr) )
		{
			printf("%sSetPledgeAddressUtxo failed !!! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
			return -5;
		}

		// 放回解质押的地址
		std::vector<std::string> utxoes;
		if (0 != pRocksDb->GetPledgeAddressUtxo(txn, txOwner, utxoes))
		{
			printf("%sGetPledgeAddressUtxo failed !!! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
			return -6;
		}

		// 如果是刚放入的utxo，则说明回滚前无质押地址，需要放回去
		if (utxoes.size() == 1)
		{
			if (0 != pRocksDb->SetPledgeAddresses(txn, txOwner))
			{
				printf("%sSetPledgeAddresses failed !!! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
				return -7;
			}
		}

		if ( 0 != pRocksDb->RemoveUtxoHashsByAddress(txn, txOwner, tx.hash()))
		{
			printf("%sRemoveUtxoHashsByAddress failed !!! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
			return -8;
		}
	}
	else if (CheckTransactionType(tx) == kTransactionType_Fee || CheckTransactionType(tx) == kTransactionType_Award)
	{
		uint64_t signFee = 0;
		std::string txOwnerAddr;
		std::string txRaw;
		if (0 != pRocksDb->GetTransactionByHash(txn, redempUtxoStr, txRaw) )
		{
			printf("%sGetTransactionByHash failed !!! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
			return -9;
		}

		CTransaction utxoTx;
		utxoTx.ParseFromString(txRaw);

		for (int j = 0; j < utxoTx.vout_size(); j++)
		{
			CTxout txOut = utxoTx.vout(j);
			if (txOut.scriptpubkey() != VIRTUAL_ACCOUNT_PLEDGE)
			{
				txOwnerAddr += txOut.scriptpubkey();
			}
		}

		for (int j = 0; j < tx.vout_size(); j++)
		{
			CTxout txout = tx.vout(j);

			if (txout.scriptpubkey() != txOwnerAddr)
			{
				signFee += txout.value();
			}

			int64_t value = 0;
			if ( 0 != pRocksDb->GetBalanceByAddress(txn, txout.scriptpubkey(), value) )
			{
				printf("%sGetBalanceByAddress  3  failed !!! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
				return -10;
			}
			int64_t amount = value - txout.value();
			if ( 0 != pRocksDb->SetBalanceByAddress(txn, txout.scriptpubkey(), amount) )
			{
				printf("%sSetBalanceByAddress  3  failed !!! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
				return -11;
			}
			if ( 0 != pRocksDb->RemoveAllTransactionByAddress(txn, txout.scriptpubkey(), tx.hash()) )
			{
				printf("%sRemoveAllTransactionByAddress  5  failed !!! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
				return -12;
			}
			if ( 0 != pRocksDb->RemoveUtxoHashsByAddress(txn, txout.scriptpubkey(), tx.hash()) )
			{
				printf("%sRemoveUtxoHashsByAddress  2  failed !!! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
				return -13;
			}
		}

		if (CheckTransactionType(tx) == kTransactionType_Fee)
		{
			int64_t value = 0;
			if ( 0 != pRocksDb->GetBalanceByAddress(txn, txOwnerAddr, value) )
			{
				printf("%sGetBalanceByAddress  3  failed !!! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
				return -14;
			}

			signFee += value;

			if ( 0 != pRocksDb->SetBalanceByAddress(txn, txOwnerAddr, signFee) )
			{
				printf("%sSetBalanceByAddress  3  failed !!! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
				return -15;
			}
		}
	}

	return 0;
}



int Rollback::RollbackPledgeTx(std::shared_ptr<Rocksdb> pRocksDb, Transaction* txn, CTransaction &tx)
{
	int db_status = 0;
	std::vector<std::string> owner_addrs = TxHelper::GetTxOwner(tx);
	ca_console ResBlockColor(kConsoleColor_Green, kConsoleColor_Black, true);

	// 取质押账户
	std::string addr;
	if (owner_addrs.size() != 0)
	{
		addr = owner_addrs[0]; 
	}
	
	if (CheckTransactionType(tx) == kTransactionType_Tx) 
	{
		if (0 !=  pRocksDb->RemovePledgeAddressUtxo(txn, addr, tx.hash()))
		{
			return -33;
		}

		std::vector<std::string> utxoes;
		pRocksDb->GetPledgeAddressUtxo(txn, addr, utxoes); // 无需判断
		
		if (utxoes.size() == 0)
		{
			if (0 != pRocksDb->RemovePledgeAddresses(txn, addr))
			{
				return -34;
			}
		}

		//vin加
		for (int j = 0; j < tx.vin_size(); j++)
		{
			CTxin txin = tx.vin(j);
			std::string vin_hash = txin.prevout().hash();  //花费的vin
			std::string vin_owner = GetBase58Addr(txin.scriptsig().pub());

			if ( 0 != pRocksDb->SetUtxoHashsByAddress(txn, vin_owner, vin_hash ))
			{
				// vin 重复时，若不是解质押产生的utxo，则返回
				std::string txRaw;
				if ( 0 != pRocksDb->GetTransactionByHash(txn, vin_hash, txRaw) )
				{
					printf("%sGetTransactionByHash  failed !!! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
					return -17;
				}

				CTransaction vinHashTx;
				vinHashTx.ParseFromString(txRaw);

				nlohmann::json extra = nlohmann::json::parse(vinHashTx.extra());
				std::string txType = extra["TransactionType"];

				if (txType != TXTYPE_REDEEM)
				{
					printf("%sSetUtxoHashsByAddress  failed !!! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
					return -17;
				}
				else
				{
					continue;
				}	
			}

			//vin加余额
			uint64_t amount = TxHelper::GetUtxoAmount(vin_hash, vin_owner);
			int64_t balance = 0;

			db_status = pRocksDb->GetBalanceByAddress(txn, vin_owner, balance);
			if (db_status != 0) 
			{
				error("AddBlock:GetBalanceByAddress");
			}

			balance += amount;
			db_status = pRocksDb->SetBalanceByAddress(txn, vin_owner, balance);
			if (db_status != 0) 
			{
				return -18;
			}
		}

		//vout减
		for (int j = 0; j < tx.vout_size(); j++)
		{
			CTxout txout = tx.vout(j);
			// if(std::find(std::begin(owner_addrs), std::end(owner_addrs), txout.scriptpubkey()) == std::end(owner_addrs))
			
			int64_t value = 0;
			if (txout.scriptpubkey() != VIRTUAL_ACCOUNT_PLEDGE)
			{
				if ( 0 != pRocksDb->GetBalanceByAddress(txn, txout.scriptpubkey(), value) )
				{
					printf("%sGetBalanceByAddress  3  failed !!! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
					return -30;
				}
				int64_t amount = value - txout.value();
				if ( 0 != pRocksDb->SetBalanceByAddress(txn, txout.scriptpubkey(), amount) )
				{
					printf("%sSetBalanceByAddress  3  failed !!! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
					return -31;
				}

				if ( 0 != pRocksDb->RemoveUtxoHashsByAddress(txn, txout.scriptpubkey(), tx.hash()))
				{
					printf("%sRemoveUtxoHashsByAddress failed !!! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
					return -15;
				}
				if ( 0 != pRocksDb->RemoveAllTransactionByAddress(txn, txout.scriptpubkey(), tx.hash()) )
				{
					printf("%sRemoveAllTransactionByAddress  5  failed !!! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
					return -32;
				}
			}
		}
	}
	else if (CheckTransactionType(tx) == kTransactionType_Fee || CheckTransactionType(tx) == kTransactionType_Award)
	{
		for (int j = 0; j < tx.vout_size(); j++)
		{
			CTxout txout = tx.vout(j);
			int64_t value = 0;
			if ( 0 != pRocksDb->GetBalanceByAddress(txn, txout.scriptpubkey(), value) )
			{
				printf("%sGetBalanceByAddress  3  failed !!! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
				return -30;
			}
			int64_t amount = value - txout.value();
			if ( 0 != pRocksDb->SetBalanceByAddress(txn, txout.scriptpubkey(), amount) )
			{
				printf("%sSetBalanceByAddress  3  failed !!! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
				return -31;
			}
			if ( 0 != pRocksDb->RemoveAllTransactionByAddress(txn, txout.scriptpubkey(), tx.hash()) )
			{
				printf("%sRemoveAllTransactionByAddress  5  failed !!! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
				return -32;
			}
			if ( 0 != pRocksDb->RemoveUtxoHashsByAddress(txn, txout.scriptpubkey(), tx.hash()) )
			{
				printf("%sRemoveUtxoHashsByAddress  2  failed !!! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
				return -18;
			}
		}
	}

	return 0;
}


int Rollback::RollbackTx(std::shared_ptr<Rocksdb> pRocksDb, Transaction* txn, CTransaction tx)
{
	ca_console ResBlockColor(kConsoleColor_Green, kConsoleColor_Black, true);
	int db_status = 0;
	std::vector<std::string> owner_addrs = TxHelper::GetTxOwner(tx);
	if(CheckTransactionType(tx) == kTransactionType_Tx) 
	{
		for (auto & addr : owner_addrs)
		{
			if ( 0 != pRocksDb->RemoveAllTransactionByAddress(txn, addr, tx.hash()) )
			{
				printf("%sRemoveAllTransactionByAddress failed !!! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
				return -1;
			}
		}
		//vin加
		for (int j = 0; j < tx.vin_size(); j++)
		{
			CTxin txin = tx.vin(j);
			std::string vin_hash = txin.prevout().hash();  //花费的vin
			std::string vin_owner = GetBase58Addr(txin.scriptsig().pub());

			if ( 0 != pRocksDb->SetUtxoHashsByAddress(txn, vin_owner, vin_hash ))
			{
				// vin 重复时，若不是解质押产生的utxo，则返回
				std::string txRaw;
				if ( 0 != pRocksDb->GetTransactionByHash(txn, vin_hash, txRaw) )
				{
					printf("%sGetTransactionByHash  failed !!! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
					return -2;
				}

				CTransaction vinHashTx;
				vinHashTx.ParseFromString(txRaw);

				nlohmann::json extra = nlohmann::json::parse(vinHashTx.extra());
				std::string txType = extra["TransactionType"];

				if (txType != TXTYPE_REDEEM)
				{
					printf("%sSetUtxoHashsByAddress  failed !!! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
					return -3;
				}
				else
				{
					continue;
				}	
			}

			//vin加余额
			uint64_t amount = TxHelper::GetUtxoAmount(vin_hash, vin_owner);
			int64_t balance = 0;
			db_status = pRocksDb->GetBalanceByAddress(txn, vin_owner, balance);
			if (db_status != 0) 
			{
				error("AddBlock:GetBalanceByAddress");
			}
			balance += amount;
			db_status = pRocksDb->SetBalanceByAddress(txn, vin_owner, balance);
			if (db_status != 0) 
			{
				return -4;
			}
		}
		//vout减
		for (int j = 0; j < tx.vout_size(); j++)
		{
			CTxout txout = tx.vout(j);
			
			int64_t value = 0;
			if ( 0 != pRocksDb->GetBalanceByAddress(txn, txout.scriptpubkey(), value) )
			{
				printf("%sGetBalanceByAddress  3  failed !!! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
				return -5;
			}
			int64_t amount = value - txout.value();
			if ( 0 != pRocksDb->SetBalanceByAddress(txn, txout.scriptpubkey(), amount) )
			{
				printf("%sSetBalanceByAddress  3  failed !!! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
				return -6;
			}

			if ( 0 != pRocksDb->RemoveUtxoHashsByAddress(txn, txout.scriptpubkey(), tx.hash()))
			{
				printf("%sRemoveUtxoHashsByAddress failed !!! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
				return -7;
			}

			// 交易接收方交易记录
			if (owner_addrs.end() == find(owner_addrs.begin(), owner_addrs.end(), txout.scriptpubkey()))
			{
				if ( 0 != pRocksDb->RemoveAllTransactionByAddress(txn, txout.scriptpubkey(), tx.hash()) )
				{
					printf("%sRemoveAllTransactionByAddress  5  failed !!! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
					return -8;
				}
			}
		}					
	}	
	else if (CheckTransactionType(tx) == kTransactionType_Fee || CheckTransactionType(tx) == kTransactionType_Award)
	{
		for (int j = 0; j < tx.vout_size(); j++)
		{
			CTxout txout = tx.vout(j);
			int64_t value = 0;
			if ( 0 != pRocksDb->GetBalanceByAddress(txn, txout.scriptpubkey(), value) )
			{
				printf("%sGetBalanceByAddress  3  failed !!! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
				return -9;
			}
			int64_t amount = value - txout.value();
			if ( 0 != pRocksDb->SetBalanceByAddress(txn, txout.scriptpubkey(), amount) )
			{
				printf("%sSetBalanceByAddress  3  failed !!! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
				return -10;
			}
			if ( 0 != pRocksDb->RemoveAllTransactionByAddress(txn, txout.scriptpubkey(), tx.hash()) )
			{
				printf("%sRemoveAllTransactionByAddress  5  failed !!! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
				return -11;
			}
			if ( 0 != pRocksDb->RemoveUtxoHashsByAddress(txn, txout.scriptpubkey(), tx.hash()) )
			{
				printf("%sRemoveUtxoHashsByAddress  2  failed !!! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
				return -12;
			}
		}
	}
	return 0;
}



int Rollback::RollbackBlockByBlockHash(Transaction* txn, std::shared_ptr<Rocksdb> & pRocksDb, const std::string & blockHash)
{
	std::lock_guard<std::mutex> lck(mutex_);

    // 日志颜色
    ca_console ResBlockColor(kConsoleColor_Red, kConsoleColor_Black, true);

	isRollbacking = true;
    ON_SCOPE_EXIT{
		isRollbacking = false;
    };

    /* 交易数据统计回滚 */
    // 交易
    uint64_t counts{0};
    pRocksDb->GetTxCount(txn, counts);
    counts--;
    pRocksDb->SetTxCount(txn, counts);
    // 燃料费
    counts = 0;
    pRocksDb->GetGasCount(txn, counts);
    counts--;
    pRocksDb->SetGasCount(txn, counts);
    // 额外奖励
    counts = 0;
    pRocksDb->GetAwardCount(txn, counts);
    counts--;
    pRocksDb->SetAwardCount(txn, counts);

    uint32_t top = 0;
    if (pRocksDb->GetBlockTop(txn, top))
    {
        printf("%sGetBlockTop failed !!! %s\n", ResBlockColor.color().c_str(), ResBlockColor.reset().c_str());
        return -2;
    }

    std::string serBlockHeader;
    if ( 0 != pRocksDb->GetBlockByBlockHash(txn, blockHash, serBlockHeader) )
    {
        printf("%sGetBlockHeaderByBlockHash failed !!! %s\n", ResBlockColor.color().c_str(), ResBlockColor.reset().c_str());
        return -3;
    }

    CBlock cblock;
    cblock.ParseFromString(serBlockHeader);

    std::vector<std::string> blockHashs;
    if (0 != pRocksDb->GetBlockHashsByBlockHeight(txn, cblock.height(), blockHashs))
    {
        printf("%sGetBlockHashsByBlockHeight failed !!! %s\n", ResBlockColor.color().c_str(), ResBlockColor.reset().c_str());
        return -4;
    }

    std::string bestChainHash;
    if ( 0 != pRocksDb->GetBestChainHash(txn, bestChainHash) )
    {
        printf("%sGetBestChainHash failed !!! %s\n", ResBlockColor.color().c_str(), ResBlockColor.reset().c_str());
        return -5;
    }

	std::string prevBestChainHash;
    if ( 0 != pRocksDb->GetBlockHashByBlockHeight(txn, top - 1, prevBestChainHash) )
    {
        printf("%sGetBlockHashByBlockHeight failed !!! %s\n", ResBlockColor.color().c_str(), ResBlockColor.reset().c_str());
        return -6;
    }

	if (blockHashs.size() == 0)
	{
		return -7;
	}

    if (blockHashs.size() == 1 && cblock.height() == top)
    {
        // 若当前高度只有一个块，则更新top和bestchain
        if ( 0 != pRocksDb->SetBlockTop(txn, --top) )
        {
            printf("%sSetBlockTop failed !!! %s\n", ResBlockColor.color().c_str(), ResBlockColor.reset().c_str());
            return -8;
        }

        if (0 != pRocksDb->SetBestChainHash(txn, prevBestChainHash) )
        {
            printf("%sSetBestChainHash failed !!! %s\n", ResBlockColor.color().c_str(), ResBlockColor.reset().c_str());
            return -9;
        }
    }
	else if (blockHashs.size() == 1 && cblock.height() != top)
	{
		return -10;
	}
    
	if (blockHashs.size() > 1 && bestChainHash == cblock.hash())
    {
        // 当前高度不只一个块，且本块是bestchain，更新bestchain
        struct timeval tv;
        gettimeofday( &tv, NULL );
        uint64_t blockTime = tv.tv_sec * 1000000 + tv.tv_usec;

		auto blockHashsIter = std::find(blockHashs.begin(), blockHashs.end(), cblock.hash());
		if (blockHashsIter != blockHashs.end())
		{
			blockHashs.erase(blockHashsIter);
		}

        for (const auto & hash : blockHashs)
        {
            std::string strBlock;
            if (0 != pRocksDb->GetBlockByBlockHash(txn, hash, strBlock) )
            {
                printf("%sGetBlockByBlockHash failed !!! %s\n", ResBlockColor.color().c_str(), ResBlockColor.reset().c_str());
                return -11;
            }

            CBlock tmpBlock;
            tmpBlock.ParseFromString(strBlock);

            if (tmpBlock.time() < blockTime)
            {
                bestChainHash = tmpBlock.hash();
				blockTime = tmpBlock.time();
            }
        }

        if (0 != pRocksDb->SetBestChainHash(txn, bestChainHash) )
        {
            printf("%sSetBestChainHash failed !!! %s\n", ResBlockColor.color().c_str(), ResBlockColor.reset().c_str());
            return -12;
        }
    }

    // 获取块交易类型
    bool isRedeem = false;
    bool isPledge = false;
    std::string redempUtxoStr;

    nlohmann::json blockExtra = nlohmann::json::parse(cblock.extra());
    std::string txType = blockExtra["TransactionType"].get<std::string>();
    if (txType == TXTYPE_PLEDGE)
    {
        isPledge = true;
    }
    else if (txType == TXTYPE_REDEEM)
    {
        isRedeem = true;
        nlohmann::json txInfo = blockExtra["TransactionInfo"].get<nlohmann::json>();
        redempUtxoStr = txInfo["RedeemptionUTXO"].get<std::string>();
    }

    for (int i = 0; i < cblock.txs_size(); i++)
    {
        CTransaction tx = cblock.txs(i);
        std::vector<std::string> owner_addrs = TxHelper::GetTxOwner(tx);
        if ( 0 != pRocksDb->DeleteTransactionByHash(txn, tx.hash()) )
        {
            printf("%sDeleteTransactionByHash failed !!! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
            return -13;
        }

        if ( 0 != pRocksDb->DeleteBlockHashByTransactionHash(txn, tx.hash()) )
        {
            printf("%sDeleteBlockHashByTransactionHash failed !!! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
            return -14;
        }
        if(CheckTransactionType(tx) == kTransactionType_Tx)
        {
            for(const auto& addr:owner_addrs)
            {
                if ( 0 != pRocksDb->RemoveAllTransactionByAddress(txn, addr, tx.hash()) )
                {
                    std::cout << "addr:" << addr << " hash:" << tx.hash() << std::endl;
                    printf("%sRemoveAllTransactionByAddress  1   failed !!! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
                    return -15;
                }
            }
        }
    }

    CBlockHeader block;
    std::string serBlock;
    if ( 0 != pRocksDb->GetBlockHeaderByBlockHash(txn, blockHash, serBlock) )
    {
        printf("%sGetBlockByBlockHash  2  failed !!! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
        return -16;
    }

    block.ParseFromString(serBlock);

	uint32_t blockHeight = 0;
	if (0 != pRocksDb->GetBlockHeightByBlockHash(txn, blockHash, blockHeight))
	{
		printf("%sGetBlockHeightByBlockHash  failed !!! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
        return -17;
	}

    if ( 0 != pRocksDb->DeleteBlockHeightByBlockHash(txn, blockHash) )
    {
        printf("%sDeleteBlockHeightByBlockHash  failed !!! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
        return -18;
    }

    if ( 0 != pRocksDb->RemoveBlockHashByBlockHeight(txn, blockHeight, blockHash) )
    {
        printf("%sRemoveBlockHashByBlockHeight  failed !!! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
        return -19;
    }

    if ( 0 != pRocksDb->DeleteBlockByBlockHash(txn, blockHash) )
    {
        printf("%sDeleteBlockHeaderByBlockHash  failed !!! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
        return -20;
    }

    if ( 0 != pRocksDb->DeleteBlockHeaderByBlockHash(txn, blockHash) )
    {
        printf("%sDeleteBlockByBlockHash  failed !!! %s\n", ResBlockColor.color().c_str(),  ResBlockColor.reset().c_str());
        return -21;
    }

    for (int i = 0; i < cblock.txs_size(); i++)
    {
        CTransaction tx = cblock.txs(i);
        if (isPledge)
        {
            if (0 != RollbackPledgeTx(pRocksDb, txn, tx) )
            {
                return -22;
            }
        }
        else if (isRedeem)
        {
            if (0 != RollbackRedeemTx(pRocksDb, txn, tx))
            {
                return -23;
            }
        }
        else
        {
            int ret = RollbackTx(pRocksDb, txn, tx);
            if( ret != 0)
            {
                return -24;
            }
        }

        // 回滚账号获得的总奖励值和总签名数
        if (CheckTransactionType(tx) == kTransactionType_Award)
        {
            for (auto & txout : tx.vout())
            {
                // 总奖励值
                uint64_t awardTotal = 0;
                if (0 != pRocksDb->GetAwardTotalByAddress(txn, txout.scriptpubkey(), awardTotal))
                {
                    std::cout << "(RollbackBlockByBlockHash) GetAwardTotalByAddress failed !" << std::endl;
                    error("(RollbackBlockByBlockHash) GetAwardTotalByAddress failed !");
                    return -25;
                }
                awardTotal = (txout.value() > 0) && (awardTotal > (uint64_t)txout.value()) ? awardTotal - txout.value() : 0;

                if (0 != pRocksDb->SetAwardTotalByAddress(txn, txout.scriptpubkey(), awardTotal))
                {
                    std::cout << "(RollbackBlockByBlockHash) SetAwardTotalByAddress failed !" << std::endl;
                    error("(RollbackBlockByBlockHash) SetAwardTotalByAddress failed !");
                    return -26;
                }

                // 总签名数
                uint64_t signSum = 0;
                if (0 != pRocksDb->GetSignNumByAddress(txn, txout.scriptpubkey(), signSum))
                {
                    std::cout << "(RollbackBlockByBlockHash) GetSignNumByAddress failed !" << std::endl;
                    error("(RollbackBlockByBlockHash) GetSignNumByAddress failed !");
                    return -27;
                }
                signSum = signSum > 0 ? signSum - 1 : 0;

                if (0 != pRocksDb->SetSignNumByAddress(txn, txout.scriptpubkey(), signSum))
                {
                    std::cout << "(RollbackBlockByBlockHash) SetSignNumByAddress failed !" << std::endl;
                    error("(RollbackBlockByBlockHash) SetSignNumByAddress failed !");
                    return -28;
                }
            }
        }
    }

    std::cout << ResBlockColor.color().c_str() << "The block " << blockHash << "is rollback successful !" << ResBlockColor.reset().c_str() << std::endl;
    return 0;
}



int Rollback::RollbackToHeight(const unsigned int & height)
{
    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
    if( txn == NULL )
    {
        std::cout << "(RollbackBlockByBlockHash) TransactionInit failed !" << std::endl;
        return -1;
    }

    ON_SCOPE_EXIT{
        pRocksDb->TransactionDelete(txn, false);
    };

    uint32_t top = 0;
    if (pRocksDb->GetBlockTop(txn, top))
    {
        std::cout << "(RollbackToHeight) GetBlockTop failed !" << std::endl;
        return -2;
    }

    if (height >= top)
    {
        std::cout << "(RollbackToHeight) Args error !" << std::endl;
        return -3;
    }

    while(top > height)
    {
        std::vector<std::string> blockHashs;
        if (pRocksDb->GetBlockHashsByBlockHeight(txn, top, blockHashs))
        {
            std::cout << "(RollbackToHeight) GetBlockHashsByBlockHeight failed !" << std::endl;
            return -3;
        }

        for (const auto & blockHash : blockHashs)
        {
            if ( 0 != RollbackBlockByBlockHash(txn, pRocksDb, blockHash) )
            {
                std::cout << "(RollbackToHeight) RollbackBlockByBlockHash failed !" << std::endl;
                return -4;
            }
        }

        // 更新top
        if (pRocksDb->GetBlockTop(txn, top))
        {
            std::cout << "(RollbackToHeight) GetBlockTop failed !" << std::endl;
            return -5;
        }
    }

	std::cout << "(RollbackToHeight) TransactionCommit !!!" << std::endl;
	if (pRocksDb->TransactionCommit(txn))
    {
        std::cout << "(RollbackBlockByBlockHash) TransactionCommit failed !" << std::endl;
        return -29;
    }

    return 0;
}


int Rollback::RollbackBlockBySyncBlock(const uint32_t & conflictHeight, const std::vector<Block> & syncBlocks)
{
	if (syncBlocks.size() == 0)
	{
		std::cout << "(RollbackBlockBySyncBlock) SyncBlocks is empty !" << std::endl;
		return -1;
	}

	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if (txn == nullptr)
	{
		std::cout << "(RollbackBlockBySyncBlock) TransactionInit failed !" << std::endl;
		return -2;
	}

	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, true);
	};

	uint32_t top = 0;
	if (0 != pRocksDb->GetBlockTop(txn, top))
	{
		std::cout << "(RollbackBlockBySyncBlock) GetBlockTop failed !" << std::endl;
		return -3;
	}

	if (conflictHeight >= top)
	{
		std::cout << "(RollbackBlockBySyncBlock) conflictHeight error !" << std::endl;
		return -4;
	}

	std::vector<std::pair<uint32_t, std::vector<std::string>>> blockHeightHashs;
	for (const auto & block : syncBlocks)
	{
		CBlock cblock = block.blockheader_;
		if (cblock.height() < conflictHeight)
		{
			continue;
		}

		bool isHeightExist = false;
		for (auto & blockHeightHash : blockHeightHashs)
		{
			if (cblock.height() == blockHeightHash.first)
			{
				isHeightExist = true;
				blockHeightHash.second.push_back(cblock.hash());
			}
		}

		if (!isHeightExist)
		{
			std::vector<std::string> blockHashs{cblock.hash()};
			blockHeightHashs.push_back(std::make_pair(cblock.height(), blockHashs));
		}
	}

	using type_pair = std::pair<uint32_t, std::vector<std::string>>;
	std::sort(blockHeightHashs.begin(), blockHeightHashs.end(), [](type_pair pair1, type_pair pair2){
		return pair1.first > pair2.first;
	});

	for (const auto & blockHeightHash : blockHeightHashs)
	{
		const uint32_t & height = blockHeightHash.first;
		const std::vector<std::string> & syncBlockHashs = blockHeightHash.second;
		if (height > top)
		{
			continue;
		}

		std::vector<std::string> blockHashs;
		if (0 != pRocksDb->GetBlockHashsByBlockHeight(txn, height, blockHashs))
		{
			std::cout << "(RollbackBlockBySyncBlock) GetBlockHashsByBlockHeight failed !" << std::endl;
			return -5;
		}

		for (const std::string & blockHash : blockHashs)
		{
			auto iter = find(syncBlockHashs.begin(), syncBlockHashs.end(), blockHash);
			if (iter == syncBlockHashs.end())
			{
				if ( 0 != RollbackBlockByBlockHash(txn, pRocksDb, blockHash) )
				{
					std::cout << "(RollbackBlockBySyncBlock) RollbackBlockByBlockHash failed !" << std::endl;
					return -6;
				}
			}
		}
	}

	std::cout << "(RollbackToHeight) TransactionCommit !!!" << std::endl;
    if (pRocksDb->TransactionCommit(txn))
    {
        std::cout << "(RollbackBlockByBlockHash) TransactionCommit failed !" << std::endl;
        return -29;
    }

	return 0;
}

