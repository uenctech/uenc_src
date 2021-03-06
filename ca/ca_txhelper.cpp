
#include "ca_blockpool.h"
#include "ca_console.h"
#include "../include/ADVobfuscator/Log.h"
#include "../include/ADVobfuscator/MetaFSM.h"
#include "../include/ADVobfuscator/MetaString.h"
#include "../include/ADVobfuscator/ObfuscatedCall.h"
#include "../include/ADVobfuscator/ObfuscatedCallWithPredicate.h"

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
#include "ca_txhelper.h"
#include "../utils/string_util.h"
#include "../utils/time_util.h"
#include "../utils/json.hpp"
#include "ca_base64.h"
#include "ca_message.h"
#include "ca_txvincache.h"

std::vector<std::string> TxHelper::GetTxOwner(const std::string tx_hash)
{
	std::vector<std::string> address;
	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if( txn == NULL )
	{
		std::cout << "(CreateTxMessage) TransactionInit failed !" << std::endl;
		return address;
	}

	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, true);
	};

	std::string strTxRaw;
	if (pRocksDb->GetTransactionByHash(txn, tx_hash, strTxRaw) != 0)
	{
		return address;
	}

	CTransaction Tx;
	Tx.ParseFromString(strTxRaw);
	
	return GetTxOwner(Tx);
}

std::string TxHelper::GetTxOwnerStr(const std::string tx_hash)
{
	std::vector<std::string> address = GetTxOwner(tx_hash);
	return StringUtil::concat(address, "_");
}

std::vector<std::string> TxHelper::GetTxOwner(const CTransaction & tx)
{
	// std::cout << "GetTxOwner:=========" << std::endl;
	std::vector<std::string> address;
	for (int i = 0; i < tx.vin_size(); i++)
	{
		CTxin txin = tx.vin(i);
		auto pub = txin.mutable_scriptsig()->pub();
		std::string addr = GetBase58Addr(pub); 
		auto res = std::find(std::begin(address), std::end(address), addr);
		if (res == std::end(address)) 
		{
			address.push_back(addr);
			// std::cout << "addr:" << addr << std::endl;
		}
	}

	return address;	
}

std::string TxHelper::GetTxOwnerStr(const CTransaction & tx)
{
	std::vector<std::string> address = GetTxOwner(tx);
	return StringUtil::concat(address, "_");
}

std::vector<std::string> TxHelper::GetUtxosByAddresses(std::vector<std::string> addresses)
{
	std::vector<std::string> vUtxoHashs;
	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if( txn == NULL )
	{
		std::cout << "(GetUtxosByAddresses) TransactionInit failed !" << std::endl;
		return vUtxoHashs;
	}

	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, true);
	};

	for(auto& addr:addresses)
	{
		std::vector<std::string> tmp;
		auto db_status = pRocksDb->GetUtxoHashsByAddress(txn, addr, tmp);
		if (db_status != 0) {
			std::cout << "GetUtxosByAddresses GetUtxoHashsByAddress:" <<  addr << std::endl;
			return vUtxoHashs;
		}
		std::for_each(tmp.begin(), tmp.end(),
				[&](std::string &s){ s = s + "_" + addr;}
		);		

		vUtxoHashs.insert(vUtxoHashs.end(), tmp.begin(), tmp.end());
	}
	return vUtxoHashs;
}

std::vector<std::string> TxHelper::GetUtxosByTx(const CTransaction & tx)
{
	std::vector<std::string> v1;
	for(int j = 0; j < tx.vin_size(); j++)
	{
		CTxin vin = tx.vin(j);
		std::string hash = vin.prevout().hash();
		
		if(hash.size() > 0)
		{
			v1.push_back(hash + "_" + GetBase58Addr(vin.scriptsig().pub()));
		}
	}
	return v1;
}


uint64_t TxHelper::GetUtxoAmount(std::string tx_hash, std::string address)
{
	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if( txn == NULL )
	{
		std::cout << "(CreateTxMessage) TransactionInit failed !" << std::endl;
		return 0;
	}

	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, true);
	};

	std::string strTxRaw;
	if (pRocksDb->GetTransactionByHash(txn, tx_hash, strTxRaw) != 0)
	{
		return 0;
	}

	CTransaction tx;
	tx.ParseFromString(strTxRaw);

	uint64_t amount = 0;
	for (int j = 0; j < tx.vout_size(); j++)
	{
		CTxout txout = tx.vout(j);
		if (txout.scriptpubkey() == address)
		{
			amount += txout.value();
		}
	}
	return amount;
}


int TxHelper::CreateTxMessage(const std::vector<std::string> & fromAddr, 
	const std::map<std::string, int64_t> toAddr, 
	uint32_t needVerifyPreHashCount, 
	uint64_t minerFees, 
	CTransaction & outTx,
	bool is_local)
{
	if (fromAddr.size() == 0 || toAddr.size() == 0)
	{
		error("CreateTxMessage fromAddr toAddr ==0");
		cout<<"CreateTxMessage test1"<<endl;
		return -1;
	}

	// std::cout << "CreateTxMessage fee is: " << minerFees << std::endl;
	std::set<std::string> fromSet;
	std::set<std::string> toSet;
	for (auto & to : toAddr)
	{
		if (to.first != VIRTUAL_ACCOUNT_PLEDGE && ! CheckBase58Addr(to.first))
		{
			cout<<"CreateTxMessage test2"<<endl;
			return -2;
		}

		toSet.insert(to.first);

		for (auto & from : fromAddr)
		{
			if (! CheckBase58Addr(from))
			{
				cout<<"CreateTxMessage test3"<<endl;
				return -2;
			}

			fromSet.insert(from);
			if (from == to.first)
			{
				return -2;
			}
		}
	}

	if (fromSet.size() != fromAddr.size() || toSet.size() != toAddr.size())
	{
		cout<<"CreateTxMessage test4"<<endl;
		return -2;
	}

	if (MagicSingleton<TxVinCache>::GetInstance()->IsConflict(fromAddr))
	{
		error("Pending transaction is in Cache!");
		std::cout << "Pending transaction is in Cache!" << std::endl;
		return -3;
	}

    int db_status = 0;
	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if( txn == NULL )
	{
		std::cout << "(CreateTxMessage) TransactionInit failed !" << std::endl;
		return -4;
	}

	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, true);
	};
	

	uint64_t amount = 0;
	for(auto& i:toAddr)
	{
		amount += i.second;
	}
	amount += ( (needVerifyPreHashCount - 1) * minerFees );

	// Do you need a packing fee 
	bool bIsNeedPackage = IsNeedPackage(fromAddr);
	
	// The transaction initiator pays the packaging fee 
	uint64_t publicNodePackageFee = 0;
	if (bIsNeedPackage)
	{
		if ( 0 != pRocksDb->GetDevicePackageFee(publicNodePackageFee) )
		{
			error("CreateTxMessage GetDevicePackageFee failed");
			cout<<"CreateTxMessage test6"<<endl;
			return -5;
		}

		amount += publicNodePackageFee;
	}

	std::map<std::string, std::vector<std::string>> utxoHashs;
	for(auto& addr:fromAddr)
	{
		std::vector<std::string> tmp;
		db_status = pRocksDb->GetUtxoHashsByAddress(txn, addr, tmp);
		if (db_status != 0) {
			error("CreateTxMessage GetUtxoHashsByAddress");
			cout<<"CreateTxMessage test7"<<endl;
			return -6;
		}
		utxoHashs[addr] = tmp;
	}

	uint64_t total = 0;
	std::string change_addr;
	std::set<std::string> txowners;
	for(auto& addr:fromAddr)
	{
		for (auto& item : utxoHashs[addr])
		{
			std::string strTxRaw;
			if (pRocksDb->GetTransactionByHash(txn, item, strTxRaw) != 0)
			{
				continue;
			}
			CTransaction utxoTx;
			utxoTx.ParseFromString(strTxRaw);

			for (int i = 0; i < utxoTx.vout_size(); i++)
			{
				CTxout txout = utxoTx.vout(i);
				if(txout.scriptpubkey() == addr)
				{
					txowners.insert(addr);
					change_addr = addr;
					// std::cout << "total:" << total << "addr:" << addr << "txout.value():" << txout.value() << std::endl;
					total += txout.value();

					CTxin * txin = outTx.add_vin();
					CTxprevout * prevout = txin->mutable_prevout();
					prevout->set_hash(utxoTx.hash());
					prevout->set_n(utxoTx.n());

					if(is_local)
					{
						std::string strPub;
						g_AccountInfo.GetPubKeyStr(txout.scriptpubkey().c_str(), strPub);
						txin->mutable_scriptsig()->set_pub(strPub);
					}
					else
					{
						txin->mutable_scriptsig()->set_pub(addr);
						std::cout << "CreateTxMessage:" << "vin addr:" << addr << std::endl;
					}
				}
			}
			if (total >= amount)
			{
				break;
			}
		}
		if (total >= amount)
		{
			break;
		}		
	}
	// std::cout << "total:" << total << std::endl;

	if (total < amount)
	{
		error("CreateTxMessage total < amount");
		cout<<"CreateTxMessage test8"<<endl;
		return -7;
	}

	if((uint64_t)minerFees < g_minSignFee || (uint64_t)minerFees > g_maxSignFee)
	{
		return -15;
	}

	for(auto& i:toAddr)
	{
		CTxout * txoutToAddr = outTx.add_vout();
		txoutToAddr->set_scriptpubkey(i.first);
		txoutToAddr->set_value(i.second);
	}
	
	CTxout * txoutFromAddr = outTx.add_vout();
	txoutFromAddr->set_value(total - amount);
	txoutFromAddr->set_scriptpubkey(change_addr);

	uint64_t time = Singleton<TimeUtil>::get_instance()->getlocalTimestamp();
	outTx.set_time(time);

	std::string tmpAddr;
	for (auto & addr : txowners)
	{
		tmpAddr += addr;
		tmpAddr += "_";
	}
	
	tmpAddr.erase(tmpAddr.end() -1);
	
	outTx.set_txowner(tmpAddr);

	outTx.set_ip(net_get_self_node_id());

	nlohmann::json extra;
	extra["NeedVerifyPreHashCount"] = needVerifyPreHashCount;
	extra["SignFee"] = minerFees;
	extra["PackageFee"] = publicNodePackageFee;
	extra["TransactionType"] = TXTYPE_TX;
	outTx.set_extra(extra.dump());

	return 0;
}

void TxHelper::DoCreateTx(const std::vector<std::string> & fromAddr, 
	const std::map<std::string, int64_t> toAddr, 
	uint32_t needVerifyPreHashCount, 
	uint64_t gasFee)
{
	if (fromAddr.size() == 0 || toAddr.size() == 0 )
	{
		error("DoCreateTx: fromAddr.size() == 0 || toAddr.size() == 0");
		return; 
	}

	CTransaction outTx;
    int ret = TxHelper::CreateTxMessage(fromAddr,toAddr, needVerifyPreHashCount, gasFee, outTx);
	if(ret != 0)
	{
		error("DoCreateTx: TxHelper::CreateTxMessage error!!!\n");
		cout<<"pending DoCreateTx: TxHelper::CreateTxMessage error!!!\n"<<endl;
		return;
	}

	std::vector<std::string> addrs;
	for (int i = 0; i < outTx.vin_size(); i++)
	{
		CTxin * txin = outTx.mutable_vin(i);;
		std::string pub = txin->mutable_scriptsig()->pub();
		txin->clear_scriptsig();
		addrs.push_back(GetBase58Addr(pub));
		// std::cout << "GetBase58Addr(pub):" << GetBase58Addr(pub) << std::endl;
	}

	std::string serTx = outTx.SerializeAsString();

	size_t encodeLen = serTx.size() * 2 + 1;
	unsigned char encode[encodeLen] = {0};
	memset(encode, 0, encodeLen);
	long codeLen = base64_encode((unsigned char *)serTx.data(), serTx.size(), encode);
	std::string encodeStr( (char *)encode, codeLen );

	std::string encodeStrHash = getsha256hash(encodeStr);

	//signature 
	for (int i = 0; i < outTx.vin_size(); i++)
	{
		std::string addr = addrs[i];
		// std::cout << "DoCreateTx:signature :" << addr << std::endl;
		std::string signature;
		std::string strPub;
		g_AccountInfo.Sign(addr.c_str(), encodeStrHash, signature);
		g_AccountInfo.GetPubKeyStr(addr.c_str(), strPub);
		CTxin * txin = outTx.mutable_vin(i);
		txin->mutable_scriptsig()->set_sign(signature);
		txin->mutable_scriptsig()->set_pub(strPub);
	}
	
	serTx = outTx.SerializeAsString();

	TxMsg txMsg;
	txMsg.set_version(getVersion());
	txMsg.set_tx( serTx );
	txMsg.set_txencodehash( encodeStrHash );

	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, true);
	};

	unsigned int top = 0;
	pRocksDb->GetBlockTop(txn, top);	
	txMsg.set_top(top);

	std::string blockHash;
    if ( 0 != pRocksDb->GetBestChainHash(txn, blockHash) )
    {
        std::cout << __LINE__ << std::endl;
        return;
    }
    txMsg.set_prevblkhash(blockHash);	
    txMsg.set_trycountdown(CalcTxTryCountDown(needVerifyPreHashCount));

	auto msg = make_shared<TxMsg>(txMsg);

	std::string txHash;
	ret = DoHandleTx(msg, txHash);
	std::cout << "Transaction processing result ，ret: "<< ret <<" txHash：" << txHash << std::endl;
	
}