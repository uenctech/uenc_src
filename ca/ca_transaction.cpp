#include <stdio.h>
#include <iostream>
#include <sstream>
#include <string>
#include <set>
#include <algorithm>
#include <assert.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <shared_mutex>
#include <mutex>
#include "ca_transaction.h"
#include "ca_message.h"
#include "ca_hexcode.h"
#include "ca_buffer.h"
#include "ca_serialize.h"
#include "ca_util.h"
#include "ca_global.h"
#include "ca_coredefs.h"
#include "ca_hexcode.h"
#include "Crypto_ECDSA.h"
#include "ca_interface.h"
#include "../include/logging.h"
#include "ca.h"
#include "ca_test.h"
#include "../include/net_interface.h"
#include "ca_clientinfo.h"
#include "../include/ScopeGuard.h"
#include "ca_clientinfo.h"
#include "../common/config.h"
#include "ca_clientinfo.h"
#include "ca_console.h"
#include "ca_device.h"
#include <string.h>
#include "ca_header.h"
#include "ca_sha2.h"
#include "ca_base64.h"
#include "ca_txhelper.h"
#include "ca_pwdattackchecker.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "../utils/time_util.h"
#include "../common/devicepwd.h"
#include "../net/node_cache.h"

static std::mutex s_ResMutex;
#include "./ca_blockpool.h"
#include "../utils/string_util.h"
#include "getmac.pb.h"
#include "ca_AwardAlgorithm.h"

#include "proto/ca_protomsg.pb.h"
#include "ca_txhelper.h"
#include "ca_txvincache.h"
#include "ca_txconfirmtimer.h"
#include "ca_block_http_callback.h"
#include "interface.pb.h"

std::shared_mutex MinutesCountLock;
static const int REASONABLE_HEIGHT_RANGE = 10;

template<typename Ack> void ReturnAckCode(const MsgData& msgdata, std::map<int32_t, std::string> errInfo, Ack & ack, int32_t code, const std::string & extraInfo = "");
template<typename TxReq> int CheckAddrs( const std::shared_ptr<TxReq>& req);

int GetAddrsFromMsg( const std::shared_ptr<CreateMultiTxMsgReq>& msg, 
                     std::vector<std::string> &fromAddr,
                     std::map<std::string, int64_t> &toAddr);

int StringSplit(std::vector<std::string>& dst, const std::string& src, const std::string& separator)
{
    if (src.empty() || separator.empty())
        return 0;

    int nCount = 0;
    std::string temp;
    size_t pos = 0, offset = 0;

    // Divide the 1st~n-1th 
    while((pos = src.find_first_of(separator, offset)) != std::string::npos)
    {
        temp = src.substr(offset, pos - offset);
        if (temp.length() > 0)
		{
            dst.push_back(temp);
            nCount ++;
        }
        offset = pos + 1;
    }

    // Split the nth 
    temp = src.substr(offset, src.length() - offset);
    if (temp.length() > 0)
	{
        dst.push_back(temp);
        nCount ++;
    }

    return nCount;
}

uint64_t CheckBalanceFromRocksDb(const std::string & address)
{
	if (address.size() == 0)
	{
		return 0;
	}

	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if( txn == NULL )
	{
		std::cout << "(CheckBalanceFromRocksDb) TransactionInit failed !" << std::endl;
		return 0;
	}

	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, true);
	};

	int64_t balance = 0;
	int r = pRocksDb->GetBalanceByAddress(txn, address, balance);
	if (r != 0)
	{
		return 0;
	}

	return balance;
}

bool FindUtxosFromRocksDb(const std::string & fromAddr, const std::string & toAddr, uint64_t amount, uint32_t needVerifyPreHashCount, uint64_t minerFees, CTransaction & outTx, std::string utxoStr)
{
	if (fromAddr.size() == 0 || toAddr.size() == 0)
	{
		error("FindUtxosFromRocksDb fromAddr toAddr ==0");
		return false;
	}

	//{{ Check pending transaction in Cache, 20201215
	std::vector<string> vectFromAddr{ fromAddr };
	if (MagicSingleton<TxVinCache>::GetInstance()->IsConflict(vectFromAddr))
	{
		error("Pending transaction is in Cache!");
		std::cout << "Pending transaction is in Cache, " << fromAddr << std::endl;
		return false;
	}
	//}}
	
	uint64_t totalGasFee = (needVerifyPreHashCount - 1) * minerFees;
	uint64_t amt = fromAddr == toAddr ? totalGasFee : amount + totalGasFee;
	
    int db_status = 0;
	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if( txn == NULL )
	{
		std::cout << "(FindUtxosFromRocksDb) TransactionInit failed !" << std::endl;
		return false;
	}

	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, true);
	};

	uint64_t total = 0;
	std::vector<std::string> utxoHashs;
	std::vector<std::string> pledgeUtxoHashs;

	// Depledge transaction 
	if (fromAddr == toAddr)
	{
		db_status = pRocksDb->GetPledgeAddressUtxo(txn, fromAddr, pledgeUtxoHashs);
		if (db_status != 0)
		{
			return false;
		}

		std::string strTxRaw;
		if (pRocksDb->GetTransactionByHash(txn, utxoStr, strTxRaw) != 0)
		{
			return false;
		}

		CTransaction utxoTx;
		utxoTx.ParseFromString(strTxRaw);

		for (int i = 0; i < utxoTx.vout_size(); i++)
		{
			CTxout txout = utxoTx.vout(i);
			if (txout.scriptpubkey() != VIRTUAL_ACCOUNT_PLEDGE)
			{
				continue;
			}
			amount = txout.value();
		}
	}
	
	db_status = pRocksDb->GetUtxoHashsByAddress(txn, fromAddr, utxoHashs);
	if (db_status != 0) 
	{
		error("FindUtxosFromRocksDb GetUtxoHashsByAddress");
		return false;
	}
	
	// De-duplication 
	std::set<std::string> setTmp(utxoHashs.begin(), utxoHashs.end());
	utxoHashs.clear();
	utxoHashs.assign(setTmp.begin(), setTmp.end());

	std::reverse(utxoHashs.begin(), utxoHashs.end());
	if (pledgeUtxoHashs.size() > 0)
	{
		std::reverse(pledgeUtxoHashs.begin(), pledgeUtxoHashs.end());
	}

	// Utxo used to record the handling fee 
	std::vector<std::string> vinUtxoHash;

	for (auto &item : utxoHashs)
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
			if (txout.scriptpubkey() != fromAddr)
			{
				continue;
			}
			else
			{
				total += txout.value();

				CTxin * txin = outTx.add_vin();
				CTxprevout * prevout = txin->mutable_prevout();
				prevout->set_hash(utxoTx.hash());
				prevout->set_n(utxoTx.n());
				// TODO

				vinUtxoHash.push_back(utxoTx.hash());
			}

			// Unstaking will generate a UTXO and two vouts to the pledge account at the same time, and both need to be included. 
			if (i < utxoTx.vout_size() - 1)
			{
				continue;
			}

			if (total >= amt)
			{
				break;
			}
		}

		if (total >= amt)
		{
			break;
		}
	}


	if (total < amt)
	{
		error("FindUtxosFromRocksDb total < amt");
		return false;
	}

	if (fromAddr == toAddr)
	{
		std::string utxoTxStr;
		if (pRocksDb->GetTransactionByHash(txn, utxoStr, utxoTxStr) != 0)
		{
			error("GetTransactionByHash error!");
			return false;
		}

		CTransaction utxoTx;
		utxoTx.ParseFromString(utxoTxStr);
		for (int i = 0; i < utxoTx.vout_size(); i++)
		{
			CTxout txout = utxoTx.vout(i);
			if (txout.scriptpubkey() != VIRTUAL_ACCOUNT_PLEDGE)
			{
				bool isAlreadyAdd = false;
				for (auto &hash : vinUtxoHash)
				{
					// The utxo is used in the handling fee 
					if (hash == utxoTx.hash())
					{
						isAlreadyAdd = true;
					}
				}

				// There is a vout in the utxo that is a normally usable asset and needs to be calculated and given to the account again 
				if (txout.scriptpubkey() == fromAddr && !isAlreadyAdd)
				{
					for (auto &utxo : utxoHashs)
					{
						// It needs to be calculated when the available assets of the utxo are not used 
						if (utxo == utxoTx.hash())
						{
							total += txout.value();
						}
					}
				}
				continue;
			}

			CTxin * txin = outTx.add_vin();
			CTxprevout * prevout = txin->mutable_prevout();
			prevout->set_hash(utxoTx.hash());
			prevout->set_n(utxoTx.n());
		}
	}

	CTxout * txoutToAddr = outTx.add_vout();
	txoutToAddr->set_value(amount);
	txoutToAddr->set_scriptpubkey(toAddr);

	CTxout * txoutFromAddr = outTx.add_vout();
	txoutFromAddr->set_value(total - amt);
	txoutFromAddr->set_scriptpubkey(fromAddr);

	uint64_t time = Singleton<TimeUtil>::get_instance()->getlocalTimestamp();
	outTx.set_time(time);
	outTx.set_txowner(fromAddr);
	outTx.set_ip(net_get_self_node_id());
	
	return true;
}

TransactionType CheckTransactionType(const CTransaction & tx)
{
	if( tx.time() == 0 || tx.hash().length() == 0 || tx.vin_size() == 0 || tx.vout_size() == 0)
	{
		return kTransactionType_Unknown;
	}

	CTxin txin = tx.vin(0);
	if ( txin.scriptsig().sign() == std::string(FEE_SIGN_STR))
	{
		return kTransactionType_Fee;
	}
	else if (txin.scriptsig().sign() == std::string(EXTRA_AWARD_SIGN_STR))
	{
		return kTransactionType_Award;
	}

	return kTransactionType_Tx;
}

bool checkTop(int top)
{
	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if(txn == NULL )
	{
		std::cout << "(checkTop) TransactionInit failed ! " << __LINE__ << std::endl;
		return false;
	}
	bool bRollback = true;
	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, bRollback);
	};

	unsigned int mytop = 0;
	pRocksDb->GetBlockTop(txn, mytop);	

	if(top < (int)mytop - 4 )
	{
		error("checkTop fail other top:%d my top:%d", top, (int)mytop);
		return false;
	}
	else if(top > (int)mytop + 1)
	{
		error("checkTop other top:%d my top:%d", top, (int)mytop);
		return false;
	}
	else
	{
		return true;
	}
}

bool checkTransaction(const CTransaction & tx)
{
	if (tx.vin_size() == 0 || tx.vout_size() == 0)
	{
		return false;
	}

	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if ( txn == NULL )
	{
		error("(FindUtxosFromRocksDb) TransactionInit failed !");
		return false;
	}

	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, false);
	};

	int db_status = 0;

	uint64_t total = 0;
	for (int i = 0; i < tx.vout_size(); i++)
	{
		CTxout txout = tx.vout(i);
		total += txout.value();
	}

	// Check the total amount 
	if (total < 0 || total > 21000000LL * COIN)
	{
		return false;
	}

	std::vector<CTxin> vTxins;
	for (int i = 0; i < tx.vin_size(); i++)
	{
		vTxins.push_back(tx.vin(i));
	}

	bool isRedeem = false;
	nlohmann::json extra = nlohmann::json::parse(tx.extra());
	std::string txType = extra["TransactionType"].get<std::string>();
	std::string redeemUtxo;

	if (txType == TXTYPE_REDEEM)
	{
		isRedeem= true;

		if (CheckTransactionType(tx) == kTransactionType_Tx)
		{
			nlohmann::json txInfo = extra["TransactionInfo"].get<nlohmann::json>();
			redeemUtxo = txInfo["RedeemptionUTXO"];

			// Check the pledge time 
			if ( 0 != IsMoreThan30DaysForRedeem(redeemUtxo) )
			{
				error("Redeem time is less than 30 days!");
				std::cout << "Redeem time is less than 30 days!" << std::endl;
				return false;
			}
		}
	}

	if (CheckTransactionType(tx) == kTransactionType_Tx)
	{
		// Check if the signer of txowner and vin are the same 
		std::vector<std::string> vinSigners;
		for (const auto & vin : vTxins)
		{
			std::string pubKey = vin.scriptsig().pub();
			std::string addr = GetBase58Addr(pubKey);
			if (vinSigners.end() == find(vinSigners.begin(), vinSigners.end(), addr))
			{
				vinSigners.push_back(addr);
			}
		}

		std::vector<std::string> txOwners = TxHelper::GetTxOwner(tx);
		if (vinSigners != txOwners)
		{
			error("TxOwner or vin signer error!");
			return false;
		}

		// Does utxo exist 
		for (const auto & vin : vTxins)
		{
			std::string pubKey = vin.scriptsig().pub();
			std::string addr = GetBase58Addr(pubKey);

			std::vector<std::string> utxos;
			if (pRocksDb->GetUtxoHashsByAddress(txn, addr, utxos))
			{
				error("GetUtxoHashsByAddress error !");
				return false;
			}
			
			std::vector<std::string> pledgeUtxo;
			pRocksDb->GetPledgeAddressUtxo(txn, addr, pledgeUtxo);
			if (utxos.end() == find(utxos.begin(), utxos.end(), vin.prevout().hash()))
			{
				if (isRedeem)
				{
					if (vin.prevout().hash() != redeemUtxo)
					{
						error("tx vin not found !");
						return false;
					}
				}
				else
				{
					error("tx vin not found !");
					return false;
				}
			}
		}
	}

	// Check for duplicate vin 
	std::sort(vTxins.begin(), vTxins.end(), [](const CTxin & txin0, const CTxin & txin1){
		if (txin0.prevout().n() > txin1.prevout().n())
		{
			return true;
		}
		else
		{
			return false;
		}
	});
	auto iter = std::unique(vTxins.begin(), vTxins.end(), [](const CTxin & txin0, const CTxin & txin1){
		return txin0.prevout().n() == txin1.prevout().n() &&
				txin0.prevout().hash() == txin1.prevout().hash() &&
				txin0.scriptsig().sign() == txin1.scriptsig().sign();
	});

	if (iter != vTxins.end())
	{
		if (isRedeem)
		{
			std::vector<std::string> utxos;
			string txowner = TxHelper::GetTxOwner(tx)[0];
			db_status = pRocksDb->GetPledgeAddressUtxo(txn, TxHelper::GetTxOwner(tx)[0], utxos);
			if (db_status != 0)
			{
				return false;
			}
			auto utxoIter = find(utxos.begin(), utxos.end(), iter->prevout().hash());
			if (utxoIter == utxos.end())
			{
				std::string txRaw;
				db_status = pRocksDb->GetTransactionByHash(txn, iter->prevout().hash(), txRaw);
				if (db_status != 0)
				{
					return false;
				}

				CTransaction utxoTx;
				utxoTx.ParseFromString(txRaw);
				if (utxoTx.vout_size() == 2)
				{
					if (utxoTx.vout(0).scriptpubkey() != utxoTx.vout(1).scriptpubkey())
					{
						return false;
					}
				}
			}
		}
		else
		{
			std::string txRaw;
			db_status = pRocksDb->GetTransactionByHash(txn, iter->prevout().hash(), txRaw);
			if (db_status != 0)
			{
				return false;
			}

			CTransaction utxoTx;
			utxoTx.ParseFromString(txRaw);
			if (utxoTx.vout_size() == 2)
			{
				if (utxoTx.vout(0).scriptpubkey() != utxoTx.vout(1).scriptpubkey())
				{
					return false;
				}
			}
		}
	}

	if (CheckTransactionType(tx) == kTransactionType_Tx)
	{
		// transaction 
		for (auto &txin : vTxins)
		{

			if (txin.prevout().n() == 0xFFFFFFFF)
			{
				return false;
			}
		}
	}
	else
	{
		// reward 
		unsigned int height = 0;
		db_status = pRocksDb->GetBlockTop(txn, height);
        if (db_status != 0) 
		{
            return false;
        }
		if (tx.signprehash().size() > 0 && 0 == height)
		{
			return false;
		}

		CTxin txin0 = tx.vin(0);
		int scriptSigLen = txin0.scriptsig().sign().length() + txin0.scriptsig().pub().length();
		if (scriptSigLen < 2 || scriptSigLen > 100)
		{
			return false;
		}

		for (auto &txin : vTxins)
		{
			if (height == 0 && (txin.scriptsig().sign() + txin.scriptsig().pub()) == OTHER_COIN_BASE_TX_SIGN)
			{
				return false;
			}
		}
	}
	
	return true;
}

std::vector<std::string> randomNode(unsigned int n)
{
	std::vector<Node> nodeInfos ;
	if (Singleton<PeerNode>::get_instance()->get_self_node().is_public_node)
	{
		nodeInfos = Singleton<PeerNode>::get_instance()->get_nodelist();
		cout<<"randomNode PeerNode size() ="<<nodeInfos.size()<<endl;
	}
	else
	{
		nodeInfos = Singleton<NodeCache>::get_instance()->get_nodelist();
		cout<<"randomNode NodeCache size() ="<<nodeInfos.size()<<endl;
	}
	std::vector<std::string> v;
	for (const auto & i : nodeInfos)
	{
		v.push_back(i.id);
	}
	
	unsigned int nodeSize = n;
	std::vector<std::string> sendid;
	if ((unsigned int)v.size() < nodeSize)
	{
		debug("not enough node to send");
		return  sendid;
	}

	std::string s = net_get_self_node_id();
	auto iter = std::find(v.begin(), v.end(), s);
	if (iter != v.end())
	{
		v.erase(iter);
	}

	if (v.empty())
	{
		return v;
	}

	if (v.size() <= nodeSize)
	{
		for (auto & i : v)
		{
			sendid.push_back(i);	
		}
	}
	else
	{
		std::set<int> rSet;
		srand(time(NULL));
		while (1)
		{
			int i = rand() % v.size();
			rSet.insert(i);
			if (rSet.size() == nodeSize)
			{
				break;
			}
		}

		for (auto &i : rSet)
		{
			sendid.push_back(v[i]);
		}
	}
	
	return sendid;
}

int GetSignString(const std::string & message, std::string & signature, std::string & strPub)
{
	if (message.size() <= 0)
	{
		error("(GetSignString) parameter is empty!");
		return -1;
	}

	bool result = false;
	result = SignMessage(g_privateKey, message, signature);
	if (!result)
	{
		return -1;
	}

	GetPublicKey(g_publicKey, strPub);
	return 0;
}

/** Create a transaction body on the mobile terminal  */
int CreateTransactionFromRocksDb( const std::shared_ptr<CreateTxMsgReq>& msg, std::string &serTx)
{
	if ( msg == NULL )
	{
		return -1;
	}

	CTransaction outTx;
	double amount = stod( msg->amt() );
	uint64_t amountConvert = amount * DECIMAL_NUM;
	double minerFeeConvert = stod( msg->minerfees() );
	if(minerFeeConvert <= 0)
	{
		std::cout << "phone -> minerFeeConvert == 0 !" << std::endl;
		return -2;
	}
	uint64_t gasFee = minerFeeConvert * DECIMAL_NUM;

	uint32_t needVerifyPreHashCount = stoi( msg->needverifyprehashcount() );

    std::vector<std::string> fromAddr;
    fromAddr.emplace_back(msg->from());

    std::map<std::string, int64_t> toAddr;
    toAddr[msg->to()] = amountConvert;

	int ret = TxHelper::CreateTxMessage(fromAddr,toAddr, needVerifyPreHashCount, gasFee, outTx);
	if( ret != 0)
	{
		error("CreateTransaction Error ...\n");
		return -3;
	}
	
	for (int i = 0; i < outTx.vin_size(); i++)
	{
		CTxin * txin = outTx.mutable_vin(i);;
		txin->clear_scriptsig();
	}
	
	serTx = outTx.SerializeAsString();
	return 0;
}


int GetRedemUtxoAmount(const std::string & redeemUtxoStr, uint64_t & amount)
{
	if (redeemUtxoStr.size() != 64)
	{
		error("(GetRedemUtxoAmount) param error !");
		return -1;
	}

	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if( txn == NULL )
	{
		std::cout << "(VerifyBlockHeader) TransactionInit failed !" << std::endl;
		return -2;
	}

	bool bRollback = true;
	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, bRollback);
	};

	std::string txRaw;
	if ( 0 != pRocksDb->GetTransactionByHash(txn, redeemUtxoStr, txRaw) )
	{
		error("(GetRedemUtxoAmount) GetTransactionByHash failed !");
		return -3;
	}

	CTransaction tx;
	tx.ParseFromString(txRaw);

	for (int i = 0; i < tx.vout_size(); ++i)
	{
		CTxout txout = tx.vout(i);
		if (txout.scriptpubkey() == VIRTUAL_ACCOUNT_PLEDGE)
		{
			amount = txout.value();
		}
	}

	return 0;
}


bool VerifyBlockHeader(const CBlock & cblock)
{
	// uint64_t t1 = Singleton<TimeUtil>::get_instance()->getlocalTimestamp();
	std::string hash = cblock.hash();
    int db_status = 0;
	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if( txn == NULL )
	{
		std::cout << "(VerifyBlockHeader) TransactionInit failed !" << std::endl;
		return false;
	}

	bool bRollback = true;
	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, bRollback);
	};

	std::string strTempHeader;
	db_status = pRocksDb->GetBlockByBlockHash(txn, hash, strTempHeader);

	if (strTempHeader.size() != 0)
	{
		std::cout<<"BlockInfo has exist , do not need to add ..."<<std::endl;
		debug("BlockInfo has exist , do not need to add ...");
		bRollback = true;
        return false;
	}

	std::string strPrevHeader;
	db_status = pRocksDb->GetBlockByBlockHash(txn, cblock.prevhash(), strPrevHeader);
    if (db_status != 0) 
	{
        std::cout << "VerifyBlockHeader (GetBlockByBlockHash) failed ! " << __LINE__ << std::endl;
    }
	if (strPrevHeader.size() == 0)
	{
		error("bp_block_valid lookup hashPrevBlock ERROR !!!");
		bRollback = true;
		return false;
	}

	// Block check 

	// Timestamp check 
	std::string strGenesisBlockHash;
	db_status = pRocksDb->GetBlockHashByBlockHeight(txn, 0, strGenesisBlockHash);
	if (db_status != 0)
	{
		error("GetBlockHashByBlockHeight failed!" );
		return false;
	}
	std::string strGenesisBlockHeader;
	pRocksDb->GetBlockByBlockHash(txn, strGenesisBlockHash, strGenesisBlockHeader);
	if (db_status != 0)
	{
		error("GetBlockHashByBlockHeight failed!" );
		return false;
	}

	if (strGenesisBlockHeader.length() == 0)
	{
		error("Genesis Block is not exist");
		return false;
	}
	
	CBlock genesisBlockHeader;
	genesisBlockHeader.ParseFromString(strGenesisBlockHeader);
	uint64_t blockHeaderTimestamp = cblock.time();
	uint64_t genesisBlockHeaderTimestamp = genesisBlockHeader.time();
	if (blockHeaderTimestamp == 0 || genesisBlockHeaderTimestamp == 0 || blockHeaderTimestamp <= genesisBlockHeaderTimestamp)
	{
		error("block timestamp error!");
		return false;
	}

	if (cblock.txs_size() == 0)
	{
		error("cblock.txs_size() == 0");
		bRollback = true;
		return false;
	}

	if (cblock.SerializeAsString().size() > MAX_BLOCK_SIZE)
	{
		error("cblock.SerializeAsString().size() > MAX_BLOCK_SIZE");
		bRollback = true;
		return false;
	}

	if (CalcBlockHeaderMerkle(cblock) != cblock.merkleroot())
	{
		error("CalcBlockHeaderMerkle(cblock) != cblock.merkleroot()");
		bRollback = true;
		return false;
	}

	// Get the number of signatures in this block 
	uint64_t blockSignNum = 0;
	for (auto & txTmp : cblock.txs())
	{
		if (CheckTransactionType(txTmp) == kTransactionType_Fee)
		{
			blockSignNum = txTmp.vout_size();
		}
	}

	std::map<std::string, uint64_t> addrAwards;
	// Obtain the node information in the reward transaction, calculate the reward value of each account, and use it for verification 
	if (cblock.height() > g_compatMinHeight)
	{
		for (auto & txTmp : cblock.txs())
		{
			if (CheckTransactionType(txTmp) == kTransactionType_Award)
			{
				nlohmann::json txExtra;
				try
				{
					txExtra = nlohmann::json::parse(txTmp.extra());
				}
				catch(const std::exception& e)
				{
					std::cerr << e.what() << '\n';
					bRollback = true;
					return false;
				}
				
				std::vector<std::string> addrs;
				std::vector<double> onlines;
				std::vector<uint64_t> awardTotals;
				std::vector<uint64_t> signSums;

				for(const auto& info : txExtra["OnlineTime"])
				{
					try
					{
						addrs.push_back(info["Addr"].get<std::string>());
						onlines.push_back(info["OnlineTime"].get<double>());
						awardTotals.push_back(info["AwardTotal"].get<uint64_t>());
						signSums.push_back(info["SignSum"].get<uint64_t>());
					}
					catch(const std::exception& e)
					{
						std::cerr << e.what() << '\n';
						bRollback = true;
						return false;
					}
				}

				if (addrs.size() == 0 || onlines.size() == 0 || awardTotals.size() == 0 || signSums.size() == 0)
				{
					if (cblock.height() != 0)
					{
						error("Get sign node info error !");
						std::cout << "Get sign node info error !" << std::endl;
						bRollback = true;
						return false;
					}
				}
				else
				{
					a_award::AwardAlgorithm awardAlgorithm;
					if ( 0 != awardAlgorithm.Build(blockSignNum, addrs, onlines, awardTotals, signSums) )
					{
						std::cout << "awardAlgorithm.Build() failed!" << std::endl;
						error("awardAlgorithm.Build() failed!");
						bRollback = true;
						return false;
					}

					auto awardInfo = awardAlgorithm.GetDisAward();
					for (auto & award : awardInfo)
					{
						addrAwards.insert(std::make_pair(award.second, award.first));
					}	
				}
				// awardAlgorithm.TestPrint(true);
			}
		}
	}

	// transaction check 
	for (int i = 0; i < cblock.txs_size(); i++)
	{
		CTransaction tx = cblock.txs(i);
		if (!checkTransaction(tx))
		{
			error("checkTransaction(tx)");
			bRollback = true;
			return false;
		}

		bool iscb = CheckTransactionType(tx) == kTransactionType_Fee || CheckTransactionType(tx) == kTransactionType_Award;

		if (iscb && 0 == tx.signprehash_size())
		{
			continue;
		}

		std::string bestChainHash;
		db_status = pRocksDb->GetBestChainHash(txn, bestChainHash);
        if (db_status != 0) {
			error(" pRocksDb->GetBestChainHash");
			bRollback = true;
            return false;
        }
		bool isBestChainHash = bestChainHash.size() != 0;
		if (! isBestChainHash && iscb && 0 == cblock.txs_size())
		{
			continue;
		}

		if (0 == cblock.txs_size() && ! isBestChainHash)
		{
			error("0 == cblock.txs_size() && ! isBestChainHash");
			bRollback = true;
			return false;
		}

		if (isBestChainHash)
		{
			int verifyPreHashCount = 0;
			std::string txBlockHash;

            std::string txHashStr;
            
            for (int i = 0; i < cblock.txs_size(); i++)
            {
                CTransaction transaction = cblock.txs(i);
                if ( CheckTransactionType(transaction) == kTransactionType_Tx)
                {
                    CTransaction copyTx = transaction;
                    for (int i = 0; i != copyTx.vin_size(); ++i)
                    {
                        CTxin * txin = copyTx.mutable_vin(i);
                        txin->clear_scriptsig();
                    }

                    copyTx.clear_signprehash();
                    copyTx.clear_hash();

                    std::string serCopyTx = copyTx.SerializeAsString();

                    size_t encodeLen = serCopyTx.size() * 2 + 1;
                    unsigned char encode[encodeLen] = {0};
                    memset(encode, 0, encodeLen);
                    long codeLen = base64_encode((unsigned char *)serCopyTx.data(), serCopyTx.size(), encode);
                    std::string encodeStr( (char *)encode, codeLen );

                    txHashStr = getsha256hash(encodeStr);
                }
            }

			if (! VerifyTransactionSign(tx, verifyPreHashCount, txBlockHash, txHashStr))
			{
				error("VerifyTransactionSign");
				bRollback = true;
				return false;
			}

			if (verifyPreHashCount < g_MinNeedVerifyPreHashCount)
			{
				error("verifyPreHashCount < g_MinNeedVerifyPreHashCount");
				bRollback = true;
				return false;
			}
		}

		// Get transaction type 
		// bool bIsRedeem = false;
		std::string redempUtxoStr;
		for (int i = 0; i < cblock.txs_size(); i++)
		{
			CTransaction transaction = cblock.txs(i);
			if ( CheckTransactionType(transaction) == kTransactionType_Tx)
			{
				CTransaction copyTx = transaction;

				nlohmann::json txExtra = nlohmann::json::parse(copyTx.extra());
				std::string txType = txExtra["TransactionType"].get<std::string>();

				if (txType == TXTYPE_REDEEM)
				{
					// bIsRedeem = true;

					nlohmann::json txInfo = txExtra["TransactionInfo"].get<nlohmann::json>();
					redempUtxoStr = txInfo["RedeemptionUTXO"].get<std::string>();
				}
			}
		}
		//{{ Redeem time is more than 30 days, 20201214
		// std::cout << "Enter check Redeem time! " << redempUtxoStr << std::endl;
		if (!redempUtxoStr.empty() && cblock.height() > g_compatMinHeight)
		{
			int result = IsMoreThan30DaysForRedeem(redempUtxoStr);
			if (result != 0)
			{
				error("Redeem time is less than 30 days!");
				std::cout << "Redeem time is less than 30 days!" << std::endl;
				bRollback = true;
				return false;
			}
			else
			{
				std::cout << "Redeem time is more than 30 days!" << std::endl;
			}
		}
		//}}

		// Verify that the signature public key is consistent with the base58 address 
		std::vector< std::string > signBase58Addrs;
		for (int i = 0; i < cblock.txs_size(); i++)
		{
			CTransaction transaction = cblock.txs(i);
			if ( CheckTransactionType(transaction) == kTransactionType_Tx)
			{
				// Take out the base58 addresses of all signed accounts 
				for (int k = 0; k < transaction.signprehash_size(); k++) 
                {
                    char buf[2048] = {0};
                    size_t buf_len = sizeof(buf);
                    GetBase58Addr(buf, &buf_len, 0x00, transaction.signprehash(k).pub().c_str(), transaction.signprehash(k).pub().size());
					std::string bufStr(buf);
					signBase58Addrs.push_back( bufStr );
                }
			}
		}

		std::vector<std::string> txOwners;
		uint64_t packageFee = 0;
		uint64_t signFee = 0;
		for (int i = 0; i < cblock.txs_size(); i++)
		{
			CTransaction transaction = cblock.txs(i);
			if ( CheckTransactionType(transaction) == kTransactionType_Tx)
			{
				SplitString(tx.txowner(), txOwners, "_");
				if (txOwners.size() < 1)
				{
					error("txOwners error!");
					bRollback = true;
					return false;
				}

				nlohmann::json extra = nlohmann::json::parse(tx.extra());
				packageFee = extra["PackageFee"].get<uint64_t>();
				signFee = extra["SignFee"].get<uint64_t>();
			}
		}

		for (int i = 0; i < cblock.txs_size(); i++)
		{
			CTransaction transaction = cblock.txs(i);
			if ( CheckTransactionType(transaction) == kTransactionType_Fee)
			{
				// The number of signed accounts is inconsistent with the number of vout, error 
				if( signBase58Addrs.size() != (size_t)transaction.vout_size() )
				{
					error("signBase58Addrs.size() != (size_t)transaction.vout_size()");
					bRollback = true;
					return false;
				}

				//The base58 address is inconsistent and wrong 
				for(int l = 0; l < transaction.vout_size(); l++)
				{
					CTxout txout = transaction.vout(l);	
					auto iter = find(signBase58Addrs.begin(), signBase58Addrs.end(), txout.scriptpubkey());
					if( iter == signBase58Addrs.end() )
					{
						error("iter == signBase58Addrs.end()");
						bRollback = true;
						return false;
					}

					if (txout.value() < 0)
					{
						error("vout error !");
						std::cout << "vout error !" << std::endl;
						bRollback = true;
						return false;
					}

					if (txOwners.end() != find(txOwners.begin(), txOwners.end(), txout.scriptpubkey()))
					{
						if (txout.value() != 0)
						{
							bRollback = true;
							return false;
						}
					}
					else
					{
						if ((uint64_t)txout.value() != signFee && (uint64_t)txout.value() != packageFee)
						{
							error("SignFee or packageFee error !");
							std::cout << "SignFee or packageFee error ! " << std::endl;
							bRollback = true;
							return false;
						}
					}
				}
			}
			else if ( CheckTransactionType(transaction) == kTransactionType_Award )
			{
				uint64_t awardAmountTotal = 0;
				for (auto & txout : transaction.vout())
				{
					// Uint64 is not used to prevent negative values 
					int64_t value = txout.value();
					std::string voutAddr = txout.scriptpubkey();

					if (cblock.height() > g_compatMinHeight)
					{		
						// Initiator account reward is 0 
						if (txOwners.end() != find(txOwners.begin(), txOwners.end(), voutAddr))
						{
							if (value != 0)
							{
								error("Award error !");
								bRollback = true;
								return false;
							}
						}
						else
						{
							// When the reward is negative or greater than the maximum reward value of a single transaction, an error occurs and return 
							if (value < 0 || (uint64_t)value > g_MaxAwardTotal)
							{
								error("Award error !");
								bRollback = true;
								return false;
							}
							else if (value == 0)
							{
								for (int i = 0; i < cblock.txs_size(); i++)
								{
									CTransaction transaction = cblock.txs(i);
									uint32_t count = 0;
									if ( CheckTransactionType(transaction) == kTransactionType_Award)
									{
										for (auto & txout : transaction.vout())
										{
											if (txout.value() == 0)
											{
												count++;
											}
										}

										if (count > 1)
										{
											bRollback = true;
											return false;
										}
									}
								}
							}

							nlohmann::json txExtra;
							try
							{
								txExtra = nlohmann::json::parse(tx.extra());
							}
							catch(const std::exception& e)
							{
								std::cerr << e.what() << '\n';
								bRollback = true;
								return false;
							}
							
							for (nlohmann::json::iterator it = txExtra.begin(); it != txExtra.end(); ++it) 
							{
								if (voutAddr == it.key())
								{
									auto iter = addrAwards.find(voutAddr);
									if (iter == addrAwards.end())
									{
										std::cout << "Transaction award error !" << __LINE__ << std::endl;
										error("Transaction award error !");
										bRollback = true;
										return false;
									}
									else
									{
										if ((uint64_t)value != iter->second)
										{
											std::cout << "Transaction award error !" << __LINE__ << std::endl;
											error("Transaction award error !");
											bRollback = true;
											return false;
										}
									}
								}
							}
						}
					}
					awardAmountTotal += txout.value();
				}

				if (awardAmountTotal > g_MaxAwardTotal)
				{
					error("awardAmountTotal error !");
					bRollback = true;
					return false;
				}
			}
		}
	}


	// The consensus number cannot be less than g_MinNeedVerifyPreHashCount 
	if (cblock.height() > g_compatMinHeight)
	{
		for(auto & tx : cblock.txs())
		{
			if (CheckTransactionType(tx) == kTransactionType_Tx)
			{
				std::vector<std::string> txownersTmp = TxHelper::GetTxOwner(tx);

				for (auto & txout : tx.vout())
				{
					if (txout.value() <= 0 && txownersTmp.end() == find(txownersTmp.begin(), txownersTmp.end(), txout.scriptpubkey()))
					{
						// transaction The amount received by the receiver cannot be 0 
						error("Tx vout error !");
						bRollback = true;
						return false;
					}
					else if (txout.value() < 0 && txownersTmp.end() != find(txownersTmp.begin(), txownersTmp.end(), txout.scriptpubkey()))
					{
						// transaction The initiator's remaining assets can be 0, but cannot be less than 0 
						error("Tx vout error !");
						bRollback = true;
						return false;
					}
				}
			}
			else if ( CheckTransactionType(tx) == kTransactionType_Fee)
			{
				if (tx.vout_size() < g_MinNeedVerifyPreHashCount)
				{
					error("The number of signers is not less than %d !", g_MinNeedVerifyPreHashCount);
					bRollback = true;
					return false;
				}
			}
		}
	}

	return true;
}


std::string CalcBlockHeaderMerkle(const CBlock & cblock)
{
	std::string merkle;
	if (cblock.txs_size() == 0)
	{
		return merkle;
	}

	std::vector<std::string> vTxHashs;
	for (int i = 0; i != cblock.txs_size(); ++i)
	{
		CTransaction tx = cblock.txs(i);
		vTxHashs.push_back(tx.hash());
	}

	unsigned int j = 0, nSize;
    for (nSize = cblock.txs_size(); nSize > 1; nSize = (nSize + 1) / 2)
	{
        for (unsigned int i = 0; i < nSize; i += 2)
		{
            unsigned int i2 = MIN(i+1, nSize-1);

			std::string data1 = vTxHashs[j + i];
			std::string data2 = vTxHashs[j + i2];
			data1 = getsha256hash(data1);
			data2 = getsha256hash(data2);

			vTxHashs.push_back(getsha256hash(data1 + data2));
        }

        j += nSize;
    }

	merkle = vTxHashs[vTxHashs.size() - 1];

	return merkle;
}

void CalcBlockMerkle(CBlock & cblock)
{
	if (cblock.txs_size() == 0)
	{
		return;
	}

	cblock.set_merkleroot(CalcBlockHeaderMerkle(cblock));
}

CBlock CreateBlock(const CTransaction & tx, const std::shared_ptr<TxMsg>& SendTxMsg)
{
	CBlock cblock;

	uint64_t time = Singleton<TimeUtil>::get_instance()->getlocalTimestamp();
    cblock.set_time(time);
	cblock.set_version(0);

	nlohmann::json txExtra = nlohmann::json::parse(tx.extra());
	int NeedVerifyPreHashCount = txExtra["NeedVerifyPreHashCount"].get<int>();
	std::string txType = txExtra["TransactionType"].get<std::string>();

	// Put the number of signatures into the block extension information in Json format 
	nlohmann::json blockExtra;
	blockExtra["NeedVerifyPreHashCount"] = NeedVerifyPreHashCount;

	if (txType == TXTYPE_TX)
	{
		blockExtra["TransactionType"] = TXTYPE_TX;
	}
	else if (txType == TXTYPE_PLEDGE)
	{
		nlohmann::json txInfoTmp = txExtra["TransactionInfo"].get<nlohmann::json>();

		nlohmann::json blockTxInfo;
		blockTxInfo["PledgeAmount"] = txInfoTmp["PledgeAmount"].get<int>();

		blockExtra["TransactionType"] = TXTYPE_PLEDGE;
		blockExtra["TransactionInfo"] = blockTxInfo;
	}
	else if (txType == TXTYPE_REDEEM)
	{
		nlohmann::json txInfoTmp = txExtra["TransactionInfo"].get<nlohmann::json>();

		nlohmann::json blockTxInfo;
		blockTxInfo["RedeemptionUTXO"] = txInfoTmp["RedeemptionUTXO"].get<std::string>();

		blockExtra["TransactionType"] = TXTYPE_REDEEM;
		blockExtra["TransactionInfo"] = blockTxInfo;
	}

	cblock.set_extra( blockExtra.dump() );

    int db_status = 0;
	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if( txn == NULL )
	{
		std::cout << "(CreateBlock) TransactionInit failed !" << std::endl;
	}

	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, true);
	};

	std::string prevBlockHash = SendTxMsg->prevblkhash();
	unsigned int prevBlockHeight = 0;
	if (0 != pRocksDb->GetBlockHeightByBlockHash(txn, prevBlockHash, prevBlockHeight))
	{
		// The parent block does not exist, do not build the block 
		cblock.clear_hash();
		return cblock;
	}

	// The height of the block to be added 
	unsigned int cblockHeight = ++prevBlockHeight;

	unsigned int myTop = 0;
	pRocksDb->GetBlockTop(txn, myTop);
	if ( (myTop  > 9) && (myTop - 9 > cblockHeight))
	{
		cblock.clear_hash();
		return cblock;
	}
	else if (myTop + 1 < cblockHeight)
	{
		cblock.clear_hash();
		return cblock;
	}

	std::string bestChainHash;
	db_status = pRocksDb->GetBestChainHash(txn, bestChainHash);
    if (db_status != 0) 
	{
        std::cout << __LINE__ << std::endl;
    }
	if (bestChainHash.size() == 0)
	{
		cblock.set_prevhash(std::string(64, '0'));
		cblock.set_height(0);
	}
	else
	{
		cblock.set_prevhash(bestChainHash);
		unsigned int preheight = 0;
		db_status = pRocksDb->GetBlockHeightByBlockHash(txn, bestChainHash, preheight);
		if (db_status != 0) 
		{
			error("CreateBlock GetBlockHeightByBlockHash");
		}
		cblock.set_height(preheight + 1);
	}

	CTransaction * tx0 = cblock.add_txs();
	*tx0 = tx;

	if (ENCOURAGE_TX && CheckTransactionType(tx) == kTransactionType_Tx)
	{
		debug("Crreate Encourage TX ... ");

		CTransaction workTx = CreateWorkTx(*tx0);
		if (workTx.hash().empty())
		{
			cblock.clear_hash();
			return cblock;
		}
		CTransaction * tx1 = cblock.add_txs();
		*tx1 = workTx;

        if (get_extra_award_height()) {
            debug("Crreate Encourage TX 2 ... ");
            CTransaction workTx2 = CreateWorkTx(*tx0, true, SendTxMsg);
			if (workTx2.hash().empty())
			{
				cblock.clear_hash();
				return cblock;
			}
            CTransaction * txadd2 = cblock.add_txs();
            *txadd2 = workTx2;
        }
	}

	CalcBlockMerkle(cblock);

	std::string serBlockHeader = cblock.SerializeAsString();
	cblock.set_hash(getsha256hash(serBlockHeader));

	return cblock;
}

bool AddBlock(const CBlock & cblock, bool isSync)
{
	unsigned int preheight;
    int db_status = 0;
	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if( txn == NULL )
	{
		std::cout << "(AddBlock) TransactionInit failed !" << std::endl;
		return false;
	}

	bool bRollback = true;
	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, bRollback);
	};

	db_status = pRocksDb->GetBlockHeightByBlockHash(txn, cblock.prevhash(), preheight);
    if (db_status != 0) {
		bRollback = true;
        return false;
    }

	CBlockHeader block;
	block.set_hash(cblock.hash());
	block.set_prevhash(cblock.prevhash());
	block.set_time(cblock.time());
	block.set_height(preheight +1);

	unsigned int top = 0;
	if (pRocksDb->GetBlockTop(txn, top) != 0)
	{
		bRollback = true;
		return false;
	}

	//Update top and BestChain 
	bool is_mainblock = false;
	if (block.height() > top)  
	{
		is_mainblock = true;
		db_status = pRocksDb->SetBlockTop(txn, block.height());
        if (db_status != 0) {
			bRollback = true;
            return false;
        }
		db_status = pRocksDb->SetBestChainHash(txn, block.hash());
        if (db_status != 0) 
		{
			bRollback = true;
            return false;
        }
	}
	else if (block.height() == top)
	{
		std::string strBestChainHash;
		if (pRocksDb->GetBestChainHash(txn, strBestChainHash) != 0)
		{
			bRollback = true;
			return false;
		}

		std::string strBestChainHeader;
		if (pRocksDb->GetBlockByBlockHash(txn, strBestChainHash, strBestChainHeader) != 0)
		{
			bRollback = true;
			return false;
		}

		CBlock bestChainBlockHeader;
		bestChainBlockHeader.ParseFromString(strBestChainHeader);

		if (cblock.time() < bestChainBlockHeader.time())
		{
			is_mainblock = true;
			db_status = pRocksDb->SetBestChainHash(txn, block.hash());
            if (db_status != 0) 
			{
				bRollback = true;
                return false;
            }
		}
	}
	else if(block.height() < top)
	{
		std::string main_hash;
		pRocksDb->GetBlockHashByBlockHeight(txn, block.height(), main_hash);
		std::string main_block_str;
		if (pRocksDb->GetBlockByBlockHash(txn, main_hash, main_block_str) != 0)
		{
			bRollback = true;
			return false;
		}
		CBlock main_block;
		main_block.ParseFromString(main_block_str);	
		if (cblock.time() < main_block.time())
		{
			is_mainblock = true;
		}
	}

	// uint64_t t3 = Singleton<TimeUtil>::get_instance()->getlocalTimestamp();
	// std::cout << "t3:" << t3 - t2 << std::endl;


	db_status = pRocksDb->SetBlockHeightByBlockHash(txn, block.hash(), block.height());
    if (db_status != 0) 
	{
		bRollback = true;
        return false;
    }
	db_status = pRocksDb->SetBlockHashByBlockHeight(txn, block.height(), block.hash(), is_mainblock);
    if (db_status != 0) 
	{
		bRollback = true;
        return false;
    }
	db_status = pRocksDb->SetBlockHeaderByBlockHash(txn, block.hash(), block.SerializeAsString());
    if (db_status != 0) 
	{
		bRollback = true;
        return false;
    }
	db_status = pRocksDb->SetBlockByBlockHash(txn, block.hash(), cblock.SerializeAsString());
    if (db_status != 0) 
	{
		bRollback = true;
        return false;
    }

	// Determine whether the transaction is a special transaction 
	bool isPledge = false;
	bool isRedeem = false;
	std::string redempUtxoStr;

	nlohmann::json extra = nlohmann::json::parse(cblock.extra());
	std::string txType = extra["TransactionType"].get<std::string>();
	if (txType == TXTYPE_PLEDGE)
	{
		isPledge = true;
	}
	else if (txType == TXTYPE_REDEEM)
	{
		isRedeem = true;
		nlohmann::json txInfo = extra["TransactionInfo"].get<nlohmann::json>();
		redempUtxoStr = txInfo["RedeemptionUTXO"].get<std::string>();
	}
	
	// Calculate the total fuel cost spent 
	uint64_t totalGasFee = 0;
	for (int txCount = 0; txCount < cblock.txs_size(); txCount++)
	{
		CTransaction tx = cblock.txs(txCount);
		if ( CheckTransactionType(tx) == kTransactionType_Fee)
		{
			for (int j = 0; j < tx.vout_size(); j++)
			{
				CTxout txout = tx.vout(j);
				totalGasFee += txout.value();
			}
		}
	}

	if(totalGasFee == 0 && !isRedeem)
	{
		ca_console redColor(kConsoleColor_Red, kConsoleColor_Black, true);
		std::cout << redColor.color() << "tx sign GasFee is 0! AddBlock failed! " << redColor.reset() << std::endl;

		bRollback = true;
		return false;
	}

	for (int i = 0; i < cblock.txs_size(); i++)
	{
		CTransaction tx = cblock.txs(i);
		bool isTx = false;
		if (CheckTransactionType(tx) == kTransactionType_Tx)
		{
			isTx = true;
		}

		for (int j = 0; j < tx.vout_size(); j++)
		{
			CTxout txout = tx.vout(j);

			if (isPledge && isTx)
			{
				if ( !txout.scriptpubkey().compare(VIRTUAL_ACCOUNT_PLEDGE) )
				{
					db_status = pRocksDb->SetPledgeAddresses(txn, TxHelper::GetTxOwner(tx)[0]);
					if (db_status != 0 && db_status != pRocksDb->ROCKSDB_IS_EXIST)
					{
						bRollback = true;
						return false;
					}

					db_status = pRocksDb->SetPledgeAddressUtxo(txn, TxHelper::GetTxOwner(tx)[0], tx.hash()); 
					if (db_status != 0)
					{
						bRollback = true;
						return false;
					}
				}
			}

			if ( txout.scriptpubkey().compare(VIRTUAL_ACCOUNT_PLEDGE) )
			{
				db_status = pRocksDb->SetUtxoHashsByAddress(txn, txout.scriptpubkey(), tx.hash());
				if (db_status != 0 && db_status != pRocksDb->ROCKSDB_IS_EXIST) 
				{
					bRollback = true;
					return false;
				}	
			}
		}

		db_status = pRocksDb->SetTransactionByHash(txn, tx.hash(), tx.SerializeAsString());
        if (db_status != 0) {
			bRollback = true;
            return false;
        }
		db_status = pRocksDb->SetBlockHashByTransactionHash(txn, tx.hash(), cblock.hash());
        if (db_status != 0) {
			bRollback = true;
            return false;
        }

		std::vector<std::string> vPledgeUtxoHashs;
		if (isRedeem && isTx)
		{
			db_status = pRocksDb->GetPledgeAddressUtxo(txn, TxHelper::GetTxOwner(tx)[0], vPledgeUtxoHashs);
			if (db_status != 0) 
			{
				bRollback = true;
				return false;
			}			
		}

		// Determine whether there is a normal utxo part generated by pledge in the vin of transaction 
		nlohmann::json extra = nlohmann::json::parse(tx.extra());
		std::string txType = extra["TransactionType"];
		std::string redempUtxoStr;
		uint64_t packageFee = 0;
		if (txType == TXTYPE_REDEEM)
		{
			nlohmann::json txInfo = extra["TransactionInfo"].get<nlohmann::json>();
			redempUtxoStr = txInfo["RedeemptionUTXO"].get<std::string>();
			packageFee = extra["PackageFee"].get<uint64_t>();
		}

		std::vector<CTxin> txVins;
		uint64_t vinAmountTotal = 0;
		uint64_t voutAmountTotal = 0;

		for (auto & txin : tx.vin())
		{
			txVins.push_back(txin);
		}

		if (txType == TXTYPE_REDEEM)
		{
			for (auto iter = txVins.begin(); iter != txVins.end(); ++iter)
			{
				if (iter->prevout().hash() == redempUtxoStr)
				{
					txVins.erase(iter);
					break;
				}
			}
		}

		std::vector<std::string> utxos;
		for (auto & txin : txVins)
		{
			if (utxos.end() != find(utxos.begin(), utxos.end(), txin.prevout().hash()))
			{
				continue;
			}

			std::string txinAddr = GetBase58Addr(txin.scriptsig().pub());
			vinAmountTotal += TxHelper::GetUtxoAmount(txin.prevout().hash(), txinAddr);
			utxos.push_back(txin.prevout().hash());
		}

		bool bIsUsed = false;
		if (CheckTransactionType(tx) == kTransactionType_Tx && txType == TXTYPE_REDEEM)
		{
			for (int txCount = 0; txCount < cblock.txs_size(); txCount++)
			{
				CTransaction txTmp = cblock.txs(txCount);
				if (CheckTransactionType(txTmp) != kTransactionType_Award)
				{
					for (auto & txout : txTmp.vout())
					{
						voutAmountTotal += txout.value();
					}
				}
			}

			if (voutAmountTotal != vinAmountTotal)
			{
				uint64_t usable = TxHelper::GetUtxoAmount(redempUtxoStr, TxHelper::GetTxOwner(tx)[0]);
				uint64_t redeemAmount = TxHelper::GetUtxoAmount(redempUtxoStr,VIRTUAL_ACCOUNT_PLEDGE);
				if (voutAmountTotal == vinAmountTotal + usable + redeemAmount)
				{
					// This transaction uses the normal part of pledge utxo 
					bIsUsed = true;
				}
				else if (voutAmountTotal == vinAmountTotal + redeemAmount + packageFee)
				{
					bIsUsed = false;
				}
				else
				{
					if (cblock.height() > g_compatMinHeight)
					{
						bRollback = true;
						return false;
					}
				}
				
			}
		}

		// vin
		std::vector<std::string> fromAddrs;
		if (CheckTransactionType(tx) == kTransactionType_Tx)
		{
			// Depledge transaction has duplicate UTXO, De-duplication 
			std::set<std::pair<std::string, std::string>> utxoAddrSet; 
			for (auto & txin : tx.vin())
			{
				std::string addr = GetBase58Addr(txin.scriptsig().pub());

				// transaction recording 
				if ( 0 != pRocksDb->SetAllTransactionByAddress(txn, addr, tx.hash()))
				{
					bRollback = true;
					return false;
				}
				fromAddrs.push_back(addr);

				std::vector<std::string> utxoHashs;
				db_status = pRocksDb->GetUtxoHashsByAddress(txn, addr, utxoHashs);
				if (db_status != 0) 
				{
					bRollback = true;
					return false;
				}

				// Find out whether the utxo used by vin exists in all utxos 
				if (utxoHashs.end() == find(utxoHashs.begin(), utxoHashs.end(), txin.prevout().hash() ) )
				{
					// If it is not found in the available utxo, it is judged whether it is a pledged utxo 
					if (txin.prevout().hash() != redempUtxoStr)
					{
						bRollback = true;
						return false;
					}
					else
					{
						continue;
					}
				}

				std::pair<std::string, std::string> utxoAddr = make_pair(txin.prevout().hash(), addr);
				utxoAddrSet.insert(utxoAddr);
			}

			for (auto & utxoAddr : utxoAddrSet) 
			{
				std::string utxo = utxoAddr.first;
				std::string addr = utxoAddr.second;

				std::string txRaw;
				if (0 != pRocksDb->GetTransactionByHash(txn, utxo, txRaw) )
				{
					bRollback = true;
					return false;
				}

				CTransaction utxoTx;
				utxoTx.ParseFromString(txRaw);

				nlohmann::json extra = nlohmann::json::parse(tx.extra());
				std::string txType = extra["TransactionType"];
				
				if (txType == TXTYPE_PLEDGE && !bIsUsed && utxo == redempUtxoStr)
				{
					continue;
				}

				db_status = pRocksDb->RemoveUtxoHashsByAddress(txn, addr, utxo);
				if (db_status != 0)
				{
					bRollback = true;
					return false;
				}

				// vin minus utxo 
				uint64_t amount = TxHelper::GetUtxoAmount(utxo, addr);
				// std::cout << "Addr : " << addr << "      utxo : " << utxo << "    amt : " << amount << std::endl;
				int64_t balance = 0;
				db_status = pRocksDb->GetBalanceByAddress(txn, addr, balance);
				if (db_status != 0) 
				{
					error("AddBlock:GetBalanceByAddress");
					bRollback = true;
					return false;
				}

				balance -= amount;

				if(balance < 0)
				{
					error("balance < 0");
					bRollback = true;
					return false;
				}

				db_status = pRocksDb->SetBalanceByAddress(txn, addr, balance);
				if (db_status != 0) 
				{
					bRollback = true;
					return false;
				}
			}

			if (isRedeem)
			{
				std::string addr = TxHelper::GetTxOwner(tx)[0];
				db_status = pRocksDb->RemovePledgeAddressUtxo(txn, addr, redempUtxoStr);
				if (db_status != 0)
				{
					bRollback = true;
					return false;
				}
				std::vector<string> utxoes;
				db_status = pRocksDb->GetPledgeAddressUtxo(txn, addr, utxoes);
				if (db_status != 0)
				{
					bRollback = true;
					return false;
				}
				if(utxoes.size() == 0)
				{
					db_status = pRocksDb->RemovePledgeAddresses(txn, addr);
					if (db_status != 0)
					{
						bRollback = true;
						return false;
					}
				}
			}
		}

		// vout
		if ( CheckTransactionType(tx) == kTransactionType_Tx)
		{	
			for (int j = 0; j < tx.vout_size(); j++)
			{
				//voutAdd balance 
				CTxout txout = tx.vout(j);
				std::string vout_address = txout.scriptpubkey();
				int64_t balance = 0;
				db_status = pRocksDb->GetBalanceByAddress(txn, vout_address, balance);
				if (db_status != 0) 
				{
					info("AddBlock:GetBalanceByAddress");
				}
				balance += txout.value();
				db_status = pRocksDb->SetBalanceByAddress(txn, vout_address, balance);
				if (db_status != 0) 
				{
					bRollback = true;
					return false;
				}

				if (isRedeem && 
					tx.vout_size() == 2 && 
					tx.vout(0).scriptpubkey() == tx.vout(1).scriptpubkey())
				{
					if (j == 0)
					{
						db_status = pRocksDb->SetAllTransactionByAddress(txn, txout.scriptpubkey(), tx.hash());
						if (db_status != 0) {
							bRollback = true;
							return false;
						}
					}
				}
				else
				{
					// transaction The initiator has recorded 
					if (fromAddrs.end() != find( fromAddrs.begin(), fromAddrs.end(), txout.scriptpubkey()))
					{
						continue;
					}

					db_status = pRocksDb->SetAllTransactionByAddress(txn, txout.scriptpubkey(), tx.hash());
					if (db_status != 0) {
						bRollback = true;
						return false;
					}
				}
			}
		}
		else
		{
			for (int j = 0; j < tx.vout_size(); j++)
			{
				CTxout txout = tx.vout(j);
				int64_t value = 0;
				db_status = pRocksDb->GetBalanceByAddress(txn, txout.scriptpubkey(), value);
				if (db_status != 0) {
					info("AddBlock:GetBalanceByAddress");
				}
				value += txout.value();
				db_status = pRocksDb->SetBalanceByAddress(txn, txout.scriptpubkey(), value);
				if (db_status != 0) {
					bRollback = true;
					return false;
				}
				db_status = pRocksDb->SetAllTransactionByAddress(txn, txout.scriptpubkey(), tx.hash());
				if (db_status != 0) {
					bRollback = true;
					return false;
				}
			}
		}

		// Accumulate additional rewards 
		if ( CheckTransactionType(tx) == kTransactionType_Award)
		{
			for (int j = 0; j < tx.vout_size(); j++)
			{
				CTxout txout = tx.vout(j);

				uint64_t awardTotal = 0;
				pRocksDb->GetAwardTotal(txn, awardTotal);

				awardTotal += txout.value();
				if ( 0 != pRocksDb->SetAwardTotal(txn, awardTotal) )
				{
					bRollback = true;
					return false;
				}

				if (txout.value() >= 0)
				{
					// Accumulate the total value of additional rewards for the account 
					uint64_t addrAwardTotal = 0;
					if (0 != pRocksDb->GetAwardTotalByAddress(txn, txout.scriptpubkey(), addrAwardTotal))
					{
						error("(AddBlock) GetAwardTotalByAddress failed !");
					}
					addrAwardTotal += txout.value();

					if (0 != pRocksDb->SetAwardTotalByAddress(txn, txout.scriptpubkey(), addrAwardTotal))
					{
						std::cout << "(AddBlock) SetAwardTotalByAddress failed !" << std::endl;
						error("(AddBlock) SetAwardTotalByAddress failed !");
						bRollback = true;
						return false;
					}
				}

				// Accumulate the number of signatures of the account 
				uint64_t signSum = 0;
				if (0 != pRocksDb->GetSignNumByAddress(txn, txout.scriptpubkey(), signSum))
				{
					error("(AddBlock) GetSignNumByAddress failed !");
				}
				++signSum;

				if (0 != pRocksDb->SetSignNumByAddress(txn, txout.scriptpubkey(), signSum))
				{
					std::cout << "(AddBlock) SetSignNumByAddress failed !" << std::endl;
					error("(AddBlock) SetSignNumByAddress failed !");
					bRollback = true;
					return false;
				}
			}
		}
	}

	if( pRocksDb->TransactionCommit(txn) )
	{
		std::cout << "(Addblock) TransactionCommit failed !" << std::endl;
		bRollback = true;
		return false;
	}

	//{{ Delete pending transaction, 20201215
	for (int i = 0; i < cblock.txs_size(); i++)
	{
		CTransaction tx = cblock.txs(i);
		if (CheckTransactionType(tx) == kTransactionType_Tx)
		{
			std::vector<std::string> txOwnerVec;
			SplitString(tx.txowner(), txOwnerVec, "_");
			int result = MagicSingleton<TxVinCache>::GetInstance()->Remove(tx.hash(), txOwnerVec);
			if (result == 0)
			{
				std::cout << "Remove pending transaction in Cache " << tx.hash() << " from ";
				for_each(txOwnerVec.begin(), txOwnerVec.end(), [](const string& owner){ cout << owner << " "; });
				std::cout << std::endl;
			}
		}
	}
	//}}

	//{{ Update the height of the self node, 20210323 Liu
	Singleton<PeerNode>::get_instance()->set_self_chain_height();
	//}}
	
	//{{ Check http callback, 20210324 Liu
	if (Singleton<Config>::get_instance()->HasHttpCallback())
	{
		if (MagicSingleton<CBlockHttpCallback>::GetInstance()->IsRunning())
		{
			MagicSingleton<CBlockHttpCallback>::GetInstance()->AddBlock(cblock);
		}
	}
	//}}
	return true;
}

void CalcTransactionHash(CTransaction & tx)
{
	std::string hash = tx.hash();
	if (hash.size() != 0)
	{
		return;
	}

	CTransaction copyTx = tx;

	copyTx.clear_signprehash();

	std::string serTx = copyTx.SerializeAsString();

	hash = getsha256hash(serTx);
	tx.set_hash(hash);
}

bool ContainSelfSign(const CTransaction & tx)
{
	for (int i = 0; i != tx.signprehash_size(); ++i)
	{
		CSignPreHash signPreHash = tx.signprehash(i);

		char pub[2045] = {0};
		size_t pubLen = sizeof(pub);
		GetBase58Addr(pub, &pubLen, 0x00, signPreHash.pub().c_str(), signPreHash.pub().size());

		if (g_AccountInfo.isExist(pub))
		{
			info("Signer [%s] Has Signed !!!", pub);
			return true;
		}
	}
	return false;
}

bool VerifySignPreHash(const CSignPreHash & signPreHash, const std::string & serTx)
{
	int pubLen = signPreHash.pub().size();
	char * rawPub = new char[pubLen * 2 + 2]{0};
	encode_hex(rawPub, signPreHash.pub().c_str(), pubLen);

	ECDSA<ECP, SHA1>::PublicKey publicKey;
	std::string sPubStr;
	sPubStr.append(rawPub, pubLen * 2);
	SetPublicKey(publicKey, sPubStr);

	delete [] rawPub;

	return VerifyMessage(publicKey, serTx, signPreHash.sign());
}

bool VerifyScriptSig(const CScriptSig & scriptSig, const std::string & serTx)
{
	std::string addr = GetBase58Addr(scriptSig.pub());
	// std::cout << "VerifyScriptSig:" << addr << std::endl;

	int pubLen = scriptSig.pub().size();
	char * rawPub = new char[pubLen * 2 + 2]{0};
	encode_hex(rawPub, scriptSig.pub().c_str(), pubLen);

	ECDSA<ECP, SHA1>::PublicKey publicKey;
	std::string sPubStr;
	sPubStr.append(rawPub, pubLen * 2);
	SetPublicKey(publicKey, sPubStr);

	delete [] rawPub;

	return VerifyMessage(publicKey, serTx, scriptSig.sign());

	//===============
	// ECDSA<ECP, SHA1>::PublicKey publicKey;
	// std::string sPubStr = GetBase58Addr(scriptSig.pub());
	// std::cout << "VerifyScriptSig:" << sPubStr << std::endl;
	// SetPublicKey(publicKey, sPubStr);

	// return VerifyMessage(publicKey, serTx, scriptSig.sign());
}

bool isRedeemTx(const CTransaction &tx)
{
	std::vector<std::string> txOwners = TxHelper::GetTxOwner(tx);
	for (int i = 0; i < tx.vout_size(); ++i)
	{
		CTxout txout = tx.vout(i);
		if (txOwners.end() == find(txOwners.begin(), txOwners.end(), txout.scriptpubkey()))
		{
			return false;
		}
	}
	return true;
}

bool VerifyTransactionSign(const CTransaction & tx, int & verifyPreHashCount, std::string & txBlockHash, std::string txHash)
{
	// TODO blockPrevHash 的
    int db_status = 0;
	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if( txn == NULL )
	{
		std::cout << "(VerifyTransactionSign) TransactionInit failed !" << std::endl;
		return false;
	}

	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, true);
	};

	if ( CheckTransactionType(tx) == kTransactionType_Tx)
	{
		CTransaction copyTx = tx;
		for (int i = 0; i != copyTx.vin_size(); ++i)
		{
			CTxin * txin = copyTx.mutable_vin(i);
			txin->clear_scriptsig();
		}

		copyTx.clear_signprehash();
		copyTx.clear_hash();

		std::string serCopyTx = copyTx.SerializeAsString();
		size_t encodeLen = serCopyTx.size() * 2 + 1;
		unsigned char encode[encodeLen] = {0};
		memset(encode, 0, encodeLen);
		long codeLen = base64_encode((unsigned char *)serCopyTx.data(), serCopyTx.size(), encode);
		std::string encodeStr( (char *)encode, codeLen );

		std::string txHashStr = getsha256hash(encodeStr);
		txBlockHash = txHashStr;

		if(0 != txHashStr.compare(txHash))
		{
			std::cout << "Received txhash value != calculated txhash value  ! " << std::endl;
			return false;
		}
		//Verify the transferor's signature 
		for (int i = 0; i != tx.vin_size(); ++i)
		{
			CTxin txin = tx.vin(i);
			if (! VerifyScriptSig(txin.scriptsig(), txHash))
			{
				printf("Verify TX InputSign failed ... \n");
				return false;
			}
		}
		
		std::vector<std::string> owner_pledge_utxo;
		if (isRedeemTx(tx))
		{
			std::vector<std::string> txOwners = TxHelper::GetTxOwner(tx);
			for (auto i : txOwners)
			{
				std::vector<string> utxos;
				if (0 != pRocksDb->GetPledgeAddressUtxo(txn, i, utxos))
				{
					printf("GetPledgeAddressUtxo failed ... \n");
					return false;
				}

				std::for_each(utxos.begin(), utxos.end(),
						[&](std::string &s){ s = s + "_" + i;}
				);

				std::vector<std::string> tmp_owner = owner_pledge_utxo;
				std::sort(utxos.begin(), utxos.end());
				std::set_union(utxos.begin(),utxos.end(),tmp_owner.begin(),tmp_owner.end(),std::back_inserter(owner_pledge_utxo));
				std::sort(owner_pledge_utxo.begin(), owner_pledge_utxo.end());
			}
		}

		std::vector<std::string> owner_utxo_tmp = TxHelper::GetUtxosByAddresses(TxHelper::GetTxOwner(tx));
		std::sort(owner_utxo_tmp.begin(), owner_utxo_tmp.end());

		std::vector<std::string> owner_utxo;
		std::set_union(owner_utxo_tmp.begin(),owner_utxo_tmp.end(),owner_pledge_utxo.begin(),owner_pledge_utxo.end(),std::back_inserter(owner_utxo));
		std::sort(owner_utxo.begin(), owner_utxo.end());

		std::vector<std::string> tx_utxo = TxHelper::GetUtxosByTx(tx);
    	std::sort(tx_utxo.begin(), tx_utxo.end());

		std::vector<std::string> v_union;
		std::set_union(owner_utxo.begin(),owner_utxo.end(),tx_utxo.begin(),tx_utxo.end(),std::back_inserter(v_union));
		std::sort(v_union.begin(), v_union.end());
		//v_union.erase(unique(v_union.begin(), v_union.end()), v_union.end());

		// Depledge transaction UTXO repeat,De-duplication 
		std::set<std::string> tmpSet(v_union.begin(), v_union.end());
		v_union.assign(tmpSet.begin(), tmpSet.end());

		std::vector<std::string> v_diff;
		std::set_difference(v_union.begin(),v_union.end(),owner_utxo.begin(),owner_utxo.end(),std::back_inserter(v_diff));

		if(v_diff.size() > 0)
		{
			printf("VerifyTransactionSign fail. not have enough utxo!!! \n");
			return false;
		}

		// When judging the mobile phone or RPCtransaction, whether the transaction signer is the transaction initiator 
		std::set<std::string> txVinVec;
		for(auto & vin : tx.vin())
		{
			std::string prevUtxo = vin.prevout().hash();
			std::string strTxRaw;
			db_status = pRocksDb->GetTransactionByHash(txn, prevUtxo, strTxRaw);
			if (db_status != 0)
			{
				return false;
			}

			CTransaction prevTx;
			prevTx.ParseFromString(strTxRaw);
			if (prevTx.hash().size() == 0)
			{
				return false;
			}
			
			std::string vinBase58Addr = GetBase58Addr(vin.scriptsig().pub());
			txVinVec.insert(vinBase58Addr);

			std::vector<std::string> txOutVec;
			for (auto & txOut : prevTx.vout())
			{
				txOutVec.push_back(txOut.scriptpubkey());
			}

			if (std::find(txOutVec.begin(), txOutVec.end(), vinBase58Addr) == txOutVec.end())
			{
				return false;
			}
		}

		std::vector<std::string> txOwnerVec;
		SplitString(tx.txowner(), txOwnerVec, "_");

		std::vector<std::string> tmptxVinSet;
		tmptxVinSet.assign(txVinVec.begin(), txVinVec.end());

		std::vector<std::string> ivec(txOwnerVec.size() + tmptxVinSet.size());
		auto iVecIter = set_symmetric_difference(txOwnerVec.begin(), txOwnerVec.end(), tmptxVinSet.begin(), tmptxVinSet.end(), ivec.begin());
		ivec.resize(iVecIter - ivec.begin());

		if (ivec.size()!= 0)
		{
			return false;
		}
	}
	else
	{
		std::string strBestChainHash;
		db_status = pRocksDb->GetBestChainHash(txn, strBestChainHash);
        if (db_status != 0) 
		{
            return false;
        }
		if (strBestChainHash.size() != 0)
		{
			txBlockHash = COIN_BASE_TX_SIGN_HASH;
		}
	}
	//Verify miner's signature 
	for (int i = 0; i < tx.signprehash_size(); i++)
	{
		CSignPreHash signPreHash = tx.signprehash(i);
		if (! VerifySignPreHash(signPreHash, txBlockHash))
		{
			printf("VerifyPreHashCount  VerifyMessage failed ... \n");
			return false;
		}

		info("Verify PreBlock HashSign succeed !!! VerifySignedCount[%d] -> %s", verifyPreHashCount + 1, txBlockHash.c_str());
		(verifyPreHashCount)++ ;
	}

	return true;
}

unsigned get_extra_award_height() 
{
    const unsigned MAX_AWARD = 500000; //TODO test 10
    unsigned award {0};
    unsigned top {0};
    int db_status = 0;
    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if( txn == NULL )
	{
		std::cout << "(get_extra_award_height) TransactionInit failed !" << std::endl;
	}

	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, true);
	};

    db_status = pRocksDb->GetBlockTop(txn, top);
    if (db_status != 0) 
	{
        return 0;
    }
    auto b_height = top;
    if (b_height <= MAX_AWARD) 
	{
        award = 2000;
    }
    return award;
}


bool IsNeedPackage(const CTransaction & tx)
{
	std::vector<std::string> owners = TxHelper::GetTxOwner(tx);
	return IsNeedPackage(owners);
}

bool IsNeedPackage(const std::vector<std::string> & fromAddr)
{
	bool bIsNeedPackage = true;
	for (auto &account : g_AccountInfo.AccountList)
	{
		if (fromAddr.end() != find(fromAddr.begin(), fromAddr.end(), account.first))
		{
			bIsNeedPackage = false;
		}
	}
	return bIsNeedPackage;
}


int new_add_ouput_by_signer(CTransaction &tx, bool bIsAward, const std::shared_ptr<TxMsg>& msg) 
{
    //Get consensus number 
	nlohmann::json extra = nlohmann::json::parse(tx.extra());
	int needVerifyPreHashCount = extra["NeedVerifyPreHashCount"].get<int>();
	int gasFee = extra["SignFee"].get<int>();
	int packageFee = extra["PackageFee"].get<int>();

    //Additional rewards 
    std::vector<int> award_list;
    int award = 2000000;
    getNodeAwardList(needVerifyPreHashCount, award_list, award);
    auto award_begin = award_list.begin();
    auto award_end = award_list.end();

	std::vector<std::string> signers;
    std::vector<double> online_time;
    std::vector<uint64_t> vec_award_total;
    std::vector<uint64_t> vec_sign_num;
	std::vector<uint64_t> num_arr;

	for (int i = 0; i < tx.signprehash_size(); i++)
	{
        if (bIsAward) 
		{
            CSignPreHash txpre = tx.signprehash(i);
            int pubLen = txpre.pub().size();
            char *rawPub = new char[pubLen * 2 + 2]{0};
            encode_hex(rawPub, txpre.pub().c_str(), pubLen);
            ECDSA<ECP, SHA1>::PublicKey publicKey;
            std::string sPubStr;
            sPubStr.append(rawPub, pubLen * 2);
            SetPublicKey(publicKey, sPubStr);
            delete [] rawPub;

            for (int j = 0; j < msg->signnodemsg_size(); j++) 
			{
                std::string ownPubKey;
                GetPublicKey(publicKey, ownPubKey);
                char SignerCstr[2048] = {0};
                size_t SignerCstrLen = sizeof(SignerCstr);
                GetBase58Addr(SignerCstr, &SignerCstrLen, 0x00, ownPubKey.c_str(), ownPubKey.size());
                auto psignNodeMsg = msg->signnodemsg(j);
                std::string signpublickey = psignNodeMsg.signpubkey();
                const char * signpublickeystr = signpublickey.c_str();

                if (!strcmp(SignerCstr, signpublickeystr)) 
				{
                    std::string temp_signature = psignNodeMsg.signmsg();
                    psignNodeMsg.clear_signmsg();
                    std::string message = psignNodeMsg.SerializeAsString();

                    auto re = VerifyMessage(publicKey, message, temp_signature);
                    if (!re) 
					{
                        cout << __FILE__ << " " << __LINE__ << "VerifyMessage err!!!!!!!" << endl;
						return -1;
					} 
					else 
					{
                        signers.push_back(signpublickeystr);
                        online_time.push_back(psignNodeMsg.onlinetime());
						vec_award_total.push_back(psignNodeMsg.awardtotal());
						vec_sign_num.push_back(psignNodeMsg.signsum());
                    }
                }
            }
        } 
		else 
		{
			bool bIsLocal = false;    // Transaction initiated by this node 
			std::vector<std::string> txOwners = TxHelper::GetTxOwner(tx);
			if (txOwners.size() == 0) 
			{
				continue;
			}

			char buf[2048] = {0};
            size_t buf_len = sizeof(buf);

			CSignPreHash signPreHash = tx.signprehash(i);
			GetBase58Addr(buf, &buf_len, 0x00, signPreHash.pub().c_str(), signPreHash.pub().size());
			signers.push_back(buf);

			if (txOwners[0] == buf)
			{
				bIsLocal = true;
			}

			uint64_t num = 0;
			// When the first signature is the initiator by default 
            if (i == 0)
            {
				if (bIsLocal)
				{
					num = 0;
				}
				else
				{
					num = packageFee;
				}
            }
            else
            {
                if (!bIsAward) 
				{
					num = gasFee;
                } 
				else 
				{
                    num = (*award_begin);
                    ++award_begin;
                    if (award_begin == award_end) break;
                }
            }

            num_arr.push_back(num);
        }
	}

    if (bIsAward) 
	{
        num_arr.push_back(0); //TODO
        a_award::AwardAlgorithm ex_award;
        if ( 0 != ex_award.Build(needVerifyPreHashCount, signers, online_time, vec_award_total, vec_sign_num) )
		{
			error("ex_award.Build error !");
			return -2;
		}
        auto dis_award = ex_award.GetDisAward();
        for (auto v : dis_award) 
		{
            CTxout * txout = tx.add_vout();
            txout->set_value(v.first);
            txout->set_scriptpubkey(v.second);
        }
        // ex_award.TestPrint(true);
        
		// Write the online duration of the signing node into the block to verify the reward value 
		nlohmann::json signNodeInfos;
		uint64_t count = 0;
		for (auto & nodeInfo : msg->signnodemsg())
		{
			nlohmann::json info;
			info["OnlineTime"] = nodeInfo.onlinetime();
			info["AwardTotal"] = nodeInfo.awardtotal();
			info["SignSum"] = nodeInfo.signsum();
			info["Addr"] = nodeInfo.signpubkey();

			signNodeInfos[count] = info;
			// std::string signAddr = GetBase58Addr(nodeInfo.signpubkey());
			// nlohmann::json info;
			// info["OnlineTime"] = nodeInfo.onlinetime();
			// info["AwardTotal"] = nodeInfo.awardtotal();
			// info["SignSum"] = nodeInfo.signsum();
			// signNodeInfos[signAddr] = info.dump();
			++count;
		}

		nlohmann::json awardTxExtra;
		try
		{
			awardTxExtra = nlohmann::json::parse(tx.extra());
		}
		catch(const std::exception& e)
		{
			std::cerr << e.what() << '\n';
		}
		awardTxExtra["OnlineTime"] = signNodeInfos;
		tx.set_extra(awardTxExtra.dump());
    } 
	else 
	{
        for (int i = 0; i < needVerifyPreHashCount; ++i)
        {
            CTxout * txout = tx.add_vout();
            txout->set_value(num_arr[i]);
            txout->set_scriptpubkey(signers[i]);
            info("Transaction signer [%s]", signers[i].c_str());
        }
    }

	return 0;
}

CTransaction CreateWorkTx(const CTransaction & tx, bool bIsAward, const std::shared_ptr<TxMsg>& psignNodeMsg ) 
{
    CTransaction retTx;
    if (tx.vin_size() == 0) {
        return retTx;
    }
	std::string owner = TxHelper::GetTxOwner(tx)[0];
    g_AccountInfo.SetKeyByBs58Addr(g_privateKey, g_publicKey, owner.c_str());
    retTx = tx;

    retTx.clear_vin();
    retTx.clear_vout();
    retTx.clear_hash();


    int db_status = 0;
	(void)db_status;
    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if( txn == NULL )
	{
		std::cout << "(CreateWorkTx) TransactionInit failed !" << std::endl;
	}

	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, true);
	};

    unsigned int txIndex = 0;
    db_status = pRocksDb->GetTransactionTopByAddress(txn, retTx.txowner(), txIndex);
    if (db_status != 0) {
        // std::cout << __LINE__ << std::endl;
    }

    txIndex++;
    retTx.set_n(txIndex);

	uint64_t time = Singleton<TimeUtil>::get_instance()->getlocalTimestamp();
    retTx.set_time(time);
    retTx.set_ip(net_get_self_node_id());

	CTxin ownerTxin = tx.vin(0);

    CTxin * txin = retTx.add_vin();
    txin->set_nsequence(0xffffffff);
    CTxprevout * prevout = txin->mutable_prevout();
    prevout->set_n(0xffffffff);
    CScriptSig * scriptSig = txin->mutable_scriptsig();
    // scriptSig->set_sign("I am coinbase tx");
    // scriptSig->set_pub("");
	*scriptSig = ownerTxin.scriptsig();

    if (!bIsAward) 
	{
        prevout->set_hash(tx.hash());
    }

    if ( 0 != new_add_ouput_by_signer(retTx, bIsAward, psignNodeMsg) )
	{
		retTx.clear_hash();
		return retTx;
	}

    retTx.clear_signprehash();

    std::string serRetTx = retTx.SerializeAsString();
    std::string signature;
    std::string strPub;
    GetSignString(serRetTx, signature, strPub);

    for (int i = 0; i < retTx.vin_size(); i++)
    {
        CTxin * txin = retTx.mutable_vin(i);
        CScriptSig * scriptSig = txin->mutable_scriptsig();

        if (!bIsAward) 
		{
            scriptSig->set_sign(FEE_SIGN_STR);
        } 
		else 
		{
            scriptSig->set_sign(EXTRA_AWARD_SIGN_STR);
        }
        scriptSig->set_pub("");
    }

    CalcTransactionHash(retTx);

    return retTx;
}

void InitAccount(accountinfo *acc, const char *path)
{
	// Default account 
	if (g_testflag)
	{
		g_InitAccount = "1vkS46QffeM4sDMBBjuJBiVkMQKY7Z8Tu";
	}
	else
	{
		g_InitAccount = "16psRip78QvUruQr9fMzr8EomtFS1bVaXk";
	}

	if(NULL == path)
	{
		g_AccountInfo.path =  OWNER_CERT_PATH;
	}
	else
	{
		g_AccountInfo.path =  path;
	}

	if('/' != g_AccountInfo.path[g_AccountInfo.path.size()-1]) 
	{
		g_AccountInfo.path += "/";
	}

	if(access(g_AccountInfo.path.c_str(), F_OK))
    {
        if(mkdir(g_AccountInfo.path.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH | S_IXUSR | S_IXGRP | S_IXOTH ))
        {
            assert(false);
            return;
        }
    }

    if(!acc)
	{
		std::cout<<"InitAccount Failed ..."<<std::endl;
        return;
	}
	
    DIR *dir;
    struct dirent *ptr;

    if ((dir=opendir(g_AccountInfo.path.c_str())) == NULL)
    {
		error("OPEN DIR%s ERROR ..." , g_AccountInfo.path.c_str());
		return;
    }

    while ((ptr=readdir(dir)) != NULL)
    {
        if(strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0)
		{
            continue;
		}
        else
        {
			debug("type[%d] filename[%s]\n", ptr->d_type, ptr->d_name);
            std::string bs58addr;
            if(0 == memcmp(ptr->d_name, OWNER_CERT_DEFAULT, strlen(OWNER_CERT_DEFAULT)))
            {
				std::string name(ptr->d_name);
				char *p = ptr->d_name + name.find('.') + 1;
				std::string ps(p);
                bs58addr.append(ptr->d_name, p + ps.find('.') - ptr->d_name );
                acc->GenerateKey(bs58addr.c_str(), true);
            }
            else
            {
				std::string name(ptr->d_name);
                bs58addr.append(ptr->d_name, ptr->d_name + name.find('.') - ptr->d_name);
                acc->GenerateKey(bs58addr.c_str(), false);
            }
        }
    }
	closedir(dir);

	if(!g_AccountInfo.GetAccSize())
    {
        g_AccountInfo.GenerateKey();
    }

	return;
}

std::string GetDefault58Addr()
{
	string sPubStr;
	GetPublicKey(g_publicKey, sPubStr);
	return GetBase58Addr(sPubStr);
}

/**
    The last column of the vector is the total reward 
    The total amount is (consensus number minus one) multiplied by (base) 
*/
int getNodeAwardList(int consensus, std::vector<int> &award_list, int amount, float coe) 
{
    using namespace std;

    //*Reward distribution 
    amount = amount*coe; //TODO
    consensus -= 1;
    consensus = consensus == 0 ? 1 : consensus;
    //auto award = consensus * base;
    int base {amount/consensus}; //Evenly divide the assets, there will be surplus 
    int surplus = amount - base*consensus; //More than 
    award_list.push_back(amount);
    for (auto i = 1; i <= consensus; ++i) 
	{ //Initialization starts from 1 and removes the total 
        award_list.push_back(base);
    }
    award_list[consensus] += surplus;

    //Interest rate distribution 
    auto list_end_award {0};
    for (auto i = 1; i < consensus; ++i) 
	{
        award_list[i] -= i;
        list_end_award += i;
    }

    auto temp_consensus = consensus;
    auto diff_value = 10; //Final value difference (the larger the value, the greater the difference) 
    while (list_end_award > diff_value) 
	{
        if (list_end_award > diff_value && list_end_award < consensus) 
		{
            consensus = diff_value;
        }
        for (auto i = 1; i < consensus; ++i) 
		{ //XXX
            award_list[i] += 1; //XXX
        }
        if (list_end_award < consensus) 
		{
            list_end_award -= diff_value;
        } 
		else 
		{
            list_end_award -= consensus-1;
        }

    }

    award_list[temp_consensus] += list_end_award;
    sort(award_list.begin(), award_list.end());

    //Remove negative numbers 
    while (award_list[0] <= 0) 
	{ //Symmetrical Negative Value 
        for (auto i = 0; i < temp_consensus - 1; ++i) 
		{
            if (award_list[i] <= 0) 
			{
                if (award_list[i] == 0) 
				{
                    award_list[i] = 1;
                    award_list[temp_consensus-1-i] -= 1;
                } 
				else 
				{
                    award_list[i] = abs(award_list[i]) + 1;
                    award_list[temp_consensus-1-i] -= award_list[i] + (award_list[i] - 1);
                }
            } 
			else 
			{
                break;
            }
        }

        sort(award_list.begin(), award_list.end());
    }

    //The last reward cannot be equal to the previous one  XXX
    while (award_list[temp_consensus-1] == award_list[temp_consensus-2]) 
	{
        award_list[temp_consensus-1] += 1;
        award_list[temp_consensus-2] -= 1;
        sort(award_list.begin(), award_list.end());
    }

    if (amount == 0) 
	{
        for (auto &v : award_list) 
		{
            v = 0;
        }
    }

    return 1;
}

bool ExitGuardian()
{
	std::string name = "ebpc_daemon";
	char cmd[128];
	memset(cmd, 0, sizeof(cmd));

	sprintf(cmd, "ps -ef | grep %s | grep -v grep | awk '{print $2}' | xargs kill -9 ",name.c_str());
	system(cmd);
	return true;
}


void HandleBuileBlockBroadcastMsg( const std::shared_ptr<BuileBlockBroadcastMsg>& msg, const MsgData& msgdata )
{
	// Determine whether the version is compatible 
	if( 0 != Util::IsVersionCompatible( msg->version() ) )
	{
		error("HandleBuileBlockBroadcastMsg IsVersionCompatible");
		return ;
	}

	std::string serBlock = msg->blockraw();
	CBlock cblock;
	cblock.ParseFromString(serBlock);

	MagicSingleton<BlockPoll>::GetInstance()->Add(Block(cblock));
}

// Create: receive pending transaction from network and add to cache,  20210114   Liu
void HandleTxPendingBroadcastMsg(const std::shared_ptr<TxPendingBroadcastMsg>& msg, const MsgData& msgdata)
{
	// Determine whether the version is compatible 
	if (Util::IsVersionCompatible(msg->version()) != 0)
	{
		error("HandleTxPendingBroadcastMsg IsVersionCompatible");
		return ;
	}

	std::string transactionRaw = msg->txraw();
	CTransaction tx;
	tx.ParseFromString(transactionRaw);
	int result = MagicSingleton<TxVinCache>::GetInstance()->Add(tx, false);
	cout << "Receive pending transaction broadcast message ^^^^^VVVVV " << result << endl;
}


int VerifyBuildBlock(const CBlock & cblock)
{
	// Check whether the signature node has an abnormal account 
	std::vector<std::string> addrList;
	if ( 0 != GetAbnormalAwardAddrList(addrList) )
	{
		error("GetAbnormalAwardAddrList failed!");
		return -1;
	}

	for (auto & tx : cblock.txs())
	{
		if (CheckTransactionType(tx) == kTransactionType_Award)
		{
			for (auto & txout : tx.vout())
			{
				if (addrList.end() != find(addrList.begin(), addrList.end(), txout.scriptpubkey()))
				{
					std::vector<std::string> txOwnerVec;
					SplitString(tx.txowner(), txOwnerVec, "_");

					if (txOwnerVec.end() == find(txOwnerVec.begin(), txOwnerVec.end(), txout.scriptpubkey()))
					{
						if (txout.value() != 0)
						{
							std::cout << "sign addr Abnormal !" << std::endl;
							error("sign addr Abnormal !");
							return -2;
						}
					}
				}
			}
		}
	}

	return 0;
}

int BuildBlock(std::string &recvTxString, const std::shared_ptr<TxMsg>& SendTxMsg)
{
	CTransaction tx;
	tx.ParseFromString(recvTxString);

	if (! checkTransaction(tx))
	{
		error("BuildBlock checkTransaction");
		return -1;
	}
	CBlock cblock = CreateBlock(tx, SendTxMsg);
	if (cblock.hash().empty())
	{
		return -6;
	}

	std::string serBlock = cblock.SerializeAsString();

	if ( 0 != VerifyBuildBlock(cblock) ) 
	{
		error("VerifyBuildBlock failed ! ");
		return -2;
	}
	//Verify legitimacy 
	bool ret = VerifyBlockHeader(cblock);

	if(!ret)
	{
		error("BuildBlock VerifyBlockHeader fail!!!");
		return -3;
	}
	if(MagicSingleton<BlockPoll>::GetInstance()->CheckConflict(cblock))
	{
		error("BuildBlock BlockPoll have CheckConflict!!!");
		return -4;
	}

	if (MagicSingleton<TxVinCache>::GetInstance()->IsBroadcast(tx.hash()))
	{
		std::cout << "Block has already broadcast ^^^^VVVV" << std::endl;
		return -5;
	}

	BuileBlockBroadcastMsg buileBlockBroadcastMsg;
	buileBlockBroadcastMsg.set_version(getVersion());
	buileBlockBroadcastMsg.set_blockraw(serBlock);

	net_broadcast_message<BuileBlockBroadcastMsg>(buileBlockBroadcastMsg, net_com::Priority::kPriority_High_1);

	MagicSingleton<TxVinCache>::GetInstance()->SetBroadcastFlag(tx.hash());
	// Add transaction broadcast time, 20210506   Liu
	MagicSingleton<TxVinCache>::GetInstance()->UpdateTransactionBroadcastTime(tx.hash());
	
	info("BuildBlock BuileBlockBroadcastMsg");
	return 0;
}

int IsBlockExist(const std::string & blkHash)
{
	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if (!txn)
	{
		error("TransactionInit error !");
		return -1;
	}

	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, true);
	};

	unsigned int blockHeight = 0;
	if (0 != pRocksDb->GetBlockHeightByBlockHash(txn, blkHash, blockHeight))
	{
		error("GetBlockHeightByBlockHash error !");
		return -2;
	}

	return 0;
}

int CalcTxTryCountDown(int needVerifyPreHashCount)
{
	if (needVerifyPreHashCount < g_MinNeedVerifyPreHashCount)
	{
		return 0;
	}
	else
	{
		return needVerifyPreHashCount - 4;
	}
}

int GetTxTryCountDwon(const TxMsg & txMsg)
{
	return txMsg.trycountdown();
}

int GetLocalDeviceOnlineTime(double_t & onlinetime)
{
	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if( txn == NULL )
	{
		std::cout << "(GetLocalDeviceOnlineTime) TransactionInit failed !" << std::endl;
		return -1;
	}

	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, true);
	};

	int ret = 0;
	std::string ownPubKey = GetDefault58Addr();
	std::vector<string> pledgeUtxos;
	if (0 != pRocksDb->GetPledgeAddressUtxo(txn, ownPubKey, pledgeUtxos))
	{
		ret = 1;
	}

	uint64_t pledgeTime = time(NULL);
	uint64_t startTime = pledgeTime;
	for (auto & hash : pledgeUtxos)
	{
		std::string txRaw;
		if (0 != pRocksDb->GetTransactionByHash(txn, hash, txRaw))
		{
			ret = -2;
		}

		CTransaction utxoTx;
		utxoTx.ParseFromString(txRaw);

		for (auto & txout : utxoTx.vout())
		{
			if (txout.scriptpubkey() == VIRTUAL_ACCOUNT_PLEDGE)
			{
				if (txout.value() > (int64_t)g_TxNeedPledgeAmt && utxoTx.time() < pledgeTime)
				{
					pledgeTime = utxoTx.time();
				}
			}
		}
	}
	
	if (pledgeTime == startTime)
	{
		onlinetime = 1;
	}
	else
	{
		onlinetime = (time(NULL) - pledgeTime) / 3600 / 24;
		onlinetime = onlinetime > 1 ? onlinetime : 1;
	}

	return ret;
}

int SendTxMsg(const CTransaction & tx, const std::shared_ptr<TxMsg>& msg, uint32_t number)
{
	// The id of all signed nodes 
	std::vector<std::string> signedIds;  
	for (auto & item : msg->signnodemsg())
	{
		signedIds.push_back( item.id() );
	}

	std::vector<std::string> sendid;
	int ret = FindSignNode(tx, number, signedIds, sendid);
	if( ret < 0 )
	{
		return -1;
	}

	for (auto id : sendid)
	{
		net_send_message<TxMsg>(id.c_str(), *msg, net_com::Priority::kPriority_High_1);
	}

	return 0;
}

int RetrySendTxMsg(const CTransaction & tx, const std::shared_ptr<TxMsg>& msg)
{
	int tryCountDown = msg->trycountdown();
	tryCountDown--;
	msg->set_trycountdown(tryCountDown);
	if (tryCountDown <= 0)
	{
		return -1;
	}
	else
	{
		// Keep trying to forward 
		SendTxMsg(tx, msg, 1);
	}

	return 0;
}

int AddSignNodeMsg(const std::shared_ptr<TxMsg>& msg)
{
	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if( txn == NULL )
	{
		return -1;
	}

	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, true);
	};

	// Whether there are enough staking nodes 
	bool isEnoughPledgeNode = false;
	uint32_t pledegeNodeCount = 0;
	std::vector<std::string> addressVec;
	pRocksDb->GetPledgeAddress(txn, addressVec);

	for (auto & addr : addressVec)
	{
		uint64_t pledgeamount = 0;
		SearchPledge(addr, pledgeamount);
		if (pledgeamount >= g_TxNeedPledgeAmt)
		{
			pledegeNodeCount++;
		}
		if (pledegeNodeCount > g_minPledgeNodeNum)
		{
			isEnoughPledgeNode = true;
			break;
		}
	}

	double_t onlinetime = 0.0;
	if (0 > GetLocalDeviceOnlineTime(onlinetime))
	{
		if (isEnoughPledgeNode)
		{
			return -2;
		}
	}

	uint64_t mineSignatureFee = 0;
	pRocksDb->GetDeviceSignatureFee( mineSignatureFee );
	if(mineSignatureFee <= 0)
	{
		return -3;
	}

	std::string default58Addr = GetDefault58Addr();

	uint64_t addrAwardTotal = 0;
	pRocksDb->GetAwardTotalByAddress(txn, default58Addr, addrAwardTotal);
	
	uint64_t signSum = 0;
	pRocksDb->GetSignNumByAddress(txn, default58Addr, signSum);

	SignNodeMsg * psignNodeMsg = msg->add_signnodemsg();
	psignNodeMsg->set_id(net_get_self_node_id());
	psignNodeMsg->set_signpubkey( default58Addr );
	psignNodeMsg->set_gasfee( std::to_string( mineSignatureFee ) );
	psignNodeMsg->set_onlinetime(onlinetime);
	psignNodeMsg->set_awardtotal(addrAwardTotal);
	psignNodeMsg->set_signsum(signSum);

	std::string ser = psignNodeMsg->SerializeAsString();
	std::string signatureMsg;
	std::string strPub;
	GetSignString(ser, signatureMsg, strPub);
	psignNodeMsg->set_signmsg(signatureMsg);

	return 0;
}

int CheckTxMsg( const std::shared_ptr<TxMsg>& msg )
{
	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if (txn == NULL)
	{
		return -1;
	}

	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, true);
	};

	//Take transaction body 
	CTransaction tx;
	tx.ParseFromString(msg->tx());

	// Take the transaction initiator 
	std::vector<std::string> vTxOwners = TxHelper::GetTxOwner(tx);

	// Get the signer in the transaction body 
	std::vector<std::string> vMsgSigners;
	for (const auto & signer : msg->signnodemsg())
	{
		vMsgSigners.push_back(signer.signpubkey());
	}

	// Take the signer in the transaction body 
	std::vector<std::string> vTxSigners;
	for (const auto & signInfo : tx.signprehash())
	{
		std::string addr = GetBase58Addr(signInfo.pub());
		vTxSigners.push_back(addr);
	}

	std::sort(vMsgSigners.begin(), vMsgSigners.end());
	std::sort(vTxSigners.begin(), vTxSigners.end());

	// Comparison difference 
	if (vMsgSigners != vTxSigners)
	{
		return -2;
	}

	// Take the transaction type 
	bool bIsPledgeTx = false;
	auto extra = nlohmann::json::parse(tx.extra());
	std::string txType = extra["TransactionType"].get<std::string>();
	if ( txType == TXTYPE_PLEDGE )
	{
		bIsPledgeTx = true;
	}

	// Get the entire network pledge account 
	std::vector<string> pledgeAddrs;
	pRocksDb->GetPledgeAddress(txn, pledgeAddrs);

	// Determine whether it is the initial account transaction 
	bool bIsInitAccount = false;
	if (vTxOwners.end() != find(vTxOwners.begin(), vTxOwners.end(), g_InitAccount))
	{
		bIsInitAccount = true;
	}

	// Determine whether the signature node needs to be pledged 
	if ( (bIsPledgeTx || bIsInitAccount) && pledgeAddrs.size() < g_minPledgeNodeNum )
	{
		return 0;
	}

	// Determine the pledge amount of the signature node 
	for (auto & addr : vTxSigners)
	{
		// The initiator does not make a pledge judgment 
		if (vTxOwners.end() != std::find(vTxOwners.begin(), vTxOwners.end(), addr))
		{
			continue;
		}

		uint64_t amount = 0;
		SearchPledge(addr, amount);
		if (amount < g_TxNeedPledgeAmt)
		{
			return -3;
		}
	}
	return 0;
}

void HandleTx( const std::shared_ptr<TxMsg>& msg, const MsgData& msgdata)
{
	std::string tx_hash;
	DoHandleTx( msg, tx_hash);
}

int DoHandleTx( const std::shared_ptr<TxMsg>& msg, std::string & tx_hash )
{
	// Determine whether the version is compatible 
	if( 0 != Util::IsVersionCompatible( msg->version() ) )
	{
		return -1;
	}
	
	// Determine whether the height meets 
	if(!checkTop(msg->top()))
	{
		return -2;
	}

	// Check whether the parent block of the transaction exists on this node 
	if (IsBlockExist(msg->prevblkhash()))
	{
		return -3;
	}

	std::cout << "Recv TX ..." << std::endl;

	//The header of TX carries the id of the signed network node, in the format  num [nodeid,nodeid,...] tx // Can be deleted 
	CTransaction tx;
	tx.ParseFromString(msg->tx());

	// Consensus number of this transaction 
	auto extra = nlohmann::json::parse(tx.extra());
    int needVerifyPreHashCount = extra["NeedVerifyPreHashCount"].get<int>();
	if (needVerifyPreHashCount < g_MinNeedVerifyPreHashCount)
	{
		return -4;
	}

	CalcTransactionHash(tx);

	if (! checkTransaction(tx))
	{
		return -5;
	}

	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if( txn == NULL )
	{
		return -6;
	}

	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, true);
	};

	// The id of all signed nodes 
	std::vector<std::string> signedIds;
	for (auto & item : msg->signnodemsg())
	{
		signedIds.push_back( item.id() );
		if (item.onlinetime() <= 0)
		{
			return -7;
		}
	}

	if (0 != CheckTxMsg(msg))
	{
		return -8;
	}

	//Verify someone else's signature 
	int verifyPreHashCount = 0;
	std::string txBlockHash;
	std::string blockPrevHash;
	bool rc = VerifyTransactionSign(tx, verifyPreHashCount, txBlockHash, msg->txencodehash());
	if(!rc)
	{
		if(tx.signprehash_size() == 0)
		{
			return -9;
		}
		else
		{
			return (0 != RetrySendTxMsg(tx, msg) ? -9 : 0);
		}
	}

	std::cout << "verifyPreHashCount: " << verifyPreHashCount << std::endl;

	if ( verifyPreHashCount < needVerifyPreHashCount && ContainSelfSign(tx))
	{
		return (0 != RetrySendTxMsg(tx, msg) ? -10 : 0);
	}

	// Determine whether it is a pledge transaction 
	bool isPledgeTx = false;
	std::string txType = extra["TransactionType"].get<std::string>();
	if (txType == TXTYPE_PLEDGE)
	{
		isPledgeTx = true;
	}

	// Determine whether it is a transaction initiated by the initial account 
	bool isInitAccountTx = false;
	for (int i = 0; i < tx.vout_size(); ++i)
	{
		CTxout txout = tx.vout(i);
		if (txout.scriptpubkey() == g_InitAccount)
		{
			isInitAccountTx = true;
		}
	}

	//Signed transfer transactions are not allowed if the account is not pledged, but signed pledged transactions are allowed  
	// verifyPreHashCount == 0 时为自己发起transaction 允许签名，verifyPreHashCount == needVerifyPreHashCount 时签名已经足够 开始建块
	if (!isPledgeTx && !isInitAccountTx && (verifyPreHashCount != 0 && verifyPreHashCount != needVerifyPreHashCount) )
	{
		std::string defauleAddr = GetDefault58Addr();

		uint64_t amount = 0;
		SearchPledge(defauleAddr, amount);
		if (amount < g_TxNeedPledgeAmt && defauleAddr != g_InitAccount)
		{
			return (0 != RetrySendTxMsg(tx, msg) ? -11 : 0);
		}
	}

	// transaction Recipient prohibits signing 
	std::string default58Addr = GetDefault58Addr();
	for(int i = 0; i < tx.vout_size(); i++)
	{
		CTxout txout = tx.vout(i);
		if(default58Addr == txout.scriptpubkey() && default58Addr != TxHelper::GetTxOwner(tx)[0])
		{
			return (0 != RetrySendTxMsg(tx, msg) ? -12 : 0);
		}
	}

	// Self-signed 
	if ( verifyPreHashCount < needVerifyPreHashCount)
	{
		tx_hash = tx.hash();
		//Sign your own 
		std::string strSignature;
		std::string strPub;
		GetSignString(txBlockHash, strSignature, strPub);

		std::cout << "[" << GetDefault58Addr() << "] add Sign ..." << endl;

		CSignPreHash * signPreHash = tx.add_signprehash();
		signPreHash->set_sign(strSignature);
		signPreHash->set_pub(strPub);
		verifyPreHashCount++;
	}
	
	uint64_t mineSignatureFee = 0;
	pRocksDb->GetDeviceSignatureFee( mineSignatureFee );
	if(mineSignatureFee <= 0)
	{
		if (tx.signprehash_size() == 1)
		{
			// The initiator must set a mining fee 
			return -13;
		}
		else
		{
			// transaction The transfer node can retry sending 
			return (0 != RetrySendTxMsg(tx, msg) ? -13 : 0);
		}
	}

	std::string ownID = net_get_self_node_id();
	int txOwnerPayGasFee = extra["SignFee"].get<int>();
	if (ownID != tx.ip())
	{
		// transaction If the commission paid by the initiator is lower than the signature fee set by this node, no signature will be given 
		if(verifyPreHashCount != 0 && ((uint64_t)txOwnerPayGasFee) < mineSignatureFee )
		{
			return (0 != RetrySendTxMsg(tx, msg) ? -14 : 0);
		}
	}

	if((uint64_t)txOwnerPayGasFee < g_minSignFee || (uint64_t)txOwnerPayGasFee > g_maxSignFee)
	{
		return -15;
	}
 
	std::string serTx = tx.SerializeAsString();
	msg->set_tx(serTx);

	if (verifyPreHashCount < needVerifyPreHashCount)
	{
		// When the number of signatures is insufficient 

		// Add signature information for transaction circulation 		
		if (0 != AddSignNodeMsg(msg))
		{
			return -16;
		}

		if (!CheckAllNodeChainHeightInReasonableRange())
		{
			std::cout << "Check chain height of node failed, ^^^VVV" << std::endl;
			return -22;
		}
		
		//There is only one signature, the signature is itself, the ip is equal, and it is added to the Cache , 20201214
		if (ownID == tx.ip() && tx.signprehash_size() == 1)
		{
			char buf[2048] = {0};
			size_t buf_len = sizeof(buf);
			string pub = tx.signprehash(0).pub();
			GetBase58Addr(buf, &buf_len, 0x00, pub.c_str(), pub.size());
			std::string strBuf(buf, strlen(buf));

			std::string default58Addr = GetDefault58Addr();
			if (strBuf == default58Addr)
			{
				int result = MagicSingleton<TxVinCache>::GetInstance()->Add(tx);
				std::cout << "Transaction add to Cache ^^^^^VVVVV " << result << " " << TxVinCache::TxToString(tx) << std::endl;
			}
		}

		int nodeSize = needVerifyPreHashCount * 1;
		if(verifyPreHashCount > 1)   
		{
			// In addition to its own signature, the number of forwarding nodes is 1 when other nodes sign 
			nodeSize = 1;
		}

		if (0 != SendTxMsg(tx, msg, nodeSize))
		{
			return -17;
		}
		
		std::cout << "TX begin broadcast" << std::endl;
	}
	else
	{
		// The number of signatures reaches the consensus number 
		std::string ip = net_get_self_node_id();

		if (ip != tx.ip())
		{
			// If it is the number of nodes pledged in the whole network 
			
			// Add signature information for transaction circulation 	
			if (0 != AddSignNodeMsg(msg))
			{
				return -18;
			}

			std::cout << "Send to ip[" << tx.ip().c_str() << "] to CreateBlock ..." << std::endl;
			net_send_message<TxMsg>(tx.ip(), *msg, net_com::Priority::kPriority_High_1);

			std::cout << "TX send to Create Block" << std::endl;
		}
		else
		{
			// Return to the originating node 
			std::string blockHash;
			pRocksDb->GetBlockHashByTransactionHash(txn, tx.hash(), blockHash);
			
			if(blockHash.length())
			{
				// Inquired that the description has been added 
				return -19;
			}

			if(msg->signnodemsg_size() != needVerifyPreHashCount)
			{
				return -20;
			}

			if( 0 != BuildBlock( serTx, msg) )
			{
				std::cout << "HandleTx BuildBlock fail" << std::endl;
				return -21;
			}
		}
	}
	return 0;
}

std::map<int32_t, std::string> GetPreTxRawCode()
{
	std::map<int32_t, std::string> errInfo = {make_pair(0, "success "), 
												make_pair(-1, "Incompatible version "),
												make_pair(-2, "Database open error "),
												make_pair(-2, "Failed to obtain main chain information "),
												};
	return errInfo;
}

void HandlePreTxRaw( const std::shared_ptr<TxMsgReq>& msg, const MsgData& msgdata )
{
	TxMsgAck txMsgAck;

	auto errInfo = GetPreTxRawCode();

	// Determine whether the version is compatible 
	if( 0 != Util::IsVersionCompatible( msg->version() ) )
	{
		ReturnAckCode<TxMsgAck>(msgdata, errInfo, txMsgAck, -1);
		return ;
	}

	// Reverse base64 of transaction information body, public key, and signature information 
	unsigned char serTxCstr[msg->sertx().size()] = {0};
	unsigned long serTxCstrLen = base64_decode((unsigned char *)msg->sertx().data(), msg->sertx().size(), serTxCstr);
	std::string serTxStr((char *)serTxCstr, serTxCstrLen);

	CTransaction tx;
	tx.ParseFromString(serTxStr);

	unsigned char strsignatureCstr[msg->strsignature().size()] = {0};
	unsigned long strsignatureCstrLen = base64_decode((unsigned char *)msg->strsignature().data(), msg->strsignature().size(), strsignatureCstr);

	unsigned char strpubCstr[msg->strsignature().size()] = {0};
	unsigned long strpubCstrLen = base64_decode((unsigned char *)msg->strpub().data(), msg->strpub().size(), strpubCstr);

	for (int i = 0; i < tx.vin_size(); i++)
	{
		CTxin * txin = tx.mutable_vin(i);
		txin->mutable_scriptsig()->set_sign( std::string( (char *)strsignatureCstr, strsignatureCstrLen ) );
		txin->mutable_scriptsig()->set_pub( std::string( (char *)strpubCstr, strpubCstrLen ) );
	}

	std::string serTx = tx.SerializeAsString();

	auto extra = nlohmann::json::parse(tx.extra());
	int needVerifyPreHashCount = extra["NeedVerifyPreHashCount"].get<int>();

	TxMsg phoneToTxMsg;
	phoneToTxMsg.set_version(getVersion());
	phoneToTxMsg.set_tx(serTx);
	phoneToTxMsg.set_txencodehash(msg->txencodehash());

	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if( txn == NULL )
	{
		ReturnAckCode<TxMsgAck>(msgdata, errInfo, txMsgAck, -2);
		return;
	}

	bool bRollback = true;
	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, bRollback);
	};

	std::string blockHash;
    if ( 0 != pRocksDb->GetBestChainHash(txn, blockHash) )
    {
    	ReturnAckCode<TxMsgAck>(msgdata, errInfo, txMsgAck, -3);
        return;
    }
    phoneToTxMsg.set_prevblkhash(blockHash);
	phoneToTxMsg.set_trycountdown(CalcTxTryCountDown(needVerifyPreHashCount));

	unsigned int top = 0;
	pRocksDb->GetBlockTop(txn, top);	
	phoneToTxMsg.set_top(top);

	auto txmsg = make_shared<TxMsg>(phoneToTxMsg);
	std::string txHash;
	int ret = DoHandleTx(txmsg, txHash);
	txMsgAck.set_txhash(txHash);
	if (ret != 0)
	{
		ret -= 100;
	}

	std::cout << "ret: " << ret << std::endl;

	ReturnAckCode<TxMsgAck>(msgdata, errInfo, txMsgAck, ret);
}

/* ====================================================================================  
 # Mobile terminal transaction process: 
  #1. The mobile phone sends a CreateTxMsgReq request to the PC, and the PC calls the HandleCreateTxInfoReq interface to process the mobile phone request;
  #2. After the key transaction information sent by the PC through the mobile phone in HandleCreateTxInfoReq is packaged into a transaction information body, and the transaction information body is base64,
  # Pass the CreateTxMsgAck protocol back to the mobile phone. The txHash field in the CreateTxMsgAck protocol is entered after the base64 of the transaction body.
  # Line sha256, the obtained hash value
  #3. After receiving CreateTxMsgAck on the mobile phone, it compares the hash calculated by itself with the txHash transmitted from the PC. Inconsistencies indicate that the data is incorrect; if they are consistent, adjust
  # Call interface_NetMessageReqTxRaw to sign the hash value.
  # 4, the mobile phone sends back TxMsgReq to the PC, and the PC processes the received transaction from the mobile phone through the HandlePreTxRaw interface 
 ==================================================================================== */

 std::map<int32_t, std::string> GetCreateTxInfoReqCode()
{
	std::map<int32_t, std::string> errInfo = {make_pair(0, "success "), 
												make_pair(-1, "Incompatible version "),
												make_pair(-2, "Parameter error "),
												make_pair(-3, "No related utxo found "),
												};
	return errInfo;
}
// Mobile transaction processing 
void HandleCreateTxInfoReq( const std::shared_ptr<CreateTxMsgReq>& msg, const MsgData& msgdata )
{
	CreateTxMsgAck createTxMsgAck;
	auto errInfo = GetCreateTxInfoReqCode();

	// Determine whether the version is compatible 
	if( 0 != Util::IsVersionCompatible( msg->version() ) )
	{
		ReturnAckCode<CreateTxMsgAck>(msgdata, errInfo, createTxMsgAck, -1);		
		return ;
	}

	// Create the transaction body from the data sent by the mobile phone 
	std::string txData;
	int ret = CreateTransactionFromRocksDb(msg, txData);
	if( 0 != ret )
	{
		std::string sendMessage;
		int code = -2;
		if(ret == -1)
		{
			sendMessage = "parameter error!";
		}
		else
		{
			code = -3;
			sendMessage = "UTXO not found!";
		}
		
		ReturnAckCode<CreateTxMsgAck>(msgdata, errInfo, createTxMsgAck, code);
		return ;
	}

	// The transaction body is base64 to facilitate transmission, and txHash is used on the mobile phone to verify whether the transmitted data is correct 
	size_t encodeLen = txData.size() * 2 + 1;
	unsigned char encode[encodeLen] = {0};
	memset(encode, 0, encodeLen);
	long codeLen = base64_encode((unsigned char *)txData.data(), txData.size(), encode);

	createTxMsgAck.set_txdata( (char *)encode, codeLen );
	std::string encodeStr((char *)encode, codeLen);
	std::string txEncodeHash = getsha256hash(encodeStr);
	createTxMsgAck.set_txencodehash(txEncodeHash);

	ReturnAckCode<CreateTxMsgAck>(msgdata, errInfo, createTxMsgAck, 0);
}

void HandleGetMacReq(const std::shared_ptr<GetMacReq>& getMacReq, const MsgData& from)
{	
	std::cout << "GetDeviceInfo ok" << std::endl;
	std::vector<string> outmac;
	get_mac_info(outmac);
	
	std::string macstr;
	for(auto &i:outmac)
	{
		macstr += i;
	}
	string md5 = getMD5hash(macstr.c_str());
	GetMacAck getMacAck;
	getMacAck.set_mac(md5);
	cout <<"mac:="<<md5<<endl;

	net_send_message(from, getMacAck);
}

int get_mac_info(vector<string> &vec)
{	 
	int fd;
    int interfaceNum = 0;
    struct ifreq buf[16] = {0};
    struct ifconf ifc;
    char mac[16] = {0};

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket");
        close(fd);
        return -1;
    }
    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = (caddr_t)buf;
    if (!ioctl(fd, SIOCGIFCONF, (char *)&ifc))
    {
        interfaceNum = ifc.ifc_len / sizeof(struct ifreq);
        while (interfaceNum-- > 0)
        {
            //printf("ndevice name: %s\n", buf[interfaceNum].ifr_name);
            if(string(buf[interfaceNum].ifr_name) == "lo")
            {
                continue;
            }
            if (!ioctl(fd, SIOCGIFHWADDR, (char *)(&buf[interfaceNum])))
            {
                memset(mac, 0, sizeof(mac));
                snprintf(mac, sizeof(mac), "%02x%02x%02x%02x%02x%02x",
                    (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[0],
                    (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[1],
                    (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[2],

                    (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[3],
                    (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[4],
                    (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[5]);
                // allmac[i++] = mac;
                std::string s = mac;
                vec.push_back(s);
            }
            else
            {
                printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
                close(fd);
                return -1;
            }
        }
    }
    else
    {
        printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

int SearchPledge(const std::string &address, uint64_t &pledgeamount, std::string pledgeType)
{
	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();

	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, false);
	};
	
	std::vector<string> utxos;
	int db_status = pRocksDb->GetPledgeAddressUtxo(txn, address, utxos);
	if (db_status != 0) 
	{
		// std::cout << __LINE__ << std::endl;
		return -1;
	}
	uint64_t total = 0;
	for (auto &item : utxos) 
    {
	 	std::string strTxRaw;
		if (pRocksDb->GetTransactionByHash(txn, item, strTxRaw) != 0)
		{
			continue;
		}
		CTransaction utxoTx;
		utxoTx.ParseFromString(strTxRaw);

		nlohmann::json extra = nlohmann::json::parse(utxoTx.extra());
		nlohmann::json txInfo = extra["TransactionInfo"].get<nlohmann::json>();
		std::string txPledgeType = txInfo["PledgeType"].get<std::string>();
		if (txPledgeType != pledgeType)
		{
			continue;
		}

		for (int i = 0; i < utxoTx.vout_size(); i++)
		{
			CTxout txout = utxoTx.vout(i);
			if (txout.scriptpubkey() == VIRTUAL_ACCOUNT_PLEDGE)
			{
				total += txout.value();
			}
		}
    }
	pledgeamount = total;
	return 0;
}

int GetAbnormalAwardAddrList(std::vector<std::string> & addrList)
{
	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if (txn == NULL)
	{
		error("(GetAbnormalAwardAddrList) TransactionInit failed !");
		return -1;
	}

	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, true);
	};

	const uint64_t heightRange = 1000;  // Check the abnormal height range 
	std::map<std::string, uint64_t> addrAwards;  // Deposit account and top 500 total rewards 
	std::map<std::string, uint64_t> addrSignNum;  // The total number of signatures stored in the account and the top 500 heights 

	unsigned int top = 0;
	if ( 0 != pRocksDb->GetBlockTop(txn, top) )
	{
		std::cout << "(GetAbnormalAwardAddrList) GetBlockTop failed! " << std::endl;
		return -2;
	}

	uint64_t minHeight = top > heightRange ? (int)top - heightRange : 0;  // Minimum height of abnormality 

	for ( ; top != minHeight; --top)
	{
		std::vector<std::string> blockHashs;
		if ( 0 != pRocksDb->GetBlockHashsByBlockHeight(txn, top, blockHashs) )
		{
			std::cout << "(GetAbnormalAwardAddrList) GetBlockHashsByBlockHeight failed! " << std::endl;
			return -3;
		}

		for (auto & hash : blockHashs)
		{
			std::string blockStr;
			if (0 != pRocksDb->GetBlockByBlockHash(txn, hash, blockStr))
			{
				std::cout << "(GetAbnormalAwardAddrList) GetBlockByBlockHash failed! " << std::endl;
				return -4;
			}

			CBlock block;
			block.ParseFromString(blockStr);

			for (auto & tx : block.txs())
			{
				if (CheckTransactionType(tx) == kTransactionType_Award)
				{
					for (auto & txout : tx.vout())
					{
						if (txout.value() == 0)
						{
							continue;
						}

						// Total reward 
						auto iter = addrAwards.find(txout.scriptpubkey());
						if (addrAwards.end() != iter)
						{
							addrAwards[txout.scriptpubkey()] = iter->second + txout.value();
						}
						else
						{
							addrAwards[txout.scriptpubkey()] = txout.value();
						}

						// Total number of signatures 
						auto signNumIter = addrSignNum.find(txout.scriptpubkey());
						if (addrSignNum.end() != signNumIter)
						{
							addrSignNum[txout.scriptpubkey()] = (++signNumIter->second);
						}
						else
						{
							addrSignNum[txout.scriptpubkey()] = 1;
						}
						
					}
				}
			}
		}
	}

	if (addrAwards.size() == 0 || addrSignNum.size() == 0)
	{
		return 0;
	}

	std::vector<uint64_t> awards;  // Store all reward values 
	std::vector<uint64_t> vecSignNum;  // Store all reward values 
	for (auto & addrAward : addrAwards)
	{
		awards.push_back(addrAward.second);
	}

	for(auto & signNum : addrSignNum)
	{
		vecSignNum.push_back(signNum.second);
	}

	std::sort(awards.begin(), awards.end());
	std::sort(vecSignNum.begin(), vecSignNum.end());

	uint64_t awardQuarterNum = awards.size() * 0.25;
	uint64_t awardThreeQuarterNum = awards.size() * 0.75;
	
	uint64_t signNumQuarterNum = vecSignNum.size() * 0.25;
	uint64_t signNumThreeQuarterNum = vecSignNum.size() * 0.75;

	if (awardQuarterNum == awardThreeQuarterNum || signNumQuarterNum == signNumThreeQuarterNum)
	{
		return 0;
	}

	uint64_t awardQuarterValue = awards[awardQuarterNum];
	uint64_t awardThreeQuarterValue = awards[awardThreeQuarterNum];

	uint64_t signNumQuarterValue = vecSignNum[signNumQuarterNum];
	uint64_t signNumThreeQuarterValue = vecSignNum[signNumThreeQuarterNum];

	uint64_t awardDiffValue = awardThreeQuarterValue - awardQuarterValue;
	uint64_t awardUpperLimitValue = awardThreeQuarterValue + (awardDiffValue * 1.5);

	uint64_t signNumDiffValue = signNumThreeQuarterValue - signNumQuarterValue;
	uint64_t signNumUpperLimitValue = signNumThreeQuarterValue + (signNumDiffValue * 1.5);

	std::vector<std::string> awardList;
	std::vector<std::string> signNumList;
	for (auto & addrAward : addrAwards)
	{
		if (addrAward.second > awardUpperLimitValue)
		{
			awardList.push_back(addrAward.first);
		}
	}

	for (auto & addrSign : addrSignNum)
	{
		if (addrSign.second > signNumUpperLimitValue)
		{
			signNumList.push_back(addrSign.first);
		}
	}

	set_union(awardList.begin(), awardList.end(), signNumList.begin(), signNumList.end(), std::back_inserter(addrList));

	return 0;
}

int FindSignNode(const CTransaction & tx, const int nodeNumber, const std::vector<std::string> & signedNodes, std::vector<std::string> & nextNodes)
{
	// Parameter judgment 
	if(nodeNumber <= 0)
	{
		return -1;
	}

	nlohmann::json txExtra = nlohmann::json::parse(tx.extra());
	uint64_t minerFee = txExtra["SignFee"].get<int>();

	bool isPledge = false;
	std::string txType = txExtra["TransactionType"].get<std::string>();
	if (txType == TXTYPE_PLEDGE)
	{
		isPledge = true;
	}

	bool isInitAccount = false;
	std::vector<std::string> vTxowners = TxHelper::GetTxOwner(tx);
	if (vTxowners.size() == 1 && vTxowners.end() != find(vTxowners.begin(), vTxowners.end(), g_InitAccount) )
	{
		isInitAccount = true;
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

	const Node & selfNode = Singleton<PeerNode>::get_instance()->get_self_node();
	std::vector<Node> nodelist;
	if (selfNode.is_public_node)
	{
		nodelist = Singleton<PeerNode>::get_instance()->get_nodelist();
	}
	else
	{
		nodelist = Singleton<NodeCache>::get_instance()->get_nodelist();
	}
	
	// When the height of the current data block is 0, GetPledgeAddress will return an error, so no return judgment is made 
	std::vector<string> addresses; // Pledged address 
	std::vector<string> pledgeAddrs; // Pledged address to be selected 
	// pRocksDb->GetPledgeAddress(txn, addresses);
	pRocksDb->GetPledgeAddress(txn, pledgeAddrs);

	// Remove duplicate nodes in base58 
	std::map<std::string, std::string> tmpBase58Ids; // Temporary De-duplication 
	std::vector<std::string> vRepeatedIds; // Duplicate address 
	
	for (auto & node : nodelist)
	{
		// Query the pledged address in the list 
		// std::string addr = node.base58address;
		// auto iter = find(addresses.begin(), addresses.end(), addr);
		// if (iter != addresses.end())
		// {
		// 	pledgeAddrs.push_back(addr);
		// }

		// Query duplicate base58 addresses 
		auto ret = tmpBase58Ids.insert(make_pair(node.base58address, node.id));
		if (!ret.second)
		{
			vRepeatedIds.push_back(node.id);
		}
	}
	
	for (auto & id : vRepeatedIds)
	{
		auto iter = std::find_if(nodelist.begin(), nodelist.end(), [id](auto & node){
			return id == node.id;
		});
		if (iter != nodelist.end())
		{
			nodelist.erase(iter);
		}
	}

	std::string ownerID = net_get_self_node_id(); // Own node 

	// Take out both sides of the transaction 
	std::vector<std::string> txAddrs;
	for(int i = 0; i < tx.vout_size(); ++i)
	{
		CTxout txout = tx.vout(i);
		txAddrs.push_back(txout.scriptpubkey());
	}

	// Get the node of the abnormal account 
	std::vector<std::string> abnormalAddrList;
	if ( 0 != GetAbnormalAwardAddrList(abnormalAddrList) )
	{
		std::cout << "(FindSignNode) GetAbnormalAwardAddrList failed ! " << std::endl;
		return -3;
	}
	
	for (auto iter = nodelist.begin(); iter != nodelist.end(); )
	{
		// Delete own node 
		if (iter->id == ownerID)
		{
			iter = nodelist.erase(iter);
			continue;
		}

		// Delete both nodes of transaction 
		if (txAddrs.end() != find(txAddrs.begin(), txAddrs.end(), iter->base58address))
		{
			iter = nodelist.erase(iter);
			continue;
		}

		// Remove accounts with abnormal reward values 
		
		if (abnormalAddrList.end() != find(abnormalAddrList.begin(), abnormalAddrList.end(), iter->base58address))
		{
			iter = nodelist.erase(iter);
			continue;
		}

		if (iter->chain_height + REASONABLE_HEIGHT_RANGE < selfNode.chain_height || 
			selfNode.chain_height + REASONABLE_HEIGHT_RANGE < iter->chain_height)
		{
			iter = nodelist.erase(iter);
			continue;
		}

		++iter;
	}
	
	std::vector<std::pair<std::string, uint64_t>> vecIdsInfos;
	for (auto & node : nodelist)
	{
		vecIdsInfos.push_back(std::make_pair(node.id, node.fee));
	}

	// Randomly pick nodes 
	random_device rd;
	while (nextNodes.size() != (uint64_t)nodeNumber && vecIdsInfos.size() != 0)
	{
		default_random_engine rng {rd()};
		uniform_int_distribution<int> dist {0, (int)vecIdsInfos.size() - 1};
		int randNum = dist(rng);

		if (vecIdsInfos[randNum].second <= minerFee)
		{
			std::string id = vecIdsInfos[randNum].first;
			
			auto iter = std::find_if(nodelist.begin(), nodelist.end(), [id](auto & node){
				return id == node.id;
			});

			if (nodelist.end() != iter)
			{
				if ( (isPledge || isInitAccount) && pledgeAddrs.size() < g_minPledgeNodeNum )
				{
					
					nextNodes.push_back(iter->id);
				}
				else
				{
					uint64_t pledgeAmount = 0;
					if ( 0 != SearchPledge(iter->base58address, pledgeAmount) )
					{
						vecIdsInfos.erase(vecIdsInfos.begin() + randNum);
						continue;
					}

					if (pledgeAmount >= g_TxNeedPledgeAmt)
					{
						nextNodes.push_back(iter->id);
					}
				}
			}
		}
		vecIdsInfos.erase(vecIdsInfos.begin() + randNum);
	}

	// Filter signed 
	for(auto signedId : signedNodes)
	{
		auto iter = std::find(nextNodes.begin(), nextNodes.end(), signedId);
		if (iter != nextNodes.end())
		{
			nextNodes.erase(iter);
		}
	}

	// Screen random nodes 
	std::vector<std::string> sendid;
	if (nextNodes.size() <= (uint32_t)nodeNumber)
	{
		for (auto & nodeid  : nextNodes)
		{
			sendid.push_back(nodeid);
		}
	}
	else
	{
		std::set<int> rSet;
		srand(time(NULL));
		int num = std::min((int)nextNodes.size(), nodeNumber);
		for(int i = 0; i < num; i++)
		{
			int j = rand() % nextNodes.size();
			rSet.insert(j);		
		}

		for (auto i : rSet)
		{
			sendid.push_back(nextNodes[i]);
		}
	}

	nextNodes = sendid;

	return 0;
}

void GetOnLineTime()
{
	static time_t startTime = time(NULL);
	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if(!txn) 
	{
		std::cout << " TransactionInit failed !" << std::endl;
		std::cout << __LINE__ << std::endl;
		return ;
	}

	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, true);
	};

	{
		// patch
		double minertime = 0.0;
		if (0 != pRocksDb->GetDeviceOnLineTime(minertime))
		{
			if ( 0 != pRocksDb->SetDeviceOnlineTime(0.00001157) )
			{
				error("(GetOnLineTime) SetDeviceOnlineTime failed!");
				return;
			}
		}

		if (minertime > 365.0)
		{
			if ( 0 != pRocksDb->SetDeviceOnlineTime(0.00001157) )
			{
				error("(GetOnLineTime) SetDeviceOnlineTime failed!");
				return;
			}
		}
	}

	// Record online time since transaction 
	std::vector<std::string> vTxHashs;
	std::string addr = g_AccountInfo.DefaultKeyBs58Addr;
	int db_get_status = pRocksDb->GetAllTransactionByAddreess(txn, addr, vTxHashs); 	
	if (db_get_status != 0) 
	{
		std::cout << __LINE__ << std::endl;
	}

	std::vector<Node> vnode = net_get_public_node();
	if(vTxHashs.size() >= 1 && vnode.size() >= 1 )
	{
		double onLineTime = 0.0;
		if ( 0 != pRocksDb->GetDeviceOnLineTime(onLineTime) )
		{
			if ( 0 != pRocksDb->SetDeviceOnlineTime(0.00001157) )
			{
				error("(GetOnLineTime) SetDeviceOnlineTime failed!");
				return;
			}
			return ;
		}

		time_t endTime = time(NULL);
		time_t dur = difftime(endTime, startTime);
		double durDay = (double)dur / (1*60*60*24);
		
		double minertime = 0.0;
		if (0 != pRocksDb->GetDeviceOnLineTime(minertime))
		{
			error("(GetOnLineTime) GetDeviceOnLineTime failed!");
			return ;
		}

		double accumatetime = durDay + minertime; 
		if ( 0 != pRocksDb->SetDeviceOnlineTime(accumatetime) )
		{
			error("(GetOnLineTime) SetDeviceOnlineTime failed!");
			return ;
		}
		
		startTime = endTime;
	}
	else
	{
		startTime = time(NULL);
	}
	
	if ( 0 != pRocksDb->TransactionCommit(txn) )
	{
		error("(GetOnLineTime) TransactionCommit failed!");
		return ;
	}
}

int PrintOnLineTime()
{
	double  onlinetime;
	auto 	pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	int 	db_status = pRocksDb->GetDeviceOnLineTime(onlinetime);
	int  day = 0,hour = 0,minute = 0,second = 0;

    double totalsecond = onlinetime *86400;

	cout<<"totalsecond="<<totalsecond<<endl;

	day = totalsecond/86400;
	cout<<"day="<< day <<endl;

	hour = (totalsecond - (day *86400))/3600;
	cout<<"hour="<< hour <<endl;

	minute = (totalsecond - (day *86400) - (hour *3600))/60;
	cout<<"minute="<< minute <<endl;

	second = (totalsecond - (day *86400) - (hour *3600) - (minute *60));
	cout<<"second="<< second <<endl;

	cout<<"day:"<<day<<"hour:"<<hour<<"minute:"<<minute <<"second:"<< second<<endl;

	if(db_status != 0)
	{
		std::cout << __LINE__ << std::endl;
		cout<<"Get the device time  failed"<<endl;
		return -1;
	}    
	return 0;        
}

int TestSetOnLineTime()
{
	cout<<"Check the online duration first and then set the device online duration "<<endl;
	PrintOnLineTime();
	std::cout <<"Please enter the online duration of the device "<<std::endl;
	
	static double day  = 0.0,hour = 0.0,minute = 0.0,second = 0.0,totalsecond = 0.0,accumlateday=0.0;
	double inday  = 0.0,inhour = 0.0,inminute = 0.0,insecond = 0.0,intotalsecond = 0.0,inaccumlateday=0.0;
	cout<<"Please enter the number of days the device has been online "<<endl;
	std::cin >> inday;
	cout<<"Please enter the number of hours the device is online "<<endl;
	std::cin >> inhour;
	cout<<"Please enter the number of minutes the device is online "<<endl;
	std::cin >> inminute;
	cout<<"Please enter the number of seconds that the device is online "<<endl;
	std::cin >> insecond;
	
	intotalsecond = inday *86400 + inhour *3600 + inminute*60 +insecond;
	inaccumlateday = intotalsecond/86400;
	
	cout<<"input day="<< inday<<endl;
	cout<<"input hour="<< inhour <<endl;
	cout<<"input minute="<< inminute <<endl;
	cout<<"input second="<< insecond <<endl;
	cout<< "input accumlateday= "<< inaccumlateday <<endl;
	cout<<"input totalsecond = "<<intotalsecond <<endl;
	day  += inday; 
	hour += inhour;
	minute += inminute;
	second += insecond;
	totalsecond = day *86400 + hour *3600 + minute*60 +second;
	accumlateday = totalsecond/86400;
	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	// std::string time;
	// std::cin >> time;
	// std::stringstream ssAmount(time);
	// double day;
	// ssAmount >> day;
  	int db_status = pRocksDb->SetDeviceOnlineTime(accumlateday);
	if(db_status == 0)
	{
		cout<<"set the data success"<<endl;
		return 0;
	}
	return -1;
}


/** Verify the password of the miner before connecting to the miner on the mobile phone to initiate a transaction (test whether the connection is successful)  */
void HandleVerifyDevicePassword( const std::shared_ptr<VerifyDevicePasswordReq>& msg, const MsgData& msgdata )
{	
	VerifyDevicePasswordAck verifyDevicePasswordAck;
	verifyDevicePasswordAck.set_version(getVersion());

	// Determine whether the version is compatible 
	if( 0 != Util::IsVersionCompatible( msg->version() ) )
	{
		verifyDevicePasswordAck.set_code(-1);
		verifyDevicePasswordAck.set_message("version error!");
		net_send_message<VerifyDevicePasswordAck>(msgdata, verifyDevicePasswordAck);
		error("HandleBuileBlockBroadcastMsg IsVersionCompatible");
		return ;
	}

	string  minerpasswd = Singleton<DevicePwd>::get_instance()->GetDevPassword();
	std::string passwordStr = generateDeviceHashPassword(msg->password());
	std::string password = msg->password();
    std::string hashOriPass = generateDeviceHashPassword(password);
    std::string targetPassword = Singleton<DevicePwd>::get_instance()->GetDevPassword();
    auto pCPwdAttackChecker = MagicSingleton<CPwdAttackChecker>::GetInstance(); 
   
    uint32_t minutescount ;
    bool retval = pCPwdAttackChecker->IsOk(minutescount);
    if(retval == false)
    {
        std::string minutescountStr = std::to_string(minutescount);
        verifyDevicePasswordAck.set_code(-31);
        verifyDevicePasswordAck.set_message(minutescountStr);
		
        cout<<"There are 3 consecutive errors,"<<minutescount<<" seconds before you can enter "<<endl; 
		net_send_message<VerifyDevicePasswordAck>(msgdata, verifyDevicePasswordAck);
        return;
    }

    if(hashOriPass.compare(targetPassword))
    {
        cout<<"Enter the wrong password to start recording times "<<endl;
       if(pCPwdAttackChecker->Wrong())
       {
            cout<<"Incorrect password "<<endl;
            verifyDevicePasswordAck.set_code(-2);
            verifyDevicePasswordAck.set_message("Incorrect password ");
			net_send_message<VerifyDevicePasswordAck>(msgdata, verifyDevicePasswordAck);
            return;
       } 
	   else
	   {
			verifyDevicePasswordAck.set_code(-30);
			verifyDevicePasswordAck.set_message("Incorrect password for the third time ");
			net_send_message<VerifyDevicePasswordAck>(msgdata, verifyDevicePasswordAck);
			return;
	   }  
    }
    else 
    {
        cout<<"HandleVerifyDevicePasswordPassword input is correct and reset to 0 "<<endl;
        pCPwdAttackChecker->Right();
		verifyDevicePasswordAck.set_code(0);
        verifyDevicePasswordAck.set_message("Password input is correct ");
		net_send_message<VerifyDevicePasswordAck>(msgdata, verifyDevicePasswordAck);
    }

	if (hashOriPass != targetPassword) 
    {
        verifyDevicePasswordAck.set_code(-2);
        verifyDevicePasswordAck.set_message("password error!");
        net_send_message<VerifyDevicePasswordAck>(msgdata, verifyDevicePasswordAck);
        error("password error!");
        return;
    }
	return ;
}

/** Mobile phone connection to the miner initiated transaction  */
void HandleCreateDeviceTxMsgReq( const std::shared_ptr<CreateDeviceTxMsgReq>& msg, const MsgData& msgdata )
{
	cout<<"HandleCreateDeviceTxMsgReq"<<endl;
	// Mobile receipt message 
	TxMsgAck txMsgAck;
	txMsgAck.set_version(getVersion());

	// Determine whether the version is compatible 
	if( 0 != Util::IsVersionCompatible( msg->version() ) )
	{
		txMsgAck.set_code(-1);
		txMsgAck.set_message("version error!");
		net_send_message<TxMsgAck>(msgdata, txMsgAck, net_com::Priority::kPriority_Middle_1);
		error("HandleCreateDeviceTxMsgReq IsVersionCompatible");
		return ;
	}

	// Determine whether the miner password is correct 
    std::string password = msg->password();
    std::string hashOriPass = generateDeviceHashPassword(password);
    std::string targetPassword = Singleton<DevicePwd>::get_instance()->GetDevPassword();
	auto pCPwdAttackChecker = MagicSingleton<CPwdAttackChecker>::GetInstance(); 
  
    uint32_t minutescount ;
    bool retval = pCPwdAttackChecker->IsOk(minutescount);
    if(retval == false)
    {
        std::string minutescountStr = std::to_string(minutescount);
        txMsgAck.set_code(-31);
        txMsgAck.set_message(minutescountStr);
		net_send_message<TxMsgAck>(msgdata, txMsgAck, net_com::Priority::kPriority_Middle_1);
        cout<<"There are 3 consecutive errors ，"<<minutescount<<"You can enter after seconds "<<endl;
        return ;
    }

    if(hashOriPass.compare(targetPassword))
    {
        cout<<"Enter the wrong password to start recording times "<<endl;
       if(pCPwdAttackChecker->Wrong())
       {
            cout<<"Incorrect password "<<endl;
            txMsgAck.set_code(-5);
            txMsgAck.set_message("Incorrect password ");
			net_send_message<TxMsgAck>(msgdata, txMsgAck, net_com::Priority::kPriority_Middle_1);
            return ;
       }
	   else
	   {
			txMsgAck.set_code(-30);
			txMsgAck.set_message("The third password input error ");
			net_send_message<TxMsgAck>(msgdata, txMsgAck, net_com::Priority::kPriority_Middle_1);
			return ;
	   }
    }
    else 
    {
        cout<<"HandleCreateDeviceTxMsgReq Password input is correct and reset to 0 "<<endl;
        pCPwdAttackChecker->Right();
		// txMsgAck.set_code(0);
        // txMsgAck.set_message("Password input is correct ");
		// net_send_message<TxMsgAck>(msgdata, txMsgAck);
    }

    if (hashOriPass != targetPassword) 
    {
        txMsgAck.set_code(-5);
        txMsgAck.set_message("password error!");
        net_send_message<TxMsgAck>(msgdata, txMsgAck, net_com::Priority::kPriority_Middle_1);
        error("password error!");
        return;
    }

	// Determine whether each field is legal 
	if(msg->from().size() <= 0 || msg->to().size() <= 0 || msg->amt().size() <= 0 ||
		msg->minerfees().size() <= 0 || msg->needverifyprehashcount().size() <= 0)
	{
		txMsgAck.set_code(-2);
		txMsgAck.set_message("parameter error!");
		net_send_message<TxMsgAck>(msgdata, txMsgAck, net_com::Priority::kPriority_Middle_1);

		error("HandleCreateDeviceTxMsgReq parameter error!");
		return ;
	}

	if( std::stod( msg->minerfees() ) <= 0 || 
		std::stoi( msg->needverifyprehashcount() ) < g_MinNeedVerifyPreHashCount ||
		std::stod( msg->amt() ) <= 0)
	{
		txMsgAck.set_code(-3);
		txMsgAck.set_message("minerfees or needverifyprehashcount error!");
		net_send_message<TxMsgAck>(msgdata, txMsgAck, net_com::Priority::kPriority_Middle_1);

		error("HandleCreateDeviceTxMsgReq parameter error!");
		return ;
	}

	vector<string> Addr;
	Addr.push_back(msg->from());
	if (MagicSingleton<TxVinCache>::GetInstance()->IsConflict(Addr))
	{
		cout<<"IsConflict HandleCreateDeviceTxMsgReq"<<endl;
		txMsgAck.set_code(-20);
		txMsgAck.set_message("The addr has being pengding!");
		net_send_message<TxMsgAck>(msgdata, txMsgAck, net_com::Priority::kPriority_Middle_1);
		cout<<"HandleCreateDeviceTxMsgReq CreateTx failed!!"<<endl;
		error("HandleCreateDeviceTxMsgReq CreateTx failed!!");
		return ;
	}

	int ret = CreateTx(msg->from().c_str(), msg->to().c_str(), msg->amt().c_str(), NULL, std::stoi(msg->needverifyprehashcount()), msg->minerfees().c_str());
	if(ret < 0)
	{
		txMsgAck.set_code(-4);
		txMsgAck.set_message("CreateTx failed!");
		net_send_message<TxMsgAck>(msgdata, txMsgAck, net_com::Priority::kPriority_Middle_1);

		error("HandleCreateDeviceTxMsgReq CreateTx failed!!");
		return ;
	}

	txMsgAck.set_code(0);
	txMsgAck.set_message("CreateTx successful! Waiting for broadcast!");
	net_send_message<TxMsgAck>(msgdata, txMsgAck, net_com::Priority::kPriority_Middle_1);

	debug("HandleCreateDeviceTxMsgReq CreateTx successful! Waiting for broadcast! ");
	return ;
}


std::map<int32_t, std::string> GetCreateDeviceMultiTxMsgReqCode()
{
	std::map<int32_t, std::string> errInfo = {make_pair(0, "success "), 
												make_pair(-1, "Incompatible version "),
												make_pair(-2, "Three times the password was entered incorrectly "),
												make_pair(-3, "Incorrect password "),
												make_pair(-4, "The third password input error "),
												make_pair(-5, "Incorrect password "),
												make_pair(-6, "Initiation address parameter error "),
												make_pair(-7, "Receive address parameter error "),

												make_pair(-8, "Parameter error when creating transaction "),
												make_pair(-9, "Wrong transaction address when creating transaction "),
												make_pair(-10, "When the transaction was created, the previous transaction was suspended "),
												make_pair(-11, "Error opening database when creating transaction "),
												make_pair(-12, "Failed to get packaging fee when creating transaction "),
												make_pair(-13, "Failed to obtain transaction information when creating transaction "),
												make_pair(-14, "Insufficient balance when creating transaction "),
												make_pair(-15, "Other errors when creating transaction "),

												make_pair(-16, "Open database error "),
												make_pair(-17, "Get packing fee error "),
												make_pair(-18, "Get the main chain error "),
												};

	return errInfo;												
}

void HandleCreateDeviceMultiTxMsgReq(const std::shared_ptr<CreateDeviceMultiTxMsgReq>& msg, const MsgData& msgdata)
{
    TxMsgAck txMsgAck;
	auto errInfo = GetCreateDeviceMultiTxMsgReqCode();
    
    if( 0 != Util::IsVersionCompatible( msg->version() ) )
	{		
		ReturnAckCode<TxMsgAck>(msgdata, errInfo, txMsgAck, -1);
		return ;
	}

    // Determine whether the miner password is correct 
    std::string password = msg->password();
    std::string hashOriPass = generateDeviceHashPassword(password);
    std::string targetPassword = Singleton<DevicePwd>::get_instance()->GetDevPassword();
    auto pCPwdAttackChecker = MagicSingleton<CPwdAttackChecker>::GetInstance(); 
   
    uint32_t minutescount ;
    bool retval = pCPwdAttackChecker->IsOk(minutescount);
    if(retval == false)
    {
		cout<<"There are 3 consecutive errors ，"<<minutescount<<"You can enter after seconds "<<endl;
        std::string minutescountStr = std::to_string(minutescount);
        ReturnAckCode<TxMsgAck>(msgdata, errInfo, txMsgAck, -2, minutescountStr);
        return;
    }

    if(hashOriPass.compare(targetPassword))
    {
        cout<<"Enter the wrong password to start recording times "<<endl;
       if(pCPwdAttackChecker->Wrong())
       {
            cout<<"Incorrect password "<<endl;
            ReturnAckCode<TxMsgAck>(msgdata, errInfo, txMsgAck, -3);
            return;
       } 
       else
       {
			// The third password input error 
    		ReturnAckCode<TxMsgAck>(msgdata, errInfo, txMsgAck, -4);
            return;
       }
    }
    else 
    {
        cout<<"Password input is correct and reset to 0 "<<endl;
        pCPwdAttackChecker->Right();
    }
   
    if (hashOriPass != targetPassword) 
    {
		// Incorrect password 
        ReturnAckCode<TxMsgAck>(msgdata, errInfo, txMsgAck, -5);
        return;
    }

    if (0 != CheckAddrs<CreateDeviceMultiTxMsgReq>(msg))
    {
		// Initiator parameter error 
		ReturnAckCode<TxMsgAck>(msgdata, errInfo, txMsgAck, -6);
		return ;
    }

    std::vector<std::string> fromAddr;
    std::map<std::string, int64_t> toAddr;

    for (int i = 0; i < msg->from_size(); ++i)
    {
        std::string fromAddrStr = msg->from(i);
        fromAddr.push_back(fromAddrStr);
    }

    for (int i = 0; i < msg->to_size(); ++i)
    {
        ToAddr toAddrInfo = msg->to(i);
        int64_t amount = std::stod(toAddrInfo.amt()) * DECIMAL_NUM;
        toAddr.insert( make_pair(toAddrInfo.toaddr(), amount ) );
    }

    if(toAddr.size() != (size_t)msg->to_size())
    {
        // Receiver parameter error 
		ReturnAckCode<TxMsgAck>(msgdata, errInfo, txMsgAck, -7);
        return;
    }

    uint64_t gasFee = std::stod( msg->gasfees() ) * DECIMAL_NUM;
    uint32_t needVerifyPreHashCount = stoi( msg->needverifyprehashcount() );
    
    CTransaction outTx;
    int ret = TxHelper::CreateTxMessage(fromAddr,toAddr, needVerifyPreHashCount, gasFee, outTx);
	if(ret != 0)
	{
		ret -= 100;
		ReturnAckCode<TxMsgAck>(msgdata, errInfo, txMsgAck, ret);
		return ;
	}

    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
    Transaction* txn = pRocksDb->TransactionInit();
    if (txn == NULL)
    {
        ReturnAckCode<TxMsgAck>(msgdata, errInfo, txMsgAck, -8);
        return;
    }

    ON_SCOPE_EXIT
    {
		pRocksDb->TransactionDelete(txn, false);
	};

    uint64_t packageFee = 0;
    if ( 0 != pRocksDb->GetDevicePackageFee(packageFee) )
    {
        ReturnAckCode<TxMsgAck>(msgdata, errInfo, txMsgAck, -9);
        return ;
    }

    nlohmann::json txExtra = nlohmann::json::parse(outTx.extra());
    txExtra["TransactionType"] = TXTYPE_TX;	
    txExtra["NeedVerifyPreHashCount"] = needVerifyPreHashCount;
	txExtra["SignFee"] = gasFee;
    txExtra["PackageFee"] = packageFee;   // This node is required to send a transaction on behalf of the package fee 

    outTx.set_extra(txExtra.dump());

    std::vector<std::string> addrs;
	for (int i = 0; i < outTx.vin_size(); i++)
	{
		CTxin * txin = outTx.mutable_vin(i);;
		std::string pub = txin->mutable_scriptsig()->pub();
		txin->clear_scriptsig();
		addrs.push_back(GetBase58Addr(pub));
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

    txMsg.set_tx(serTx);
	txMsg.set_txencodehash( encodeStrHash );

    std::string blockHash;
    if ( 0 != pRocksDb->GetBestChainHash(txn, blockHash) )
    {
        ReturnAckCode<TxMsgAck>(msgdata, errInfo, txMsgAck, -18);
        return;
    }
    txMsg.set_prevblkhash(blockHash);
    txMsg.set_trycountdown(CalcTxTryCountDown(needVerifyPreHashCount));
    
	unsigned int top = 0;
	pRocksDb->GetBlockTop(txn, top);	
	txMsg.set_top(top);

    auto pTxMsg = make_shared<TxMsg>(txMsg);
	std::string txHash;
	ret = DoHandleTx(pTxMsg, txHash);
	txMsgAck.set_txhash(txHash);

	if (ret != 0)
	{
		ret -= 200;
	}

	std::cout << "ret: " << ret << std::endl;

	ReturnAckCode<TxMsgAck>(msgdata, errInfo, txMsgAck, ret);
}

std::map<int32_t, std::string> GetCreateMultiTxReqCode()
{
	std::map<int32_t, std::string> errInfo = {make_pair(0, "success "), 
												make_pair(-1, "Incompatible version "), 
												make_pair(-2, "Invalid parameter "), 
												make_pair(-3, "transaction Both parties address wrong "), 
												make_pair(-4, "The address of both parties to obtain the transaction is wrong "), 

												make_pair(-1001, "Parameter error when creating transaction "),
												make_pair(-1002, "Wrong transaction address when creating transaction "),
												make_pair(-1003, "When the transaction was created, the previous transaction was suspended "),
												make_pair(-1004, "Error opening database when creating transaction "),
												make_pair(-1005, "Failed to get packaging fee when creating transaction "),
												make_pair(-1006, "Failed to obtain transaction information when creating transaction "),
												make_pair(-1007, "Insufficient balance when creating transaction "),

												make_pair(-5, "Open database error "),
												make_pair(-6, "Get packing fee error "),
												
												};

	return errInfo;												
}
void HandleCreateMultiTxReq( const std::shared_ptr<CreateMultiTxMsgReq>& msg, const MsgData& msgdata )
{
    // Message body of mobile phone receipt 
    CreateMultiTxMsgAck createMultiTxMsgAck;
	auto errInfo = GetCreateMultiTxReqCode();

    // Determine whether the version is compatible 
    if( 0 != Util::IsVersionCompatible( msg->version() ) )
	{
		ReturnAckCode<CreateMultiTxMsgAck>(msgdata, errInfo, createMultiTxMsgAck, -1);
		return ;
	}

    if (msg->from_size() > 1 && msg->to_size() > 1)
    {
        // Invalid parameter 
		ReturnAckCode<CreateMultiTxMsgAck>(msgdata, errInfo, createMultiTxMsgAck, -2);
		return ;
    }

	if (0 != CheckAddrs<CreateMultiTxMsgReq>(msg))
	{
		ReturnAckCode<CreateMultiTxMsgAck>(msgdata, errInfo, createMultiTxMsgAck, -3);
		return ;
	}

    CTransaction outTx;
    std::vector<std::string> fromAddr;
    std::map<std::string, int64_t> toAddr;

    int ret = GetAddrsFromMsg(msg, fromAddr, toAddr);
    if (0 != ret)
    {
		// transaction The address is wrong 
		ReturnAckCode<CreateMultiTxMsgAck>(msgdata, errInfo, createMultiTxMsgAck, -4);
        return ;
    }

    uint64_t minerFees = stod( msg->minerfees() ) * DECIMAL_NUM;
    uint32_t needVerifyPreHashCount = stoi( msg->needverifyprehashcount() );
    
    ret = TxHelper::CreateTxMessage(fromAddr, toAddr, needVerifyPreHashCount, minerFees, outTx);
    if(ret != 0)
	{
		ret -= 100;
        ReturnAckCode<CreateMultiTxMsgAck>(msgdata, errInfo, createMultiTxMsgAck, ret);
		return;
	}

    for(int i = 0;i <outTx.vin_size();i++)
    {
        CTxin *txin = outTx.mutable_vin(i);
        txin->clear_scriptsig();
    }

    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
    Transaction* txn = pRocksDb->TransactionInit();
    if (txn == NULL)
    {
        ReturnAckCode<CreateMultiTxMsgAck>(msgdata, errInfo, createMultiTxMsgAck, -5);
        return;
    }

    ON_SCOPE_EXIT
    {
		pRocksDb->TransactionDelete(txn, false);
	};

    uint64_t packageFee = 0;
    if ( 0 != pRocksDb->GetDevicePackageFee(packageFee) )
    {
        ReturnAckCode<CreateMultiTxMsgAck>(msgdata, errInfo, createMultiTxMsgAck, -6);
        return;
    }

    nlohmann::json extra;
    extra["TransactionType"] = TXTYPE_TX;
	extra["NeedVerifyPreHashCount"] = needVerifyPreHashCount;
	extra["SignFee"] = minerFees;
    extra["PackageFee"] = packageFee;   // This node is required to send a transaction on behalf of the package fee 
	outTx.set_extra(extra.dump());

    std::string serTx = outTx.SerializeAsString();

	size_t encodeLen = serTx.size() * 2 + 1;
	unsigned char encode[encodeLen] = {0};
	memset(encode, 0, encodeLen);
	long codeLen = base64_encode((unsigned char *)serTx.data(), serTx.size(), encode);
	std::string encodeStr( (char *)encode, codeLen );

	std::string encodeStrHash = getsha256hash(encodeStr);

    createMultiTxMsgAck.set_txdata(encodeStr);
    createMultiTxMsgAck.set_txencodehash(encodeStrHash);
    
	ReturnAckCode<CreateMultiTxMsgAck>(msgdata, errInfo, createMultiTxMsgAck, 0);
}

std::map<int32_t, std::string> GetMultiTxReqCode()
{
	std::map<int32_t, std::string> errInfo = {make_pair(0, "success "), 
												make_pair(-1, "Incompatible version "),
												make_pair(-2, "Incorrect signature "),
												make_pair(-3, "Open database error "),
												make_pair(-4, "Get the main chain error "),
												};

	return errInfo;												
}
void HandleMultiTxReq( const std::shared_ptr<MultiTxMsgReq>& msg, const MsgData& msgdata )
{
    TxMsgAck txMsgAck;

	auto errInfo = GetMultiTxReqCode();

    if( 0 != Util::IsVersionCompatible( msg->version() ) )
	{
		ReturnAckCode<TxMsgAck>(msgdata, errInfo, txMsgAck, -1);
		return ;
	}

    unsigned char serTxCstr[msg->sertx().size()] = {0};
	unsigned long serTxCstrLen = base64_decode((unsigned char *)msg->sertx().data(), msg->sertx().size(), serTxCstr);
	std::string serTxStr((char *)serTxCstr, serTxCstrLen);

    CTransaction tx;
    tx.ParseFromString(serTxStr);

    std::vector<SignInfo> vSignInfo;
    for (int i = 0; i < msg->signinfo_size(); ++i)
    {
        SignInfo signInfo = msg->signinfo(i);
        vSignInfo.push_back(signInfo);
    }

    if (vSignInfo.size() <= 0)
    {
        ReturnAckCode<TxMsgAck>(msgdata, errInfo, txMsgAck, -2);
    }

    // One-to-many transaction has only one initiator, take the 0th one 
    SignInfo signInfo = msg->signinfo(0);
    unsigned char strsignatureCstr[signInfo.signstr().size()] = {0};
	unsigned long strsignatureCstrLen = base64_decode((unsigned char *)signInfo.signstr().data(), signInfo.signstr().size(), strsignatureCstr);

	unsigned char strpubCstr[signInfo.pubstr().size()] = {0};
	unsigned long strpubCstrLen = base64_decode((unsigned char *)signInfo.pubstr().data(), signInfo.pubstr().size(), strpubCstr);

    for (int i = 0; i < tx.vin_size(); ++i)
    {
        CTxin * txin = tx.mutable_vin(i);

        txin->mutable_scriptsig()->set_sign( strsignatureCstr, strsignatureCstrLen );
        txin->mutable_scriptsig()->set_pub( strpubCstr, strpubCstrLen );
    }

    auto extra = nlohmann::json::parse(tx.extra());
    int needVerifyPreHashCount = extra["NeedVerifyPreHashCount"].get<int>();

	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if( txn == NULL )
	{
		ReturnAckCode<TxMsgAck>(msgdata, errInfo, txMsgAck, -3);
		return;
	}

	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, true);
	};

    std::string blockHash;
    if ( 0 != pRocksDb->GetBestChainHash(txn, blockHash) )
    {
        ReturnAckCode<TxMsgAck>(msgdata, errInfo, txMsgAck, -4);
        return;
    }

	std::string serTx = tx.SerializeAsString();
	unsigned int top = 0;
	pRocksDb->GetBlockTop(txn, top);
    
    TxMsg phoneToTxMsg;
    phoneToTxMsg.set_version(getVersion());
	phoneToTxMsg.set_tx(serTx);
	phoneToTxMsg.set_txencodehash(msg->txencodehash());
    phoneToTxMsg.set_prevblkhash(blockHash);
    phoneToTxMsg.set_trycountdown(CalcTxTryCountDown(needVerifyPreHashCount));
	phoneToTxMsg.set_top(top);

	auto txmsg = make_shared<TxMsg>(phoneToTxMsg);
	std::string txHash;
	int ret = DoHandleTx(txmsg, txHash);
	txMsgAck.set_txhash(txHash);

	if(ret != 0)
	{
		ret -= 100;
	}

	std::cout << "ret: " << ret << std::endl;

	ReturnAckCode<TxMsgAck>(msgdata, errInfo, txMsgAck, ret);
	return;
}

std::map<int32_t, std::string> GetCreatePledgeTxMsgReqCode()
{
	std::map<int32_t, std::string> errInfo = {make_pair(0, "success "), 
												make_pair(-1, "Incompatible version "), 
												make_pair(-2, "Parameter error "), 
												make_pair(-3, "Open database error "), 

												make_pair(-1001, "Parameter error when creating transaction "),
												make_pair(-1002, "Wrong transaction address when creating transaction"),
												make_pair(-1003, "When the transaction was created, the previous transaction was suspended "),
												make_pair(-1004, "Error opening database when creating transaction "),
												make_pair(-1005, "Failed to get packaging fee when creating transaction "),
												make_pair(-1006, "Failed to obtain transaction information when creating transaction "),
												make_pair(-1007, "Insufficient balance when creating transaction "),
												};

	return errInfo;												
}
void HandleCreatePledgeTxMsgReq(const std::shared_ptr<CreatePledgeTxMsgReq>& msg, const MsgData &msgdata)
{
    CreatePledgeTxMsgAck createPledgeTxMsgAck; 
	auto errInfo = GetCreatePledgeTxMsgReqCode();

	if( 0 != Util::IsVersionCompatible( getVersion() ) )
	{
        ReturnAckCode<CreatePledgeTxMsgAck>(msgdata, errInfo, createPledgeTxMsgAck, -1);
		return ;
	}
   
    uint64_t gasFee = std::stod(msg->gasfees().c_str()) * DECIMAL_NUM;
    uint64_t amount = std::stod(msg->amt().c_str()) * DECIMAL_NUM;
    uint32_t needverifyprehashcount  = std::stoi(msg->needverifyprehashcount()) ;
  
    if(msg->addr().size()<= 0 || amount <= 0 || needverifyprehashcount < (uint32_t)g_MinNeedVerifyPreHashCount || gasFee <= 0)
    {
        ReturnAckCode<CreatePledgeTxMsgAck>(msgdata, errInfo, createPledgeTxMsgAck, -2);
        return ;
    }

    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if ( txn == NULL )
	{
        ReturnAckCode<CreatePledgeTxMsgAck>(msgdata, errInfo, createPledgeTxMsgAck, -3);
		return ;
	}

	ON_SCOPE_EXIT
    {
		pRocksDb->TransactionDelete(txn, false);
	};

    std::vector<std::string> fromAddr;
    fromAddr.push_back(msg->addr());

    std::map<std::string, int64_t> toAddr;
    toAddr[VIRTUAL_ACCOUNT_PLEDGE] = amount;

    CTransaction outTx;
    int ret = TxHelper::CreateTxMessage(fromAddr, toAddr,needverifyprehashcount , gasFee, outTx);
	if(ret != 0)
	{
		ret -= 100;
        ReturnAckCode<CreatePledgeTxMsgAck>(msgdata, errInfo, createPledgeTxMsgAck, ret);
		return ;
	}

    for(int i = 0;i <outTx.vin_size();i++)
    {
        CTxin *txin = outTx.mutable_vin(i);
        txin->clear_scriptsig();
    }

    nlohmann::json txInfo;
    txInfo["PledgeType"] = PLEDGE_NET_LICENCE;
    txInfo["PledgeAmount"] = amount;

    auto extra = nlohmann::json::parse(outTx.extra());
    extra["TransactionType"] = TXTYPE_PLEDGE;
	extra["TransactionInfo"] = txInfo;
	outTx.set_extra(extra.dump());
    std::string txData = outTx.SerializePartialAsString();

    size_t encodeLen = txData.size() * 2 + 1;
	unsigned char encode[encodeLen] = {0};
	memset(encode, 0, encodeLen);
	long codeLen = base64_encode((unsigned char *)txData.data(), txData.size(), encode);

    std::string encodeStr((char *)encode, codeLen);
	std::string txEncodeHash = getsha256hash(encodeStr);
    createPledgeTxMsgAck.set_txdata(encodeStr);
    createPledgeTxMsgAck.set_txencodehash(txEncodeHash);

	ReturnAckCode<CreatePledgeTxMsgAck>(msgdata, errInfo, createPledgeTxMsgAck, 0);
    
    return ;
}

std::map<int32_t, std::string> GetPledgeTxMsgReqCode()
{
	std::map<int32_t, std::string> errInfo = {make_pair(0, "success "), 
												make_pair(-1, "Incompatible version "), 
												make_pair(-2, "Parameter error "), 
												make_pair(-3, "Open database error "), 
												make_pair(-4, "Get the main chain error "),
												make_pair(-5, "Get the highest height error "),

												};

	return errInfo;												
}
void HandlePledgeTxMsgReq(const std::shared_ptr<PledgeTxMsgReq>& msg, const MsgData &msgdata)
{
    TxMsgAck txMsgAck;

	auto errInfo = GetPledgeTxMsgReqCode();
   
	//Determine whether the version is compatible 
	if( 0 != Util::IsVersionCompatible(msg->version() ) )
	{
		ReturnAckCode<TxMsgAck>(msgdata, errInfo, txMsgAck, -1);
		return ;
	}

    if (msg->sertx().data() == nullptr || msg->sertx().size() == 0 || 
        msg->strsignature().data() == nullptr || msg->strsignature().size() == 0 || 
        msg->strpub().data() == nullptr || msg->strpub().size() == 0)
    {
   		ReturnAckCode<TxMsgAck>(msgdata, errInfo, txMsgAck, -2);
		return ;
    }

	// Reverse base64 of transaction information body, public key, and signature information 
	unsigned char serTxCstr[msg->sertx().size()] = {0};
	unsigned long serTxCstrLen = base64_decode((unsigned char *)msg->sertx().data(), msg->sertx().size(), serTxCstr);
	std::string serTxStr((char *)serTxCstr, serTxCstrLen);

	CTransaction tx;
	tx.ParseFromString(serTxStr);

	unsigned char strsignatureCstr[msg->strsignature().size()] = {0};
	unsigned long strsignatureCstrLen = base64_decode((unsigned char *)msg->strsignature().data(), msg->strsignature().size(), strsignatureCstr);

	unsigned char strpubCstr[msg->strsignature().size()] = {0};
	unsigned long strpubCstrLen = base64_decode((unsigned char *)msg->strpub().data(), msg->strpub().size(), strpubCstr);

	for (int i = 0; i < tx.vin_size(); i++)
	{
		CTxin * txin = tx.mutable_vin(i);
		txin->mutable_scriptsig()->set_sign( std::string( (char *)strsignatureCstr, strsignatureCstrLen ) );
		txin->mutable_scriptsig()->set_pub( std::string( (char *)strpubCstr, strpubCstrLen ) );  
	}

    std::string serTx = tx.SerializeAsString();
	
    auto extra = nlohmann::json::parse(tx.extra());
    int needVerifyPreHashCount = extra["NeedVerifyPreHashCount"].get<int>();

	TxMsg phoneToTxMsg;
    phoneToTxMsg.set_version(getVersion());
	phoneToTxMsg.set_tx(serTx);
	phoneToTxMsg.set_txencodehash(msg->txencodehash());

	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if( txn == NULL )
	{
        ReturnAckCode<TxMsgAck>(msgdata, errInfo, txMsgAck, -3);
        return;
	}

	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, true);
	};

    std::string blockHash;
    if ( 0 != pRocksDb->GetBestChainHash(txn, blockHash) )
    {
        ReturnAckCode<TxMsgAck>(msgdata, errInfo, txMsgAck, -4);
        return;
    }
    phoneToTxMsg.set_prevblkhash(blockHash);
    phoneToTxMsg.set_trycountdown(CalcTxTryCountDown(needVerifyPreHashCount));

	unsigned int top = 0;
	int db_status = pRocksDb->GetBlockTop(txn, top);
    if (db_status) 
    {
        ReturnAckCode<TxMsgAck>(msgdata, errInfo, txMsgAck, -5);
        return;
    }	
	phoneToTxMsg.set_top(top);

	auto txmsg = make_shared<TxMsg>(phoneToTxMsg);
	std::string txHash;
	int ret = DoHandleTx(txmsg, txHash);
	txMsgAck.set_txhash(txHash);

	if (ret != 0)
	{
		ret -= 100;
	}

	std::cout << "ret: " << ret << std::endl;

	ReturnAckCode<TxMsgAck>(msgdata, errInfo, txMsgAck, ret);
    return ;
}


std::map<int32_t, std::string> GetCreateRedeemTxMsgReqCode()
{
	std::map<int32_t, std::string> errInfo = {make_pair(0, "success "), 
												make_pair(-1, "Incompatible version "), 
												make_pair(-2, "Parameter error "), 
												make_pair(-3, "transaction is suspended "), 
												make_pair(-4, "Open database error "), 
												make_pair(-5, "Error in obtaining pledge information "),
												make_pair(-6, "Account is not pledged "),
												make_pair(-7, "No pledge information can be found "),
												make_pair(-8, "The pledge information is wrong "),
												make_pair(-9, "The pledge does not exceed the 30-day period "),
												make_pair(-10, "Failed to query local pledge information of this account "),
												make_pair(-11, "Query the local account that there is no such pledge information "),
												make_pair(-12, "Check that the account does not have enough utxo locally "),
												make_pair(-13, "Failed to get package fee "),
												};

	return errInfo;												
}
void HandleCreateRedeemTxMsgReq(const std::shared_ptr<CreateRedeemTxMsgReq>& msg,const MsgData &msgdata)
{
    CreateRedeemTxMsgAck createRedeemTxMsgAck;

	auto errInfo = GetCreateRedeemTxMsgReqCode();

    // Determine whether the version is compatible 
	if( 0 != Util::IsVersionCompatible( getVersion() ) )
	{
        ReturnAckCode<CreateRedeemTxMsgAck>(msgdata, errInfo, createRedeemTxMsgAck, -1);
		return ;
	}
    
    string fromAddr = msg->addr();
    uint64_t gasFee = std::stod(msg->gasfees().c_str()) * DECIMAL_NUM;
    uint64_t amount = std::stod(msg->amt().c_str()) * DECIMAL_NUM;
    uint32_t needverifyprehashcount  = std::stoi(msg->needverifyprehashcount()) ;
    string txhash = msg->txhash();
    
    if(fromAddr.size()<= 0 || amount <= 0 || needverifyprehashcount < (uint32_t)g_MinNeedVerifyPreHashCount || gasFee <= 0||txhash.empty())
    {
        ReturnAckCode<CreateRedeemTxMsgAck>(msgdata, errInfo, createRedeemTxMsgAck, -2);
        return ;
    }

	vector<string> Addr;
	Addr.push_back(fromAddr);
	if (MagicSingleton<TxVinCache>::GetInstance()->IsConflict(Addr))
	{
		ReturnAckCode<CreateRedeemTxMsgAck>(msgdata, errInfo, createRedeemTxMsgAck, -3);
		return ;
	}

    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if ( txn == NULL )
	{
        ReturnAckCode<CreateRedeemTxMsgAck>(msgdata, errInfo, createRedeemTxMsgAck, -4);
		return ;
	}

	ON_SCOPE_EXIT
    {
		pRocksDb->TransactionDelete(txn, false);
	};
    // Check whether the account has pledged assets 
    std::vector<string> addresses;
    int db_status = pRocksDb->GetPledgeAddress(txn, addresses);
    if(db_status != 0)
    {
        ReturnAckCode<CreateRedeemTxMsgAck>(msgdata, errInfo, createRedeemTxMsgAck, -5);
        return ;
    }

    auto iter = find(addresses.begin(), addresses.end(), fromAddr);
    if( iter == addresses.end() )
    {
        ReturnAckCode<CreateRedeemTxMsgAck>(msgdata, errInfo, createRedeemTxMsgAck, -6);
        return ;
    }

    CBlock cblock;
    string blockHeaderStr ;
    std::vector<string> utxoes;
    pRocksDb->GetPledgeAddressUtxo(txn,fromAddr, utxoes);
    if (utxoes.size() > 0)
    {
        std::string blockHash;
        pRocksDb->GetBlockHashByTransactionHash(txn, utxoes[0], blockHash); 
        int db_status1 = pRocksDb->GetBlockByBlockHash(txn,blockHash,blockHeaderStr);
    
        if(db_status1 != 0)
        {
            ReturnAckCode<CreateRedeemTxMsgAck>(msgdata, errInfo, createRedeemTxMsgAck, -7);
            return ;
        }
     }
    cblock.ParseFromString(blockHeaderStr);

    std::string utxoStr = msg->txhash();
    if (utxoStr.empty())
    {
        for (int i = 0; i < cblock.txs_size(); i++)
        {
            CTransaction tx = cblock.txs(i);
            if (CheckTransactionType(tx) == kTransactionType_Tx)
            {
                for (int j = 0; j < tx.vout_size(); j++)
                {   
                    CTxout vout = tx.vout(j);
                    if (vout.scriptpubkey() == VIRTUAL_ACCOUNT_PLEDGE)
                    {
                        utxoStr = tx.hash();
                    }
                }
            }
        }
    }

    if (utxoStr.empty())
    {
        ReturnAckCode<CreateRedeemTxMsgAck>(msgdata, errInfo, createRedeemTxMsgAck, -8);
		return ;   
    }

    //{{ Check redeem time, it must be more than 30 days, 20201209
    int result = IsMoreThan30DaysForRedeem(utxoStr);
    if (result != 0)
    {
        ReturnAckCode<CreateRedeemTxMsgAck>(msgdata, errInfo, createRedeemTxMsgAck, -9);
        return ;
    }
    //}}

    std::vector<string> utxos;
    db_status = pRocksDb->GetPledgeAddressUtxo(txn, fromAddr, utxos);
    if (db_status != 0)
    {
        ReturnAckCode<CreateRedeemTxMsgAck>(msgdata, errInfo, createRedeemTxMsgAck, -10);
        return ;
    }

    auto utxoIter = find(utxos.begin(), utxos.end(), utxoStr);
    if (utxoIter == utxos.end())
    {
        ReturnAckCode<CreateRedeemTxMsgAck>(msgdata, errInfo, createRedeemTxMsgAck, -11);
        return ;
    }
    CTransaction outTx;
    bool isTrue = FindUtxosFromRocksDb(fromAddr, fromAddr, 0, needverifyprehashcount, gasFee, outTx, utxoStr);
    for(int i = 0;i <outTx.vin_size();i++)
    {
        CTxin *txin = outTx.mutable_vin(i);
        txin->clear_scriptsig();
    }
    
	for (int i = 0; i != outTx.vin_size(); ++i)
	{
			CTxin * txin = outTx.mutable_vin(i);
			txin->clear_scriptsig();
	}

	outTx.clear_signprehash();
	outTx.clear_hash();

	if(!isTrue)
	{
        ReturnAckCode<CreateRedeemTxMsgAck>(msgdata, errInfo, createRedeemTxMsgAck, -12);
		return ;
	}

    uint64_t packageFee = 0;
    if ( 0 != pRocksDb->GetDevicePackageFee(packageFee) )
    {
        ReturnAckCode<CreateRedeemTxMsgAck>(msgdata, errInfo, createRedeemTxMsgAck, -13);
		return ;
    }

    nlohmann::json txInfo;
    txInfo["RedeemptionUTXO"] = txhash;
    txInfo["ReleasePledgeAmount"] = amount;

	nlohmann::json extra;
    extra["fromaddr"] = fromAddr;
	extra["NeedVerifyPreHashCount"] = needverifyprehashcount;
	extra["SignFee"] = gasFee;
    extra["PackageFee"] = packageFee;   // This node is required to send a transaction on behalf of the package fee 
	extra["TransactionType"] = TXTYPE_REDEEM;
    extra["TransactionInfo"] = txInfo;

	outTx.set_extra(extra.dump());
    std::string txData = outTx.SerializePartialAsString();

    size_t encodeLen = txData.size() * 2 + 1;
	unsigned char encode[encodeLen] = {0};
	memset(encode, 0, encodeLen);
	long codeLen = base64_encode((unsigned char *)txData.data(), txData.size(), encode);

    std::string encodeStr((char *)encode, codeLen);
	std::string txEncodeHash = getsha256hash(encodeStr);
    createRedeemTxMsgAck.set_txdata(encodeStr);
    createRedeemTxMsgAck.set_txencodehash(txEncodeHash);
    
	ReturnAckCode<CreateRedeemTxMsgAck>(msgdata, errInfo, createRedeemTxMsgAck, 0);
}


std::map<int32_t, std::string> GetRedeemTxMsgReqCode()
{
	std::map<int32_t, std::string> errInfo = {make_pair(0, "success "), 
												make_pair(-1, "Incompatible version "), 
												make_pair(-2, "Open database error "), 
												make_pair(-3, "Get the highest height error "),
												make_pair(-4, "Get the main chain error "),
												};

	return errInfo;												
}
void HandleRedeemTxMsgReq(const std::shared_ptr<RedeemTxMsgReq>& msg, const MsgData &msgdata )
{
    TxMsgAck txMsgAck; 
	auto errInfo = GetRedeemTxMsgReqCode();

    // Determine whether the version is compatible 
	if( 0 != Util::IsVersionCompatible(getVersion() ) )
	{
		ReturnAckCode<TxMsgAck>(msgdata, errInfo, txMsgAck, -1);
		return ;
	}
	// Reverse base64 of transaction information body, public key, and signature information 
	unsigned char serTxCstr[msg->sertx().size()] = {0};
	unsigned long serTxCstrLen = base64_decode((unsigned char *)msg->sertx().data(), msg->sertx().size(), serTxCstr);
	std::string serTxStr((char *)serTxCstr, serTxCstrLen);

	CTransaction tx;
	tx.ParseFromString(serTxStr);

	unsigned char strsignatureCstr[msg->strsignature().size()] = {0};
	unsigned long strsignatureCstrLen = base64_decode((unsigned char *)msg->strsignature().data(), msg->strsignature().size(), strsignatureCstr);

	unsigned char strpubCstr[msg->strsignature().size()] = {0};
	unsigned long strpubCstrLen = base64_decode((unsigned char *)msg->strpub().data(), msg->strpub().size(), strpubCstr);

	for (int i = 0; i < tx.vin_size(); i++)
	{
		CTxin * txin = tx.mutable_vin(i);
		txin->mutable_scriptsig()->set_sign( std::string( (char *)strsignatureCstr, strsignatureCstrLen ) );
		txin->mutable_scriptsig()->set_pub( std::string( (char *)strpubCstr, strpubCstrLen ) );
	}

    std::string serTx = tx.SerializeAsString();
    auto extra = nlohmann::json::parse(tx.extra());
    int needVerifyPreHashCount = extra["NeedVerifyPreHashCount"].get<int>();

	TxMsg phoneToTxMsg;
    phoneToTxMsg.set_version(getVersion());
	phoneToTxMsg.set_tx(serTx);
	phoneToTxMsg.set_txencodehash(msg->txencodehash());

	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if( txn == NULL )
	{
		ReturnAckCode<TxMsgAck>(msgdata, errInfo, txMsgAck, -2);
        return;
	}

	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, true);
	};

	unsigned int top = 0;
	int db_status = pRocksDb->GetBlockTop(txn, top);	
    if (db_status) 
    {
        ReturnAckCode<TxMsgAck>(msgdata, errInfo, txMsgAck, -3);
        return;
    }
	phoneToTxMsg.set_top(top);

    std::string blockHash;
    if ( 0 != pRocksDb->GetBestChainHash(txn, blockHash) )
    {
        ReturnAckCode<TxMsgAck>(msgdata, errInfo, txMsgAck, -4);
        return;
    }
    phoneToTxMsg.set_prevblkhash(blockHash);
    phoneToTxMsg.set_trycountdown(CalcTxTryCountDown(needVerifyPreHashCount));

	auto txmsg = make_shared<TxMsg>(phoneToTxMsg);
	std::string txHash;
	int ret = DoHandleTx(txmsg, txHash);
	txMsgAck.set_txhash(txHash);

	if (ret != 0)
	{
		ret -= 100;
	}

	std::cout << "ret: " << ret << std::endl;
	
	ReturnAckCode<TxMsgAck>(msgdata, errInfo, txMsgAck, ret);
    
    return; 
}

int CreatePledgeTransaction(const std::string & fromAddr,  const std::string & amount_str, uint32_t needVerifyPreHashCount, std::string gasFeeStr, std::string password, std::string pledgeType, std::string & txHash)
{
    uint64_t GasFee = std::stod(gasFeeStr.c_str()) * DECIMAL_NUM;
    uint64_t amount = std::stod(amount_str) * DECIMAL_NUM;
    if(fromAddr.size() <= 0 || amount <= 0 || needVerifyPreHashCount < (uint32_t)g_MinNeedVerifyPreHashCount || GasFee <= 0)
    {
        return -1;
    }

    // // Determine whether the miner password is correct 
    std::string hashOriPass = generateDeviceHashPassword(password);
    std::string targetPassword = Singleton<DevicePwd>::get_instance()->GetDevPassword();
    
    if (hashOriPass != targetPassword) 
    {
        return -2;
    }

    vector<string> Addr;
	Addr.push_back(fromAddr);
	if (MagicSingleton<TxVinCache>::GetInstance()->IsConflict(Addr))
	{
		return -3;
	}

    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if ( txn == NULL )
	{
		return -4;
	}

	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, true);
	};

    CTransaction outTx;
    bool isTrue = FindUtxosFromRocksDb(fromAddr, VIRTUAL_ACCOUNT_PLEDGE, amount, needVerifyPreHashCount, GasFee, outTx);
	if(!isTrue)
	{
		return -5;
	}

    nlohmann::json txInfo;
    txInfo["PledgeType"] = pledgeType;
    txInfo["PledgeAmount"] = amount;

    nlohmann::json extra;
    extra["NeedVerifyPreHashCount"] = needVerifyPreHashCount;
    extra["SignFee"] = GasFee;
    extra["PackageFee"] = 0;   // No packaging fee for this node's own initiation
    extra["TransactionType"] = TXTYPE_PLEDGE;
    extra["TransactionInfo"] = txInfo;

    outTx.set_extra(extra.dump());

    for (int i = 0; i < outTx.vin_size(); i++)
	{
		CTxin * txin = outTx.mutable_vin(i);;
		txin->clear_scriptsig();
	}

	std::string serTx = outTx.SerializeAsString();

	size_t encodeLen = serTx.size() * 2 + 1;
	unsigned char encode[encodeLen] = {0};
	memset(encode, 0, encodeLen);
	long codeLen = base64_encode((unsigned char *)serTx.data(), serTx.size(), encode);
	std::string encodeStr( (char *)encode, codeLen );

	std::string encodeStrHash = getsha256hash(encodeStr);

    if (!g_AccountInfo.SetKeyByBs58Addr(g_privateKey, g_publicKey, fromAddr.c_str())) 
    {
        return -6;
    }

	std::string signature;
	std::string strPub;
	GetSignString(encodeStrHash, signature, strPub);

	for (int i = 0; i < outTx.vin_size(); i++)
	{
		CTxin * txin = outTx.mutable_vin(i);
		txin->mutable_scriptsig()->set_sign(signature);
		txin->mutable_scriptsig()->set_pub(strPub);
	}

	serTx = outTx.SerializeAsString();

	TxMsg txMsg;
    txMsg.set_version( getVersion() );
	txMsg.set_tx(serTx);
	txMsg.set_txencodehash( encodeStrHash );

	unsigned int top = 0;
	int db_status = pRocksDb->GetBlockTop(txn, top);
    if (db_status) 
    {
        return -7;
    }	
	txMsg.set_top(top);

    std::string blockHash;
    if ( 0 != pRocksDb->GetBestChainHash(txn, blockHash) )
    {
        return -8;
    }
    txMsg.set_prevblkhash(blockHash);
    txMsg.set_trycountdown(CalcTxTryCountDown(needVerifyPreHashCount));

	auto msg = make_shared<TxMsg>(txMsg);
    int ret = DoHandleTx(msg, txHash);

    if (ret != 0)
	{
		ret -= 100;
	}

	std::cout << "ret: " << ret << std::endl;

    return ret;
}

std::map<int32_t, std::string> GetCreateDevicePledgeTxMsgReqCode()
{
	std::map<int32_t, std::string> errInfo = {make_pair(0, "success "), 
												make_pair(-1, "Incompatible version "), 
												make_pair(-2, "Incorrect password countdown is not over "), 
												make_pair(-3, "wrong password "),
												make_pair(-4, "The password is entered incorrectly for the third time "),

												};

	return errInfo;												
}
void HandleCreateDevicePledgeTxMsgReq(const std::shared_ptr<CreateDevicePledgeTxMsgReq>& msg, const MsgData &msgdata )
{
    TxMsgAck txMsgAck;
	auto errInfo = GetCreateDevicePledgeTxMsgReqCode();

	// Determine whether the version is compatible 
	if( 0 != Util::IsVersionCompatible(getVersion() ) )
	{
		ReturnAckCode<TxMsgAck>(msgdata, errInfo, txMsgAck, -1);
		return ;
	}
	
	std::string hashOriPass = generateDeviceHashPassword(msg->password());
    std::string targetPassword = Singleton<DevicePwd>::get_instance()->GetDevPassword();
  
  	auto pCPwdAttackChecker = MagicSingleton<CPwdAttackChecker>::GetInstance(); 
    uint32_t minutescount = 0;
    bool retval = pCPwdAttackChecker->IsOk(minutescount);
    if(retval == false)
    {
        ReturnAckCode<TxMsgAck>(msgdata, errInfo, txMsgAck, -2, std::to_string(minutescount));
        return ;
    }

    if(hashOriPass.compare(targetPassword))
    {
        cout<<"Enter the wrong password to start recording times "<<endl;
       if(pCPwdAttackChecker->Wrong())
       {
            ReturnAckCode<TxMsgAck>(msgdata, errInfo, txMsgAck, -3);
            return;
       }
       else
       {
            ReturnAckCode<TxMsgAck>(msgdata, errInfo, txMsgAck, -4);
            return;
       }   
    }
    else 
    {
        cout<<"Enter the password successfully reset to 0 "<<endl;
        pCPwdAttackChecker->Right();
    }


	std::string txHash;
	int ret = CreatePledgeTransaction(msg->addr(), msg->amt(), std::stoi(msg->needverifyprehashcount()), msg->gasfees(), msg->password(), PLEDGE_NET_LICENCE, txHash);
	txMsgAck.set_txhash(txHash);

	if (ret != 0)
	{
		ret -= 1000;
	}

	ReturnAckCode<TxMsgAck>(msgdata, errInfo, txMsgAck, ret);
    return ;
}

int CreateRedeemTransaction(const std::string & fromAddr, uint32_t needVerifyPreHashCount, std::string gasFeeStr, std::string utxo, std::string password, std::string & txHash)
{
    // Parameter judgment 
    uint64_t GasFee = std::stod(gasFeeStr.c_str()) * DECIMAL_NUM;
    if(fromAddr.size() <= 0 || needVerifyPreHashCount < (uint32_t)g_MinNeedVerifyPreHashCount || GasFee <= 0 || utxo.empty())
    {
        return -1;
    }

    // Determine whether the miner password is correct 
    std::string hashOriPass = generateDeviceHashPassword(password);
    std::string targetPassword = Singleton<DevicePwd>::get_instance()->GetDevPassword();
    if (hashOriPass != targetPassword) 
    {
        return -2;
    }

    vector<string> Addr;
	Addr.push_back(fromAddr);
	if (MagicSingleton<TxVinCache>::GetInstance()->IsConflict(Addr))
	{
		return -3;
	}

	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if( txn == NULL )
	{
		return -4;
	}

    ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, true);
	};

    // Check whether the account has pledged assets 
    std::vector<string> addresses;
    int db_status = pRocksDb->GetPledgeAddress(txn, addresses);
    if(db_status != 0)
    {
        return -5;
    }
    auto iter = find(addresses.begin(), addresses.end(), fromAddr);
    if( iter == addresses.end() )
    {
        return -6;
    }
   
    std::vector<string> utxos;
    db_status = pRocksDb->GetPledgeAddressUtxo(txn, fromAddr, utxos);
    if (db_status != 0)
    {
        return -7;
    }

    auto utxoIter = find(utxos.begin(), utxos.end(), utxo);
    if (utxoIter == utxos.end())
    {
        return -8;
    }

    //{{ Check time of the redeem, redeem time must be more than 30 days, add 20201208   LiuMingLiang
    int result = IsMoreThan30DaysForRedeem(utxo);
    if (result != 0)
    {
        return -9;
    }
    //}}End

    CTransaction outTx;
    bool isTrue = FindUtxosFromRocksDb(fromAddr, fromAddr, 0, needVerifyPreHashCount, GasFee, outTx, utxo);
	if(!isTrue)
	{
        return -10;
	}

    nlohmann::json txInfo;
    txInfo["RedeemptionUTXO"] = utxo;

    nlohmann::json extra;
    extra["NeedVerifyPreHashCount"] = needVerifyPreHashCount;
    extra["SignFee"] = GasFee;
    extra["PackageFee"] = 0;   // No packaging fee for this node's own initiation 
    extra["TransactionType"] = TXTYPE_REDEEM;
    extra["TransactionInfo"] = txInfo;

	outTx.set_extra(extra.dump());

	std::string serTx = outTx.SerializeAsString();

	size_t encodeLen = serTx.size() * 2 + 1;
	unsigned char encode[encodeLen] = {0};
	memset(encode, 0, encodeLen);
	long codeLen = base64_encode((unsigned char *)serTx.data(), serTx.size(), encode);
	std::string encodeStr( (char *)encode, codeLen );

	std::string encodeStrHash = getsha256hash(encodeStr);

    // Set the default account as the originating account 
    if (!g_AccountInfo.SetKeyByBs58Addr(g_privateKey, g_publicKey, fromAddr.c_str())) 
    {
        return -11;
    }

	std::string signature;
	std::string strPub;
	GetSignString(encodeStrHash, signature, strPub);

	for (int i = 0; i < outTx.vin_size(); i++)
	{
		CTxin * txin = outTx.mutable_vin(i);
		txin->mutable_scriptsig()->set_sign(signature);
		txin->mutable_scriptsig()->set_pub(strPub);
	}

	serTx = outTx.SerializeAsString();
	
	TxMsg txMsg;
	txMsg.set_version( getVersion() );

	txMsg.set_tx(serTx);
	txMsg.set_txencodehash( encodeStrHash );

	unsigned int top = 0;
	db_status = pRocksDb->GetBlockTop(txn, top);
    if (db_status) 
    {
        return -12;
    }
	txMsg.set_top(top);

    std::string blockHash;
    if ( 0 != pRocksDb->GetBestChainHash(txn, blockHash) )
    {
        return -13;
    }
    txMsg.set_prevblkhash(blockHash);
    txMsg.set_trycountdown(CalcTxTryCountDown(needVerifyPreHashCount));

	auto msg = make_shared<TxMsg>(txMsg);
    int ret = DoHandleTx(msg, txHash);

	if (ret != 0)
	{
		ret -= 100;
	}

	std::cout << "ret: " << ret << std::endl;

    return ret;
}


std::map<int32_t, std::string> GetCreateDeviceRedeemTxMsgReqCode()
{
	std::map<int32_t, std::string> errInfo = {make_pair(0, "success "), 
												make_pair(-1, "Incompatible version "), 
												make_pair(-2, "Incorrect password countdown is not over "), 
												make_pair(-3, "wrong password "),
												make_pair(-4, "The password is entered incorrectly for the third time "),
												};

	return errInfo;												
}

// Mobile phone connects to miner to initiate Depledge transaction 
void HandleCreateDeviceRedeemTxMsgReq(const std::shared_ptr<CreateDeviceRedeemTxReq> &msg, const MsgData &msgdata )
{
	TxMsgAck txMsgAck;
	auto errInfo = GetCreateDeviceRedeemTxMsgReqCode();

	// Determine whether the version is compatible 
	if( 0 != Util::IsVersionCompatible(getVersion() ) )
	{
		ReturnAckCode<TxMsgAck>(msgdata, errInfo, txMsgAck, -1);
		return ;
	}
	
	std::string hashOriPass = generateDeviceHashPassword(msg->password());
    std::string targetPassword = Singleton<DevicePwd>::get_instance()->GetDevPassword();
  
  	auto pCPwdAttackChecker = MagicSingleton<CPwdAttackChecker>::GetInstance(); 
    uint32_t minutescount = 0;
    bool retval = pCPwdAttackChecker->IsOk(minutescount);
    if(retval == false)
    {
        ReturnAckCode<TxMsgAck>(msgdata, errInfo, txMsgAck, -2, std::to_string(minutescount));
        return;
    }

    if(hashOriPass.compare(targetPassword))
    {
        cout<<"Enter the wrong password to start recording times "<<endl;
       if(pCPwdAttackChecker->Wrong())
       {
            ReturnAckCode<TxMsgAck>(msgdata, errInfo, txMsgAck, -3);
            return;
       }
       else
       {
            ReturnAckCode<TxMsgAck>(msgdata, errInfo, txMsgAck, -4);
            return;
       }   
    }
    else 
    {
        cout<<"Enter the password successfully reset to 0 "<<endl;
        pCPwdAttackChecker->Right();
    }

	std::string txHash;
    int ret = CreateRedeemTransaction(msg->addr(), std::stoi(msg->needverifyprehashcount()), msg->gasfees(), msg->utxo(), msg->password(), txHash);
	txMsgAck.set_txhash(txHash);

	if (ret != 0)
	{
		ret -= 1000;
	}

	ReturnAckCode<TxMsgAck>(msgdata, errInfo, txMsgAck, ret);
    return ;
}

template<typename Ack>
void ReturnAckCode(const MsgData& msgdata, std::map<int32_t, std::string> errInfo, Ack & ack, int32_t code, const std::string & extraInfo)
{
	ack.set_version(getVersion());
	ack.set_code(code);
	if (extraInfo.size())
	{
		ack.set_message(extraInfo);
	}
	else
	{
		ack.set_message(errInfo[code]);
	}

	net_send_message<Ack>(msgdata, ack, net_com::Priority::kPriority_High_1); // ReturnAckCode mostly handles transactions, the default priority is high1 
}

template<typename TxReq> 
int CheckAddrs( const std::shared_ptr<TxReq>& req)
{
	if (req->from_size() > 1 && req->to_size() > 1)
    {
		return -1;
    }
	if (req->from_size() == 0 || req->to_size() == 0)
    {
		return -2;
    }
	return 0;
}

int GetAddrsFromMsg( const std::shared_ptr<CreateMultiTxMsgReq>& msg, 
                     std::vector<std::string> &fromAddr,
                     std::map<std::string, int64_t> &toAddr)
{
    for (int i = 0; i < msg->from_size(); ++i)
    {
        std::string fromAddrStr = msg->from(i);
        fromAddr.push_back(fromAddrStr);
    }

    for (int i = 0; i < msg->to_size(); ++i)
    {
        ToAddr toAddrInfo = msg->to(i);
        if (toAddrInfo.toaddr().empty() && toAddrInfo.amt().empty())
        {
            error("parse error! toaddr or amt is empty!");
            return -1;
        }

        int64_t amount = std::stod(toAddrInfo.amt()) * DECIMAL_NUM;
        toAddr.insert( make_pair(toAddrInfo.toaddr(), amount ) );
    }

    if (fromAddr.size() == 0 || toAddr.size() == 0)
    {
        return -2;
    }

    if (toAddr.size() != (size_t)msg->to_size())
    {
        return -3;
    }

    return 0;
}

// Check time of the redeem, redeem time must be more than 30 days, add 20201208   LiuMingLiang
int IsMoreThan30DaysForRedeem(const std::string& utxo)
{
	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if (txn == NULL)
	{
		return -1;
	}

    ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, false);
	};

    std::string strTransaction;
    int db_status = pRocksDb->GetTransactionByHash(txn, utxo, strTransaction);
    if (db_status != 0)
    {
        return -1;
    }

    CTransaction utxoPledge;
    utxoPledge.ParseFromString(strTransaction);
    uint64_t nowTime = Singleton<TimeUtil>::get_instance()->getlocalTimestamp();
    static const uint64_t DAYS30 = (uint64_t)1000000 * 60 * 60 * 24 * 30;
    if ((nowTime - utxoPledge.time()) >= DAYS30)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

// Create: Check chain height of the all node, 20210225
bool CheckAllNodeChainHeightInReasonableRange()
{
	// Get chain height
    unsigned int chainHeight = get_chain_height();
	if (chainHeight == 0)
	{
		return true;
	}

	unsigned int reasonableCount = 0;
	std::vector<Node> nodelist;
	if (Singleton<PeerNode>::get_instance()->get_self_node().is_public_node)
	{
		nodelist = Singleton<PeerNode>::get_instance()->get_nodelist();
	}
	else
	{
		nodelist = Singleton<NodeCache>::get_instance()->get_nodelist();
	}
	if (nodelist.empty())
	{
		// std::cout << "NodeCache is empty!" << std::endl;
		return true;
	}
	for (auto& node : nodelist)
	{
		std::cout << "Node chain height: " << node.id << " " << node.chain_height << " " << chainHeight << std::endl;
		int difference = node.chain_height - chainHeight;
		if (abs(difference) <= REASONABLE_HEIGHT_RANGE)
		{
			++reasonableCount;
		}
	}

	float fTotalCount = nodelist.size();
	float fReasonableCount = reasonableCount;
	static const float STANDARD_VALUE = 0.60;
	bool result = ((fReasonableCount / fTotalCount) >= STANDARD_VALUE);
	std::cout << "Check chain height: " << fReasonableCount << ", " << fTotalCount << ", ^^^VVV " << result << std::endl;

	return result;
}

// Description: handle the ConfirmTransactionReq from network,   20210309   Liu
void HandleConfirmTransactionReq(const std::shared_ptr<ConfirmTransactionReq>& msg, const MsgData& msgdata)
{
	std::string version = msg->version();
	std::string id = msg->id();
	std::string txHash = msg->tx_hash();
	
	bool success = false;
	CBlock block;

	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if (txn != nullptr)
	{
		std::string txRaw;
		int db_status = pRocksDb->GetTransactionByHash(txn, txHash, txRaw);
		if (db_status == 0)
		{
			success = true;
			string blockHash;
			pRocksDb->GetBlockHashByTransactionHash(txn, txHash, blockHash);
			string blockRaw;
			pRocksDb->GetBlockByBlockHash(txn, blockHash, blockRaw);
			block.ParseFromString(blockRaw);
			std::cout << "In confirm transaction, Get block success.^^^VVV " << blockHash << std::endl;
		}
		pRocksDb->TransactionDelete(txn, true);
	}

	ConfirmTransactionAck ack;
	ack.set_version(getVersion());
	ack.set_id(net_get_self_node_id());
	ack.set_tx_hash(txHash);
	ack.set_flag(msg->flag());
	// std::cout << "Response confirm: " << txHash << ", success=" << success  << ", flag=" << (int)ack.flag() << std::endl;
	if (success)
	{
		ack.set_success(true);
		std::string blockRaw = block.SerializeAsString();
		ack.set_block_raw(blockRaw);
	}
	else
	{
		ack.set_success(false);
	}

	net_send_message<ConfirmTransactionAck>(id, ack);
}

void HandleConfirmTransactionAck(const std::shared_ptr<ConfirmTransactionAck>& msg, const MsgData& msgdata)
{
	std::string ver = msg->version();
	std::string id = msg->id();
	std::string tx_hash = msg->tx_hash();
	bool success = msg->success();
	std::cout << "Receive confirm: " << id << ", " << tx_hash << ", " << success << std::endl;
	if (success)
	{
		std::string blockRaw = msg->block_raw();
		CBlock block;
		block.ParseFromString(blockRaw);

		if (msg->flag() == ConfirmTxFlag)
		{
			MagicSingleton<TransactionConfirmTimer>::GetInstance()->update_count(tx_hash, block);
		}
		else if (msg->flag() == ConfirmRpcFlag)
		{
			g_RpcTransactionConfirm.update_count(tx_hash, block);
			g_RpcTransactionConfirm.update_id(tx_hash,id);
		}
	}
	else
	{
	}
}

void SendTransactionConfirm(const std::string& tx_hash, ConfirmCacheFlag flag, const uint32_t confirmCount)
{
	if (confirmCount == 0)
	{
		std::cout << "confirmCount is empty" << std::endl;
		return;
	}

	ConfirmTransactionReq req;
	req.set_version(getVersion());
	req.set_id(net_get_self_node_id());
	req.set_tx_hash(tx_hash);
	req.set_flag(flag);

	std::vector<Node> list;
	if (Singleton<PeerNode>::get_instance()->get_self_node().is_public_node)
	{
		list = Singleton<PeerNode>::get_instance()->get_nodelist();
	}
	else
	{
		list = Singleton<NodeCache>::get_instance()->get_nodelist();
	}

	std::random_device device;
	std::mt19937 engine(device());

	int send_size = std::min(list.size(), (size_t)confirmCount);

	int count = 0;
	while (count < send_size && !list.empty())
	{
		int index = engine() % list.size();

		net_send_message<ConfirmTransactionReq>(list[index].id, req);
		++count;
		std::cout << "Send to confirm: " << index << ", " << list[index].id << ", " << count << std::endl;

		list.erase(list.begin() + index);
	}
}
