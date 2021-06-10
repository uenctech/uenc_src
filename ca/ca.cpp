#include "./ca.h"
#include "ca_buffer.h"
#include "ca_transaction.h"
#include "ca_global.h"
#include "ca_interface.h"
#include "ca_test.h"
#include "unistd.h"
#include "ca_hexcode.h"
#include "../include/net_interface.h"
#include "ca_message.h"
#include "ca_coredefs.h"
#include "ca_console.h"
#include "ca_base64.h"
#include "ca_blockpool.h"
#include "ca_rocksdb.h"
#include "ca_txhelper.h"
#include "ca_test.h"
#include "ca_AwardAlgorithm.h"
#include "ca_rollback.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <random>
#include <map>
#include <array>
#include <fcntl.h>
#include <thread>
#include <shared_mutex>

#include "ca_device.h"
#include "ca_header.h"
#include "ca_clientinfo.h"
#include "../utils/string_util.h"
#include "../utils//util.h"
#include "../utils/time_util.h"
#include "../net/ip_port.h"
//protoc
#include "../proto/interface.pb.h"
#include "../proto/ca_test.pb.h"
#include "ca_MultipleApi.h"
#include "ca_synchronization.h"
#include "ca_txhelper.h"
#include "ca_bip39.h"
#include "../utils/qrcode.h"
#include "ca_txvincache.h"
#include "ca_txfailurecache.h"
#include "ca_txconfirmtimer.h"
#include "../ca/ca_http_api.h"
#include "../net/net_api.h"
#include "../net/node_cache.h"
#include "ca_block_http_callback.h"

const char *kVersion = "1.0.1";
std::shared_mutex g_NodeInfoLock;
bool bStopTx = false;
bool bIsCreateTx = false;

void InitFee();
void RegisterCallback();
void TestCreateTx(const std::vector<std::string> & addrs, const int & sleepTime, const std::string & signFee);


std::string PrintTime(uint64_t time)
{
	
	time_t s = (time_t)time / 1000000;
    struct tm * gm_date;
    gm_date = localtime(&s);
    char ps[512] = {};
	sprintf(ps, "%d-%d-%d %d:%d:%d (%zu) \n", gm_date->tm_year + 1900, gm_date->tm_mon + 1, gm_date->tm_mday, gm_date->tm_hour, gm_date->tm_min, gm_date->tm_sec, time);
    return string(ps);
}

int GetAllAccountSignNum()
{
	std::map<std::string, uint64_t> addrSignNum;
	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if( txn == NULL )
	{
		std::cout << "(AddBlock) TransactionInit failed !" << std::endl;
		return -1;
	}

	bool bRollback = true;
	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, bRollback);
	};

	ofstream file("sign.txt");
	if( !file.is_open() )
	{
		std::cout << "open a file  sign.txt fail！" << std::endl;
		return -2;
	}

	unsigned int blockHeight = 0;
	auto db_status = pRocksDb->GetBlockTop(txn, blockHeight);
	if (db_status != 0)
	{
		std::cout << "GetBlockTop fail！" << std::endl;
		bRollback = true;
        return -3;
    }

	// When the height of the local node is 0 
	if(blockHeight <= 0)
	{
		std::cout << "Height is 0 ！" << std::endl;
		return -4;
	}

	for(unsigned int height = 1; height <= blockHeight; height++)
	{
		std::vector<std::string> hashs;
		db_status = pRocksDb->GetBlockHashsByBlockHeight(txn, height, hashs);
		if(db_status != 0) 
		{
			std::cout << "GetBlockHashsByBlockHeight fail！" << std::endl;
			bRollback = true; 
			return -3;
		}

		for(auto blockHash : hashs)
		{
			std::string blockHeaderStr;
			db_status = pRocksDb->GetBlockByBlockHash(txn, blockHash, blockHeaderStr);
			if(db_status != 0)
			{
				std::cout << "GetBlockByBlockHash fail！" << std::endl;
				bRollback = true; 
				return -3;
			}

			CBlock cblock;
			cblock.ParseFromString(blockHeaderStr);

			for (int txCount = 0; txCount < cblock.txs_size(); txCount++)
			{
				CTransaction transaction = cblock.txs(txCount);
				if ( CheckTransactionType(transaction) == kTransactionType_Award)
				{
					CTransaction copyTx = transaction;
					for (int voutCount = 0; voutCount != copyTx.vout_size(); ++voutCount)
					{
						CTxout txout = copyTx.vout(voutCount);
						auto iter = addrSignNum.find( txout.scriptpubkey() );
						if( iter != addrSignNum.end() )
						{
							int tmpValue = iter->second;
							tmpValue += txout.value();
							addrSignNum.insert( make_pair(iter->first, iter->second) );
						}
						else
						{
							addrSignNum.insert( make_pair( txout.scriptpubkey(), txout.value() ) );
						}	
					}
				}
			}
		}
	}

	file << std::string("Total number of accounts ：");
	file << std::to_string(addrSignNum.size()) << "\n";
	for(auto addrSignAmt : addrSignNum)
	{
		file << addrSignAmt.first;
		file << ",";
		file << addrSignAmt.second;
		file << "\n";
	}

	file.close();
	return 0;
}

bool isInteger(string str) 
{
    //Judging the case where there is no input 
    if(str=="")
    {
        return false;
    }
    else 
    {
        //With input 
        for (size_t i = 0; i < str.size();i++) 
        {
            if (str.at(i) == '-' && str.size() > 1)  // There may be negative numbers 
                continue;
            // The value is between ‘0’ and ‘9’ of the ascii code (code) 
            if (str.at(i) > '9' || str.at(i) < '0')
                return false;
        }
        return true;
    }
}


int InitRocksDb()
{
    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
    if (!pRocksDb)
    {
        return -1;
    }

	Transaction* txn = pRocksDb->TransactionInit();
	if( txn == NULL )
	{
		std::cout << "(rocksdb init) TransactionInit failed !" << std::endl;
		return -1;
	}
	
	bool bRollback = true;
	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, bRollback);
	};


    std::string bestChainHash;
    pRocksDb->GetBestChainHash(txn, bestChainHash);
    if (bestChainHash.size() == 0)
    {
		//  Initialize block 0, here to simplify the processing, directly read the key information and save it in the database 
        std::string  strBlockHeader0;
        if(g_testflag == 1)
        {
            strBlockHeader0 = "08011240663431336131653831396533376335643538346331636662353835616161343364386339383133663464396633613834316537393065306136366530366134661a40303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030302a403835666466326630663834656331313234626132626336623239613665373431383134303366383232326665393232303435633036653339303165616131343532b402080110a69995f284dbeb02222131766b533436516666654d3473444d42426a754a4269566b4d514b59375a3854753a40383566646632663066383465633131323462613262633662323961366537343138313430336638323232666539323230343563303665333930316561613134354294010a480a403030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303010ffffffff0f12420a4047454e4553495320202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202018ffffffff0f4a2b088080e983b1de16122131766b533436516666654d3473444d42426a754a4269566b4d514b59375a3854753a240a223030303030303030303030303030303030303030303030303030303030303030303050a69995f284dbeb02";
        }
        else
        {
            strBlockHeader0 = "08011240306162656230393963333866646635363363356335396231393365613738346561643530343938623135656264613639376432333264663134386362356237641a40303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030302a403432613165336664353035666532613764343032366533326430633266323537303864633464616338343739366366373237343766326238353433653135623232b60208011099f393bcaae0ea0222223136707352697037385176557275517239664d7a7238456f6d74465331625661586b3a40343261316533666435303566653261376434303236653332643063326632353730386463346461633834373936636637323734376632623835343365313562324294010a480a403030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303010ffffffff0f12420a4047454e4553495320202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202018ffffffff0f4a2c088080feeabaa41b12223136707352697037385176557275517239664d7a7238456f6d74465331625661586b3a240a223136707352697037385176557275517239664d7a7238456f6d74465331625661586b5099f393bcaae0ea02";
        }
        
	    int size = strlen(strBlockHeader0.c_str());
	    char * buf = new char[size]{0};
	    size_t outLen = 0;
	    decode_hex(buf, size, strBlockHeader0.c_str(), &outLen);

		CBlock header;
		header.ParseFromString(std::string(buf));
        delete [] buf;

		CBlockHeader block;
        block.set_height(0);
		block.set_prevhash(header.prevhash());
		block.set_hash(header.hash());
		block.set_time(header.time());

		std::string serBlock = block.SerializeAsString();
		pRocksDb->SetBlockHeaderByBlockHash(txn, block.hash(), serBlock);

		if (header.txs_size() == 0)
		{
			bRollback = true;
			return -2;
		}

		CTransaction tx = header.txs(0);
		if (tx.vout_size() == 0)
		{
			bRollback = true;
			return -3;
		}

        pRocksDb->SetBlockHeightByBlockHash(txn, block.hash(), block.height());
        pRocksDb->SetBlockHashByBlockHeight(txn, block.height(), block.hash(), true);

        pRocksDb->SetBlockByBlockHash(txn, block.hash(), header.SerializeAsString());
        unsigned int height = 0;
        pRocksDb->SetBlockTop(txn, height);
        pRocksDb->SetBestChainHash(txn, block.hash());
        pRocksDb->SetUtxoHashsByAddress(txn, tx.txowner(), tx.hash());
        pRocksDb->SetTransactionByHash(txn, tx.hash(), tx.SerializeAsString());
        pRocksDb->SetBlockHashByTransactionHash(txn, tx.hash(), header.hash());

        unsigned int txIndex = 0;
        pRocksDb->SetTransactionByAddress(txn, tx.txowner(), txIndex, tx.SerializeAsString());
        pRocksDb->SetTransactionTopByAddress(txn, tx.txowner(), txIndex);

        CTxout vout = tx.vout(0);
        pRocksDb->SetBalanceByAddress(txn, tx.txowner(), vout.value());
        pRocksDb->SetAllTransactionByAddress(txn, tx.txowner(), tx.hash());
    }

    if (pRocksDb->SetInitVer(txn, getVersion()) != 0)
    {
        bRollback = true;
		std::cout << " init ver failed !" << std::endl;
		return -4;
    }
    
    ;

	if( pRocksDb->TransactionCommit(txn) )
	{
		bRollback = true;
		std::cout << "(rocksdb init) TransactionCommit failed !" << std::endl;
		return -5;
	}

	return 0;
}

void test_time() {
    tm tmp_time;

    string date_s, date_e;
    cout << "Please enter start date (Format 2020-01-0 1): ";
    cin >> date_s;
    date_s += " 00:00:00";
    strptime(date_s.c_str(), "%Y-%m-%d %H:%M:%S", &tmp_time);
    uint64_t t_s = (uint64_t)mktime(&tmp_time);
    cout << t_s << endl;

    cout << "Please enter the end date (format 2020-01-07) : ";
    cin >> date_e;
    date_e += " 23:59:59";
    strptime(date_e.c_str(), "%Y-%m-%d %H:%M:%S", &tmp_time);
    uint64_t t_e = (uint64_t)mktime(&tmp_time);
    cout << t_e << endl;

    auto rdb_ptr = MagicSingleton<Rocksdb>::GetInstance();
    Transaction* txn = rdb_ptr->TransactionInit();
    if (txn == NULL) {
        std::cout << "(GetBlockInfoAck) TransactionInit failed !" <<  __LINE__ << std::endl;
    }

    ON_SCOPE_EXIT {
        rdb_ptr->TransactionDelete(txn, false);
    };

    unsigned top = 0;
    rdb_ptr->GetBlockTop(txn, top);

    map<string, uint64_t> addr_award;
    std::vector<std::string> hashs;
    bool b {false};

    while (true) {
        auto db_status = rdb_ptr->GetBlockHashsByBlockHeight(txn, top, hashs);
        for (auto v : hashs) {
            std::string header;
            db_status = rdb_ptr->GetBlockByBlockHash(txn, v, header);
            cout << __LINE__ << " " << db_status << endl;
            CBlock cblock;
            cblock.ParseFromString(header);
            for (auto i = 2; i < cblock.txs_size(); i += 3) {
                CTransaction tx = cblock.txs(i);
                if (tx.time()/1000000 > t_s && tx.time()/1000000 < t_e) {
                    for (int j = 0; j < tx.vout_size(); j++) {
                        //cout << s++ << " top " << top << endl;
                        CTxout txout = tx.vout(j);
                        //cout << "scriptPubKey " << txout.scriptpubkey() << " txout.value() " << txout.value() << endl;
                        addr_award[txout.scriptpubkey()] += txout.value();
                    }
                }

                if (tx.time()/1000000 < t_s) {
                    b = true;
                }
            }

            if (b) {
                break;
            }
        }
        hashs.clear();
        if (b) {
            break;
        }
        top--;
    }

    for (auto v : addr_award) {
        cout << "addr " << v.first << " award " << v.second << endl;
    }
}

void InitTxCount() 
{
    size_t b_count {0};
    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
    Transaction* txn = pRocksDb->TransactionInit();
    if( txn == NULL )
    {
        std::cout << "(ca->case 5:) TransactionInit failed !" << std::endl;
    }

    ON_SCOPE_EXIT{
        pRocksDb->TransactionDelete(txn, true);
    };

    pRocksDb->GetTxCount(txn, b_count);

    if (!b_count) {
        unsigned int top {0};
        pRocksDb->GetBlockTop(txn, top);
        auto amount = to_string(top);

        std::vector<std::string> vBlockHashs;

        for (size_t i = top; i > 0; --i) {
            pRocksDb->GetBlockHashsByBlockHeight(txn, i, vBlockHashs);
            b_count += vBlockHashs.size();
            cout << "height------- " << i << endl;
            cout << "count======= " << vBlockHashs.size() << endl;
            vBlockHashs.clear();
        }


        pRocksDb->SetTxCount(txn, b_count);
        pRocksDb->SetGasCount(txn, b_count);
        pRocksDb->SetAwardCount(txn, b_count);

    }

}

int ca_startTimerTask()
{
    //Block thread 
    g_blockpool_timer.AsyncLoop(1000, [](){
		MagicSingleton<BlockPoll>::GetInstance()->Process();
	});

	//Block synchronization thread 
	int sync_time = Singleton<Config>::get_instance()->GetSyncDataPollTime();
    cout<<"sync_time = "<<sync_time<<endl;
    g_synch_timer.AsyncLoop(sync_time * 1000 , [](){
		g_synch->Process();
	});
    
    //Get the online duration of the miner 
    g_deviceonline_timer.AsyncLoop(5 * 60 * 1000, GetOnLineTime);
    
    // Public node refresh to config file, 20201110
    g_public_node_refresh_timer.AsyncLoop(60 * 60 * 1000, [](){ UpdatePublicNodeToConfigFile(); });

    MagicSingleton<TxVinCache>::GetInstance()->Process();

    g_blockpool_timer.AsyncLoop(1000, [](){
		MagicSingleton<BlockPoll>::GetInstance()->Process();
	});

    MagicSingleton<TransactionConfirmTimer>::GetInstance()->timer_start();

    // Run http callback
    string url = Singleton<Config>::get_instance()->GetHttpCallbackIp();
    int port = Singleton<Config>::get_instance()->GetHttpCallbackPort();
    if (!url.empty() && port > 0)
    {
        MagicSingleton<CBlockHttpCallback>::GetInstance()->Start(url, port);
    }
    else
    {
        cout << "Http callback is not config!" << endl;
    }

    return 0;
}

bool ca_init()
{               
    // Initialize account 
    InitAccount(&g_AccountInfo, "./cert");

    // Initialize the database 
    InitRocksDb();
    
    //Mobile phone interface registration 
    MuiltipleApi();
    
    // Initialize mining fee and packing fee settings 
    InitFee();
    
    // Register the interface with the network layer 
    RegisterCallback();

    // Register http related interface 
    ca_register_http_callbacks();

    // Initialize transaction statistics 
    InitTxCount();

    // Start timer task 
    ca_startTimerTask();
    Singleton<Config>::get_instance()->UpdateNewConfig();
    return true;
}


int ca_endTimerTask()
{
    g_blockpool_timer.Cancel();

    g_synch_timer.Cancel();
    
    g_deviceonline_timer.Cancel();

    g_public_node_refresh_timer.Cancel();

	return true;
}

void ca_cleanup()
{
    ca_endTimerTask();

	MagicSingleton<Rocksdb>::DesInstance();
}


void ca_print()
{
    using std::cout;
    using std::cin;
    using std::endl;
    using std::string;
    using std::map;
    using std::vector;
    using std::array;

	std::cout << "ca v" << ca_version() << std::endl << std::endl;
	std::string FromAddr, ToAddr, amt; // TODO

	int iswitch = -1;
    while(0 != iswitch)
    {
        std::cout << "1. Generate Public Key and Private Key." << std::endl;
		std::cout << "2. Get User's Amount." << std::endl;
        std::cout << "3. Create TX." << std::endl;
		std::cout << "4. CreateCoinBaseTx." <<std::endl;
        std::cout << "5. PrintBlockInfo." << std::endl;
        std::cout << "6. Rollback Block." << std::endl;
        std::cout << "7. Get Block." << std::endl;
		std::cout << "8. print all block." << std::endl;
		std::cout << "9. Test." << std::endl;
		std::cout << "10. ExitNode." << std::endl;
		std::cout << "11. sync block form other node." << std::endl;
        std::cout << "12. Print pending transaction in cache." << std::endl;
        std::cout << "13. Print failure transaction in cache." << std::endl;
        std::cout << "14. Clear pending transaction in cache." << std::endl;
        std::cout << "0. exit."<<std::endl;

		std::cout<<"AddrList : "<<std::endl;
		g_AccountInfo.print_AddrList();

		std::cout<<"Please Select ... -> ";
		std::string pswitch;

        std::cin>>pswitch;

		if(pswitch.size() > 2 || pswitch.c_str()[0] < '0' || pswitch.c_str()[0] > '9')
		{
			continue;
		}

		iswitch = atoi(pswitch.c_str());
        switch(iswitch)
        {
            case 1:
			{
                std::cout << "Please enter the number of accounts to be generated ：";
                int num = 0;
                std::cin >> num;
                if (num <= 0)
                {
                    break;
                }

                for (int i = 0; i != num; ++i)
                {
                    g_AccountInfo.GenerateKey();
                    std::cout<<"DefaultKeyAddr: ["<<g_AccountInfo.DefaultKeyBs58Addr<<"]"<<std::endl;    
                }

                break;
			}
			case 2:
			{
				std::cout <<"Input User's Address:"  << std::endl;
				std::cin >> FromAddr;

				std::string addr;

				if (FromAddr.size() == 1)
				{
					addr = g_AccountInfo.DefaultKeyBs58Addr;
				}
				else
				{
					addr = FromAddr;
				}

				uint64_t amount = CheckBalanceFromRocksDb(addr.c_str());
				std::cout << "User Amount[" << addr.c_str() << "] : " << amount << std::endl;
				info("User Amount[%s] : %ld", addr.c_str(), amount );

				break;
			}
			case 3:
			{
                handle_transaction();
                break;
			}
			 case 4:
			 {
                //  MagicSingleton<CBlockHttpCallback>::GetInstance()->Test();
                //  MagicSingleton<CBlockHttpCallback>::GetInstance()->Test2();
				// std::cout<<"input FromAddr :"<<std::endl;
				// std::cin>>FromAddr;
				// std::cout<<"input ToAddr :"<<std::endl;
				// std::cin>>ToAddr;
				// std::cout<<"input amount :"<<std::endl;
				// std::cin>>amt;

				// char *phoneBuf = NULL;
				// int phoneBufLen = 0;

				// if(1 == FromAddr.size())
				// {
				// 	CreateTx(NULL, ToAddr.c_str(), amt.c_str(), true, false, NULL, &phoneBuf, &phoneBufLen);
				// }
				// else
				// {
				// 	if(FromAddr != ToAddr)
				// 	{
				// 		error("input Error !!!  FromAddr need to same to ToAddr");
				// 		printf("input Error !!!  FromAddr need to same to ToAddr");
				// 		break;
				// 	}
				// 	CreateTx(FromAddr.c_str(), ToAddr.c_str(), amt.c_str(), true, false, NULL, &phoneBuf, &phoneBufLen);
				// }
                GetNodeInfoReq  getNodeInfoReq;
               getNodeInfoReq.set_version(getVersion());
               // GetDevPrivateKeyReq getDevPrivateKeyReq;
                // GetServiceInfoReq getServiceInfoReq;
                // getServiceInfoReq.set_version(getVersion());
                // int number;
                // cout<<"Please enter the serial number ："<<endl;
                // cin >> number;
                // getServiceInfoReq.set_sequence_number(number);
                // // std::string base58addr; 
                // // cout<<"please enter base58addr"<<endl;
                // // cin >> base58addr;
                // // getDevPrivateKeyReq.set_bs58addr(base58addr);
                // // std::string passwd; 
                // // cout<<"please enter passwd"<<endl;
                // // cin >> passwd;
                // // getDevPrivateKeyReq.set_password(passwd);
               
                std::string id ;
                cout<<"Please enter id account "<<endl;
                cin >> id;
                
                // std::string pwd;
                // cout<<"Please enter the password "<<endl;
                // cin >> pwd;
                // GetTxByHashReq TxByHashReq;
                // TxByHashReq.add_txhash("f3eb346651f16cfaf6cb1b6fa349315476c25bf293495d7509822483b627a7b6");
                // // SetServiceFeeReq setServiceFeeReq;
                //  TxByHashReq.set_version(getVersion());
                // // setServiceFeeReq.set_password(pwd);
                // setServiceFeeReq.set_service_fee("0.001");
                net_send_message<GetNodeInfoReq>(id, getNodeInfoReq);
                // net_send_message<GetServiceInfoReq>(id, getServiceInfoReq);
                net_register_callback<GetNodeInfoAck>([] (const std::shared_ptr<GetNodeInfoAck> & ack, const MsgData & msgdata){
                   
                   for(size_t i = 0; i != (size_t)ack->node_list_size(); ++i)
                   {
                       const NodeList & item = ack->node_list(i);
                      // const NodeInfos &info = item.node_info(i);
                
                    //    for (auto info : item.node_info())
                    //     {
                    //         std::cout << "info: " << info << std::endl;    
                    //     }
                        for (int j = 0 ;j < item.node_info_size();j++)
                        {
                            std::cout << "enable: " << item.node_info(j).enable() << std::endl;
                            std::cout << "ip: " << item.node_info(j).ip() << std::endl;   
                            std::cout << "name: " << item.node_info(j).name() << std::endl;   
                            std::cout << "port: " << item.node_info(j).port() << std::endl;  
                            std::cout << "price: " << item.node_info(j).price() << std::endl;     
                        }
                   }
                });
               
                // GetServiceInfoReqTestHandle(const std::shared_ptr<ServiceInfoReq>& msg,const MsgData& msgdata);
                //std::shared_ptr<ServiceInfoReq> req = std::make_shared<ServiceInfoReq>();
                // ServiceInfoReq req;
                // req.set_version(getVersion());
                // req.set_sequence_number(1);
                // std::string id ;
                // cout<<"Please enter id account "<<endl;
                // cin >> id;
                // net_send_message<ServiceInfoReq>(id, req);
                // GetServiceInfoAck ack;
                // m_api::HandleGetPledgeListReq(req, ack);
               // m_api::HandleGetServiceInfoReqTest(req,);
               // TestSetOnLineTime();
                break;
			}
			case 5:
			{
                auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
				Transaction* txn = pRocksDb->TransactionInit();
				if( txn == NULL )
				{
					std::cout << "(ca->case 5:) TransactionInit failed !" << std::endl;
					break;
				}

				ON_SCOPE_EXIT{
					pRocksDb->TransactionDelete(txn, true);
				};

                unsigned int top {0};
                pRocksDb->GetBlockTop(txn, top);
                auto amount = to_string(top);
                std::cout << "amount: " << amount << std::endl;

                size_t input_options {1};
                std::set<std::string> options {"1. Get the total number of transactions", "2. Get transaction block details",
                 "3. View the current block reward and total block reward", "4. View the cumulative amount of signature accounts in all blocks", "5. Request for device password",
                 "6Set device password request","7Get device private key request ",};
                for (auto v : options) 
                {
                    cout << v << endl;
                }
                cout << ": ";
                cin >> input_options;

                switch (input_options) 
                {
                    case 1: 
                    {
                        std::vector<std::string> vBlockHashs;
                        size_t b_count {0};

                        for (size_t i = top; i > 0; --i) 
                        {
                            pRocksDb->GetBlockHashsByBlockHeight(txn, i, vBlockHashs);
                            b_count += vBlockHashs.size();
                            cout << "height------- " << i << endl;
                            cout << "count======= " << vBlockHashs.size() << endl;
                            vBlockHashs.clear();
                        }
                        cout << "b_count>>>>>>> " << b_count << endl;
                        break;
                    }
                    case 2: 
                    {
                        string input_s, input_e;
                        uint64_t start, end;

                        cout << "pleace input start: ";
                        cin >> input_s;
                        if (input_s == "a" || input_s == "pa") 
                        {
                            input_s = "0";
                            input_e = amount;
                        } 
                        else 
                        {
                            if (stoul(input_s) > stoul(amount)) 
                            {
                                cout << "input > amount" << endl;
                                break;
                            }
                            cout << "pleace input end: ";
                            cin >> input_e;
                            if (stoul(input_s) < 0 || stoul(input_e) < 0) 
                            {
                                cout << "params < 0!!" << endl;
                                break;
                            }
                            if (stoul(input_s) > stoul(input_e)) 
                            {
                                input_s = input_e;
                            }
                            if (stoul(input_e) > stoul(amount)) 
                            {
                                input_e = to_string(top);
                            }
                        }
                        start = stoul(input_s);
                        end = stoul(input_e);

                        printRocksdb(start, end);
                        break;
                    }
                case 3:
                {
                    uint64_t maxheight = top ;
                    uint64_t totalaward = 0;
                   
                    vector<string> addr;
                    for(unsigned int i = maxheight;  i >0; i--)
                    { 
                        vector<string> blockhashes;
                        pRocksDb->GetBlockHashsByBlockHeight(txn,top,blockhashes);
                        cout<<"blockhashes.size()="<<blockhashes.size()<<endl;
                        uint64_t SingleTotal = 0;
                        for(auto &blockhash: blockhashes )
                        {
                            string blockstr;
                            pRocksDb->GetBlockByBlockHash(txn, blockhash, blockstr);
                            CBlock Block;
                            Block.ParseFromString(blockstr);

                            for (int j = 0; j < Block.txs_size(); j++) 
                            {
                                CTransaction tx = Block.txs(j);
                                if(CheckTransactionType(tx) == kTransactionType_Award)
                                {
                                    for (int k = 0; k < tx.vout_size(); k++)
                                    {
                                        CTxout txout = tx.vout(k);
                                        SingleTotal += txout.value();
                                    } 
                                }

                            }        
                        }
                        totalaward += SingleTotal;  
                        cout<<"第"<<top<<"Block reward amount :" <<SingleTotal<<endl;
                        top--;
                    }
                    cout<<"All block rewards :"<<totalaward<<endl;
                    break;
                }
                case 4:
                {
                    uint64_t maxheight = top;
                    map<string,int64_t> addrvalue;
                    for(unsigned int i = maxheight;  i > 0; i--)
                    { 
                        vector<string> blockhashes;
                        pRocksDb->GetBlockHashsByBlockHeight(txn,top,blockhashes);
                        cout<<"blockhashes.size()="<<blockhashes.size()<<endl;
                        for(auto &blockhash: blockhashes )
                        {
                            string blockstr;
                            pRocksDb->GetBlockByBlockHash(txn, blockhash, blockstr);
                            CBlock Block;
                            Block.ParseFromString(blockstr);
                            for (int j = 0; j < Block.txs_size(); j++) 
                            {
                                CTransaction tx = Block.txs(j);
                                if(CheckTransactionType(tx) == kTransactionType_Award)
                                {
                                    for (int k = 0; k < tx.vout_size(); k++)
                                    {
                                        CTxout txout = tx.vout(k);
                                        if(addrvalue.empty())
                                        {
                                            addrvalue.insert(pair<string,int64_t>(txout.scriptpubkey(),txout.value()));
                                            cout<<"The first time you put the first "<<i <<" block of the map into the extra reward for signature" <<txout.scriptpubkey()<< "->"<<txout.value()<<endl;
                                            
                                        }
                                        else
                                        {
                                            map<string, int64_t>::iterator  iter;  
                                            int64_t newvalue = txout.value();
                                            iter = addrvalue.find(txout.scriptpubkey());
                                            if(iter != addrvalue.end())
                                            {
                                                int64_t value = iter->second;
                                                newvalue += value;
                                                addrvalue.erase(iter);
                                                addrvalue.insert(pair<string,int64_t>(txout.scriptpubkey(),newvalue));
                                                cout<<"When the address is the same"<<i <<"block signature bonus" <<txout.scriptpubkey()<< "->"<<newvalue<<endl; 
                                                
                                            }
                                            else
                                            {
                                                addrvalue.insert(pair<string,int64_t>(txout.scriptpubkey(),txout.value()));
                                                cout<<"When the address is different"<<i <<"block signature bonus" <<txout.scriptpubkey()<< "->"<<txout.value()<<endl;
                                               
                                            }
                                        }
                                    } 
                                      
                                }
                            }        
                        }
                        top--;
                    }
                    map<string, int64_t>::iterator  it;  
                    int fd = -1;
                    if (addrvalue.size() > 10) 
                    {
                        fd = open("Base_value.txt", O_CREAT | O_WRONLY);
                    }
                    CaTestFormatType formatType;
                    if (fd == -1) 
                    {
                        formatType = kCaTestFormatType_Print;
                    } 
                    else 
                    {
                        formatType = kCaTestFormatType_File;
                    }
                    for(it = addrvalue.begin(); it != addrvalue.end(); it++)  
                    {  
                        // cout<<it->first<<" -> "<<it->second<<endl; 
                        
                        blkprint(formatType, fd, "%s,%u\n", it->first.c_str(),it->second); 
                    } 
                    close(fd); 
                    break;
                }
                case 5:
                {
                    GetDevPasswordReq getDevPasswordReq;
                    getDevPasswordReq.set_version(getVersion());
                    std::string id;
                    cout<<"please enter id"<<endl;
                    cin >> id;
                    std::string paswd ;
                    cout<<"Please enter the password account "<<endl;
                    cin >> paswd;
                    getDevPasswordReq.set_password(paswd);
                    net_send_message<GetDevPasswordReq>(id, getDevPasswordReq);
                    break;
                }
                case 6:
                {
                    SetDevPasswordReq  setDevPasswordReq;
                    std::string id;
                    cout<<"please enter id"<<endl;
                    cin >> id;
                    std::string oldpaswd ;
                    cout<<"Please enter the old password account "<<endl;
                    cin >> oldpaswd;
                    std::string newpaswd ;
                    cout<<"Please enter a new password account "<<endl;
                    cin >> newpaswd;
                    setDevPasswordReq.set_version(getVersion());
                    setDevPasswordReq.set_old_pass(oldpaswd);
                    setDevPasswordReq.set_new_pass(newpaswd);
                    net_send_message<SetDevPasswordReq>(id, setDevPasswordReq);
                    break;
                }
                case 7:
                {
                    GetDevPrivateKeyReq  getDevPrivateKeyReq;
                    std::string id;
                    cout<<"please enterid"<<endl;
                    cin >> id;
                    std::string paswd ;
                    cout<<"Please enter the old password account "<<endl;
                    cin >> paswd;
                    std::string bs58addr; 
                    cout<<"Please enter bs58addr "<<endl;
                    cin >> bs58addr;
                    getDevPrivateKeyReq.set_version(getVersion());
                    getDevPrivateKeyReq.set_bs58addr(bs58addr);
                    getDevPrivateKeyReq.set_password(paswd);
                    net_send_message<GetDevPrivateKeyReq>(id, getDevPrivateKeyReq);
                    break;
                }
                default:
                        break;
                }

                break;
			}
			case 6:
			{
                std::cout << "1.Rollback block from Height" << std::endl;
                std::cout << "2.Rollback block from Hash" << std::endl;
                std::cout << "0.Quit" << std::endl;

                int iSwitch = 0;
                std::cin >> iSwitch;
                switch(iSwitch)
                {
                    case 0:
                    {
                        break;
                    }
                    case 1:
                    {
                        unsigned int height = 0;
                        std::cout << "Rollback block height: " ;
                        std::cin >> height;

                        MagicSingleton<Rollback>::GetInstance()->RollbackToHeight(height);
                        Singleton<PeerNode>::get_instance()->set_self_chain_height();
                        break;
                    }
                    case 2:
                    {
                        std::string rollbackHash;
                        std::cout << "Rollback block hash: ";
                        std::cin >> rollbackHash;

                        auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
                        Transaction* txn = pRocksDb->TransactionInit();
                        if( txn == NULL )
                        {
                            std::cout << "(AddBlock) TransactionInit failed !" << std::endl;
                            break;
                        }

                        bool bRollback = true;
                        ON_SCOPE_EXIT{
                            pRocksDb->TransactionDelete(txn, bRollback);
                        };

                        MagicSingleton<Rollback>::GetInstance()->RollbackBlockByBlockHash(txn, pRocksDb, rollbackHash);
                        break;
                    }
                    default:
                    {
                        std::cout << "Input error !" << std::endl;
                        break;
                    }
                }
				break;
			}
			case 7:
			{
                //interface_testdevice_mac();
                PrintOnLineTime();
				break;
			}
			case 8:
			{
				std::string str = printBlocks(100);
                std::cout << str << std::endl;
                ofstream file("blockdata.txt", fstream::out);
                str = printBlocks();
                file << str;
                file.close();                
				break;
			}
			case 9:
			{
				std::string select;
				std::cout << "1. Generate mnemonic words." << std::endl;
				std::cout << "2. Simulate pledged assets ." << std::endl;
				std::cout << "3. Get the fuel cost set by the machine and the maximum and minimum average fuel cost of the entire network node " << std::endl;
				std::cout << "4. Check the balance according to UTXO " << std::endl;
				std::cout << "5. Set node signature fee " << std::endl;
                std::cout << "8. Simulate depledged assets " << std::endl;
                std::cout << "9.Query the amount of pledged assets of an account " << std::endl;
                std::cout << "10.Multi-account transaction " << std::endl;
                std::cout << "11.Query transaction list " << std::endl;
                std::cout << "12.Check transaction details " << std::endl;
                std::cout << "13.Query block list " << std::endl;
                std::cout << "14.Query block details " << std::endl;
                std::cout << "15.Query all pledge addresses " << std::endl;
                std::cout << "16.Get the total number of blocks in 5 minutes " << std::endl;
                std::cout << "17.Set node packaging fee " << std::endl;
                std::cout << "18.Get all public network node packaging fees " << std::endl;
                std::cout << "19.Automatic out-of-order transaction (simplified version) " << std::endl;
                std::cout << "20.Test to get the first 100 pieces of rewards for each account " << std::endl;
                std::cout << "21.Obtain block information through transaction hash " << std::endl;
                std::cout << "22.Get the list of failed transactions " << std::endl;
                std::cout << "23.Get whether the height of the node meets the height range " << std::endl;


				std::cout << "Please select  ---> ";
				std::cin >> select ;
				int select_int = atoi(select.c_str());
				switch(select_int)
				{
					case 1:
					{
						char out[24*10] = {0};
						char keystore_data[2400] = {0};

						interface_GetMnemonic(nullptr, out, 24*10);
						printf("Mnemonic_data: [%s]\n", out);

						memset(keystore_data, 0x00, sizeof(keystore_data));
						interface_GetKeyStore(nullptr, "123456", keystore_data, 2400);
						printf("keystore_data %s\n", keystore_data);

						memset(keystore_data, 0x00, sizeof(keystore_data));
						interface_GetPrivateKeyHex(nullptr, keystore_data, 2400);
						printf("PrivateKey_data  %s\n", keystore_data);

						memset(keystore_data, 0x00, sizeof(keystore_data));
						interface_GetBase58Address(keystore_data, 2400);
						printf("PrivateKey_58Address  %s\n", keystore_data);

						break;
					}

					case 2:
					{
                        handle_pledge();
						break;
					}
					case 3:
					{
						break;
					}
					case 4:
					{
                        auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
                        Transaction* txn = pRocksDb->TransactionInit();

                        std::cout << "look for the address ：" ;
                        std::string addr;
                        std::cin >> addr;
                        std::vector<std::string> utxoHashs;
                        pRocksDb->GetUtxoHashsByAddress(txn, addr, utxoHashs);
                        uint64_t total = 0;
                        std::cout << "Account :" << addr << std::endl << "utxo:" << std::endl;
                        for (auto i : utxoHashs)
                        {
                            std::cout << i << std::endl;
                            std::string txRaw;
                            pRocksDb->GetTransactionByHash(txn, i, txRaw);

                            CTransaction tx;
                            tx.ParseFromString(txRaw);
                            for (int j = 0; j < tx.vout_size(); j++)
                            {
                                CTxout txout = tx.vout(j);
                                if (txout.scriptpubkey() == addr)
                                {
                                    total += txout.value();
                                }
                            }
                        }

                        std::cout << "UTXO Total value ：" << total << std::endl;
						break;
					}
                    case 5 :
                    {
                        uint64_t service_fee = 0, show_service_fee;
                        cout << "Enter the mining fee : ";
                        cin >> service_fee;

                        if(service_fee < 1000  ||  service_fee > 100000)
                        {
                            cout<<" The input mining fee is not in a reasonable range ！"<<endl;
                            break;
                        }

                        auto rdb_ptr = MagicSingleton<Rocksdb>::GetInstance();
                        auto status = rdb_ptr->SetDeviceSignatureFee(service_fee);
                        if (!status) 
                        {
                            net_update_fee_and_broadcast(service_fee);
                        }

                        rdb_ptr->GetDeviceSignatureFee(show_service_fee);
                        cout << "Mining fee ---------------------------------------------------------->>" << show_service_fee << endl;
                        break;
                    }
                    case 6 :
                    {
                        std::vector<std::string> test_list;
                        test_list.push_back("1. Get the highest block hash and height ");
                        test_list.push_back("2. Get mac ");

                        for (auto v : test_list) 
                        {
                            std::cout << v << std::endl;
                        }

                        std::string node_id;
                        std::cout << "Enter id : ";
                        std::cin >> node_id;

                        uint32_t test_num {0};
                        std::cout << "Enter option number : ";
                        std::cin >> test_num;

                        CaTestInfoReq ca_test_req;
                        ca_test_req.set_test_num(test_num);

                        net_send_message<CaTestInfoReq>(node_id, ca_test_req);
                    }
                    case 7:
                    {
                        const std::string addr = "16psRip78QvUruQr9fMzr8EomtFS1bVaXk";
                        CTransaction tx;

                        uint64_t time = Singleton<TimeUtil>::get_instance()->getlocalTimestamp();
                        tx.set_version(1);
                        tx.set_time(time);
                        tx.set_txowner(addr);
                        tx.set_n(0);

                        CTxin * txin = tx.add_vin();
                        CTxprevout * prevout = txin->mutable_prevout();
                        prevout->set_hash( std::string(64, '0') );
                        prevout->set_n(-1);

                        CScriptSig * scriptSig = txin->mutable_scriptsig();
                        scriptSig->set_sign(GENESIS_BASE_TX_SIGN);

                        txin->set_nsequence(-1);

                        CTxout * txout = tx.add_vout();
                        txout->set_value(120000000000000);
                        txout->set_scriptpubkey(addr);

                        CalcTransactionHash(tx);

                        CBlock cblock;
                        cblock.set_time(time);
                        cblock.set_version(1);
                        cblock.set_prevhash(std::string(64, '0'));
                        cblock.set_height(0);
                        CTransaction * tx0 = cblock.add_txs();
                        *tx0 = tx;
                        
                        COwnerAddr * ownerAddr = cblock.add_addresses();
                        ownerAddr->set_txowner("0000000000000000000000000000000000");
                        ownerAddr->set_n(0);

                        CalcBlockMerkle(cblock);

                        std::string serBlockHeader = cblock.SerializeAsString();
                        cblock.set_hash(getsha256hash(serBlockHeader));
                        
                        std::string blockHeaderStr = cblock.SerializeAsString();
                        char * hexstr = new char[blockHeaderStr.size() * 2 + 2]{0};
                        encode_hex(hexstr, blockHeaderStr.data(), blockHeaderStr.size());

                        std::cout << std::endl << hexstr << std::endl;

                        delete [] hexstr;

                        
                        break;
                    }
					case 8:
					{
                        handle_redeem_pledge();
						break;
					}
                    case 9:
                    {
                        std::cout << "Query the amount of pledged assets of an account ：" << std::endl;
                        { // Query pledge 

                            std::string addr;
                            std::cout << "Query account ：";
                            std::cin >> addr;

                            std::string txHash;
                            std::cout << "Enter hash (Enter directly for the first time) ：";
                            std::cin.get();
                            std::getline(std::cin, txHash);

                            size_t count = 0;
                            std::cout << "Enter count ：";
                            std::cin >> count;

                            std::shared_ptr<GetPledgeListReq> req = std::make_shared<GetPledgeListReq>();
                            req->set_version(getVersion());
                            req->set_addr(addr);
                            //req->set_index(0);
                            req->set_txhash(txHash);
                            req->set_count(count);

                            GetPledgeListAck ack;
                            m_api::HandleGetPledgeListReq(req, ack);


                            if (ack.code() != 0)
                            {
                                error("get pledge failed!");
                                std::cout << "get pledge failed!" << std::endl;
                                break;
                            }

                            std::cout << "pledge total: " << ack.total() << std::endl;
                            std::cout << "last hash: " << ack.lasthash() << std::endl;

                            size_t size = (size_t)ack.list_size();
                            for (size_t i = 0; i < size; i++)
                            {
                                std::cout << "----- pledge " << i << " start -----" << std::endl;
                                const PledgeItem item = ack.list(i);
                                std::cout << "blockhash: " << item.blockhash() << std::endl;
                                std::cout << "blockheight: " << item.blockheight() << std::endl;
                                std::cout << "utxo: " << item.utxo() << std::endl;
                                std::cout << "amount: " << item.amount() << std::endl;
                                std::cout << "time: " << PrintTime(item.time()) << std::endl;
                                std::cout << "fromaddr: " << item.fromaddr() << std::endl;
                                std::cout << "toaddr: " << item.toaddr() << std::endl;
                                std::cout << "detail: " << item.detail() << std::endl;

                                std::cout << "----- pledge " << i << " end -----" << std::endl;
                            }

                        }

                        break;
                    }
                    case 10:
                    {
                        std::vector<std::string> fromAddr;
                        std::map<std::string, int64_t> toAddr;
                        uint32_t needVerifyPreHashCount = 0;
                        double minerFees = 0;

                        int addrCount = 0;
                        std::cout << "Number of initiator accounts :";
                        std::cin >> addrCount;
                        for (int i = 0; i < addrCount; ++i)
                        {
                            std::string addr;
                            std::cout << "Initiate an account " << i + 1 << ": ";
                            std::cin >> addr;
                            fromAddr.push_back(addr);
                        }

                        std::cout << "Number of recipient accounts :";
                        std::cin >> addrCount;
                        for (int i = 0; i < addrCount; ++i)
                        {
                            std::string addr;
                            double amt = 0; 
                            std::cout << "Receiving account " << i + 1 << ": ";
                            std::cin >> addr;
                            std::cout << "Amount : ";
                            std::cin >> amt;
                            toAddr.insert(make_pair(addr, amt * DECIMAL_NUM));
                        }

                        std::cout << "Number of signatures :";
                        std::cin >> needVerifyPreHashCount;

                        std::cout << "Handling fee :";
                        std::cin >> minerFees;

                        TxHelper::DoCreateTx(fromAddr, toAddr, needVerifyPreHashCount, minerFees * DECIMAL_NUM);
                        break;
                    }
                    case 11:
                    {

                        { // Transaction list 

                            std::string addr;
                            std::cout << "Query account ：";
                            std::cin >> addr;

                            //size_t index = 0;
                            std::string txHash;
                            std::cout << "Enter hash (Enter directly for the first time) ：";
                            std::cin.get();
                            std::getline(std::cin, txHash);

                            size_t count = 0;
                            std::cout << "Enter count：";
                            std::cin >> count;

                            std::shared_ptr<GetTxInfoListReq> req = std::make_shared<GetTxInfoListReq>();
                            req->set_version(getVersion());
                            req->set_addr(addr);
                            //req->set_index(index);
                            req->set_txhash(txHash);
                            req->set_count(count);

                            GetTxInfoListAck ack;
                            m_api::HandleGetTxInfoListReq(req, ack);

                            if (ack.code() != 0)
                            {
                                std::cout << "code: " << ack.code() << std::endl;
                                std::cout << "Description: " << ack.description() << std::endl;
                                error("get pledge failed!");
                                return;
                            }

                            std::cout << "total:" << ack.total() << std::endl;
                            //std::cout << "index:" << ack.index() << std::endl;
                            std::cout << "last hash:" << ack.lasthash() << std::endl;

                            for (size_t i = 0; i < (size_t)ack.list_size(); i++)
                            {
                                std::cout << "----- txInfo " << i << " start -----" << std::endl;
                                const TxInfoItem item = ack.list(i);

                                std::cout << "txhash: " << item.txhash() << std::endl;
                                std::cout << "time: " << PrintTime(item.time()) << std::endl;
                                std::cout << "amount: " << item.amount() << std::endl;

                                TxInfoType type = item.type();
                                std::string strType;
                                if (type == TxInfoType_Originator)
                                {
                                    strType = "Transaction initiator ";
                                }
                                else if (type == TxInfoType_Receiver)
                                {
                                    strType = "Transaction recipient ";
                                }
                                else if (type == TxInfoType_Gas)
                                {
                                    strType = "Handling fee reward ";
                                }
                                else if (type == TxInfoType_Award)
                                {
                                    strType = "Block reward ";
                                }
                                else if (type == TxInfoType_Pledge)
                                {
                                    strType = "Pledge ";
                                }
                                else if (type == TxInfoType_Redeem)
                                {
                                    strType = "Release pledge ";
                                }
                                else if (type == TxInfoType_PledgedAndRedeemed)
                                {
                                    strType = "Pledge but released ";
                                }

                                std::cout << "type: " << strType << std::endl;
                                std::cout << "----- txInfo " << i << " end -----" << std::endl;
                            }

                        }
                        break;
                    }
                    case 12:
                    {
                        { // Query transaction 
                            std::string txHash;
                            std::cout << "Query transaction hash ：";
                            std::cin >> txHash;

                            std::string addr;
                            std::cout << "look for the address : " ;
                            std::cin >> addr;

                            std::shared_ptr<GetTxInfoDetailReq> req = std::make_shared<GetTxInfoDetailReq>();
                            req->set_version(getVersion());
                            req->set_txhash(txHash);
                            req->set_addr(addr);
                            
                            GetTxInfoDetailAck ack;
                            m_api::HandleGetTxInfoDetailReq(req, ack);

                            std::cout << "----- txInfo start -----" << std::endl;
                            std::cout << "    ----- tx -----" << std::endl;
                            std::cout << "    code: " << ack.code() << std::endl;
                            std::cout << "    blockhash: " << ack.blockhash() << std::endl;
                            std::cout << "    blockheight: " << ack.blockheight() << std::endl;
                            std::cout << "    txhash: " << ack.txhash() << std::endl;
                            std::cout << "    time: " << PrintTime(ack.time()) << std::endl;

                            for (auto from : ack.fromaddr())
                            {
                                std::cout << "    fromaddr: " << from << std::endl;    
                            }
                            
                            for (auto to : ack.toaddr())
                            {
                                std::cout << "    to: " << to.toaddr() << " amt: " << to.amt() << std::endl;
                            }
                            
                            std::cout << "    gas: " << ack.gas() << std::endl;
                            std::cout << "    amount: " << ack.amount() << std::endl;
                            std::cout << "    award: " << ack.award() << std::endl;
                            
                            std::cout << "    ----- award -----" << std::endl;
                            std::cout << "awardGas: " << ack.awardgas() << std::endl;
                            std::cout << "awardAmount: " << ack.awardamount() << std::endl;
                            
                            
                            std::cout << "----- txInfo end -----" << std::endl;
                        }
                        break;
                    }
                    case 13:
                    {
                        size_t index = 0;
                        std::cout << "Query index ：";
                        std::cin >> index;

                        size_t count = 0;
                        std::cout << "Query count：" ;
                        std::cin >> count;
                        
                        GetBlockInfoListReq req;
                        req.set_version(getVersion());
                        req.set_index(index);
                        req.set_count(count);

                        std::cout << " --List of nearby node ids  -- " << std::endl;
                        std::vector<Node> idInfos = Singleton<NodeCache>::get_instance()->get_nodelist();
                        std::vector<std::string> ids;
                        for (const auto & idInfo : idInfos)
                        {
                            ids.push_back(idInfo.id);
                        }

                        for (auto & id : ids)
                        {
                            std::cout << id << std::endl;
                        }

                        std::string id;
                        std::cout << "Query ID ：";
                        std::cin >> id;

                        net_register_callback<GetBlockInfoListAck>([] (const std::shared_ptr<GetBlockInfoListAck> & ack, const MsgData & msgdata){
                            
                            std::cout << std::endl;
                            std::cout << "top: " << ack->top() << std::endl;
                            std::cout << "txcount: " << ack->txcount() << std::endl;
                            std::cout << std::endl;

                            for(size_t i = 0; i != (size_t)ack->list_size(); ++i)
                            {
                                std::cout << " ---- block list " << i << " start ---- " << std::endl;
                                const BlockInfoItem & item = ack->list(i);
                                std::cout << "blockhash: " << item.blockhash() << std::endl;
                                std::cout << "blockheight: " << item.blockheight() << std::endl;
                                std::cout << "time: " << PrintTime(item.time()) << std::endl;
                                std::cout << "txHash: " << item.txhash() << std::endl;

                                for (auto from : item.fromaddr())
                                {
                                    std::cout << "fromaddr: " << from << std::endl;    
                                }
                                
                                for (auto to : item.toaddr())
                                {
                                    std::cout << "    to: " << to << std::endl;
                                }
                                
                                std::cout << "amount: " << item.amount() << std::endl;


                                std::cout << " ---- block list " << i << " end ---- " << std::endl;
                            }

                            std::cout << "End of query " << std::endl;
                        });

                        net_send_message<GetBlockInfoListReq>(id, req, net_com::Priority::kPriority_Middle_1);

                        break;
                    }
                    case 14:
                    {
                        std::string hash;
                        std::cout << "Inquire  block hash：" ;
                        std::cin >> hash;

                        GetBlockInfoDetailReq req;
                        req.set_version(getVersion());
                        req.set_blockhash(hash);

                        std::cout << " -- List of nearby node ids  -- " << std::endl;
                        std::vector<Node> idInfos = Singleton<NodeCache>::get_instance()->get_nodelist();
                        std::vector<std::string> ids;
                        for (const auto & idInfo : idInfos)
                        {
                            ids.push_back(idInfo.id);
                        }
                        for (auto & id : ids)
                        {
                            std::cout << id << std::endl;
                        }

                        std::string id;
                        std::cout << "查询ID：";
                        std::cin >> id;

                        net_register_callback<GetBlockInfoDetailAck>([] (const std::shared_ptr<GetBlockInfoDetailAck> & ack, const MsgData &msgdata){
                            
                            std::cout << std::endl;

                            std::cout << " ---- block info start " << " ---- " << std::endl;

                            std::cout << "blockhash: " << ack->blockhash() << std::endl;
                            std::cout << "blockheight: " << ack->blockheight() << std::endl;
                            std::cout << "merkleRoot: " << ack->merkleroot() << std::endl;
                            std::cout << "prevHash: " << ack->prevhash() << std::endl;
                            std::cout << "time: " << PrintTime(ack->time()) << std::endl;
                            std::cout << "tatalAmount: " << ack->tatalamount() << std::endl;

                            for (auto & s : ack->signer())
                            {
                                std::cout << "signer: " << s << std::endl;
                            }

                            for (auto & i : ack->blockinfooutaddr())
                            {
                                std::cout << "addr: " << i.addr() << " amount: " << i.amount() << std::endl;
                            }

                            std::cout << " ---- block info end " << " ---- " << std::endl;

                            std::cout << "End of query " << std::endl;
                        });

                        net_send_message<GetBlockInfoDetailReq>(id, req);
                        break;
                    }
                    case 15:
                    {
                        auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
                        Transaction* txn = pRocksDb->TransactionInit();

                        ON_SCOPE_EXIT{
                            pRocksDb->TransactionDelete(txn, true);
                        };

                        std::vector<std::string> addressVec;
                        pRocksDb->GetPledgeAddress(txn, addressVec);

                        std::cout << std::endl << "---- Pledged address  start ----" << std::endl;
                        for (auto & addr : addressVec)
                        {
                            uint64_t pledgeamount = 0;
                            SearchPledge(addr, pledgeamount);
                            std::cout << addr << " : " << pledgeamount << std::endl;
                        }
                        std::cout << "---- Number of staked addresses :" << addressVec.size() << " ----" << std::endl << std::endl;

                        std::cout << "---- Pledged address  end  ----" << std::endl << std::endl;

                        break;
                    }
                    case 16:
                    {
                        std::cout << "Get the total number of blocks in 5 minutes " << std::endl;
                        a_award::AwardAlgorithm awardAlgorithm;

                        uint64_t blockSum = 0;
                        awardAlgorithm.GetBlockNumInUnitTime(blockSum);
                        std::cout << "Total number of blocks ：" << blockSum << std::endl;
                        break;
                    }
                    case 17:
                    {
                        uint64_t packageFee = 0;
                        get_device_package_fee(packageFee);

                        std::cout << "Current packaging costs : " << packageFee << std::endl;
                        std::cout << "Set packaging fee (must be a positive integer) : ";
                        std::string strPackageFee;
                        std::cin >> strPackageFee;
                        int newPackageFee = 0;
                        try
                        {
                            newPackageFee = std::stoi(strPackageFee);
                        }
                        catch (...)
                        {
                            newPackageFee = -1;
                        }
                        if (newPackageFee < 0 || newPackageFee > 100 * DECIMAL_NUM)
                        {
                            std::cout << "Please enter the packing fee within the correct range (0-100000000)" << std::endl;
                            break ;
                        }
                        
                        int result = set_device_package_fee(newPackageFee);
                        if (result == 0)
                        {
                            std::cout << "Packing fee is set successfully  " << newPackageFee << std::endl;
                        }
                        else
                        {
                            std::cout << "Failed to set package fee " << std::endl;
                        }

                        break;
                    }
                    case 18:
                    {
                        std::cout << " -- List of nearby node ids  -- " << std::endl;
                        std::vector<Node> idInfos = Singleton<NodeCache>::get_instance()->get_nodelist();
                        std::vector<std::string> ids;
                        for (const auto & i : idInfos)
                        {
                            ids.push_back(i.id);
                        }
                        for (auto & id : ids)
                        {
                            std::cout << id << std::endl;
                        }

                        std::string id;
                        std::cout << "Query ID ：";
                        std::cin >> id;


                        net_register_callback<GetNodeInfoAck>([] (const std::shared_ptr<GetNodeInfoAck> & ack, const MsgData &msgdata){

                            std::cout << "Show public network nodes " << std::endl;
                            for (auto & node : ack->node_list())
                            {
                                std::cout << " -- local " << node.local() << " -- " << std::endl;
                                for (auto & info : node.node_info())
                                {
                                    std::cout << "ip:" << info.ip() << std::endl;
                                    std::cout << "name:" << info.name() << std::endl;
                                    std::cout << "port:" << info.port() << std::endl;
                                    std::cout << "price:" << info.price() << std::endl;
                                }

                                std::cout << std::endl;
                            }
                        });

                        GetNodeInfoReq req;
                        req.set_version(getVersion());
                        net_send_message<GetNodeInfoReq>(id, req);
                        break;
                    }
                    case 19:
                    {
                        // int addrNum = 0;
                        // std::cout << "Number of accounts ：";
                        // std::cin >> addrNum;

                        // std::vector<std::string> addrs;
                        // for (int i = 0; i < addrNum; ++i)
                        // {
                        //     std::string tmpAddr;
                        //     std::cout << "account number " << i << ": ";
                        //     std::cin >> tmpAddr;
                        //     addrs.push_back(tmpAddr);
                        // }

                        // std::string signFee;
                        // std::cout << "Handling fee ：";
                        // std::cin >> signFee;

                        // int sleepTime = 0;
                        // std::cout << "Interval time (seconds) ：";
                        // std::cin >> sleepTime;

                        // while(1)
                        // {
                        //     srand(time(NULL));
                        //     int intPart = rand() % 10;
                        //     double decPart = (double)(rand() % 100) / 100;
                        //     double amount = intPart + decPart;
                        //     std::string amountStr = std::to_string(amount);

                        //     std::cout << std::endl << std::endl << std::endl << "=======================================================================" << std::endl;
                        //     CreateTx(addrs[0].c_str(), addrs[1].c_str(), amountStr.c_str(), NULL, 6, signFee);
                        //     std::cout << "=====Transaction initiator ：" << addrs[0] << std::endl;
                        //     std::cout << "=====Transaction recipient ：" << addrs[1] << std::endl;
                        //     std::cout << "=====Transaction amount   ：" << amountStr << std::endl;
                        //     std::cout << "=======================================================================" << std::endl << std::endl << std::endl << std::endl;

                        //     sleep(sleepTime);

                        //     std::cout << std::endl << std::endl << std::endl << "=======================================================================" << std::endl;
                        //     CreateTx(addrs[1].c_str(), addrs[0].c_str(), amountStr.c_str(), NULL, 6, signFee);
                        //     std::cout << "=====Transaction initiator ：" << addrs[1] << std::endl;
                        //     std::cout << "=====Transaction recipient ：" << addrs[0] << std::endl;
                        //     std::cout << "=====Transaction amount   ：" << amountStr << std::endl;
                        //     std::cout << "=======================================================================" << std::endl << std::endl << std::endl << std::endl;
                        //     sleep(sleepTime);
                        // }
                        if (bIsCreateTx)
                        {
                            int i = 0;
                            std::cout << "1. End the transaction " << std::endl;
                            std::cout << "0. Continue trading " << std::endl;
                            std::cout << ">>>" << std::endl;
                            std::cin >> i;
                            if (i == 1)
                            {
                                bStopTx = true;
                            }
                            else if (i == 0)
                            {
                                break;
                            }
                            else
                            {
                                std::cout << "Error!" << std::endl;
                                break;
                            }
                        }
                        else
                        {
                            bStopTx = false;
                            // int sleepTime = 0;
                            // std::cout << "Interval time (seconds) ：";
                            // std::cin >> sleepTime;
                            int addrNum = 0;
                            std::cout << "Number of accounts ：";
                            std::cin >> addrNum;

                            std::vector<std::string> addrs;
                            for (int i = 0; i < addrNum; ++i)
                            {
                                std::string tmpAddr;
                                std::cout << "account number " << i << ": ";
                                std::cin >> tmpAddr;
                                addrs.push_back(tmpAddr);
                            }

                            std::string signFee;
                            std::cout << "Handling fee ：";
                            std::cin >> signFee;

                            int sleepTime = 0;
                            std::cout << "Interval time (seconds) ：";
                            std::cin >> sleepTime;

                            std::thread th(TestCreateTx, addrs, sleepTime, signFee);
                            th.detach();
                            break;
                        }
                        break;
                    }
                    case 20: 
                    {
                        int blockNum = 1000;

                        // Start time 
                        clock_t start = clock();
                        // Get the average income of each account in the first 500 blocks 
                        auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
                        Transaction* txn = pRocksDb->TransactionInit();
                        if (txn == NULL)
                        {
                            error("(GetAward) TransactionInit failed !");
                            break;
                        }

                        ON_SCOPE_EXIT{
                            pRocksDb->TransactionDelete(txn, true);
                        };

                        unsigned int top = 0;
                        if ( 0 != pRocksDb->GetBlockTop(txn, top) )
                        {
                            std::cout << "GetBlockTop failed! " << std::endl;
                            break;
                        }

                        // Deposit account and top 500 total rewards 
                        std::map<std::string, int64_t> addrAwards;
                        std::map<std::string, uint64_t> addrSignNum;  // Stored account number and total signatures of the top 500 height 

                        uint64_t minHeight = (int)top > blockNum ? (int)top - blockNum : 0;  
                        for ( ; top != minHeight; --top)
                        {
                            std::vector<std::string> blockHashs;
                            if ( 0 != pRocksDb->GetBlockHashsByBlockHeight(txn, top, blockHashs) )
                            {
                                std::cout << "GetBlockHashsByBlockHeight failed! " << std::endl;
                                break;
                            }

                            for (auto & hash : blockHashs)
                            {
                                std::string blockStr;
                                if (0 != pRocksDb->GetBlockByBlockHash(txn, hash, blockStr))
                                {
                                    std::cout << "GetBlockByBlockHash failed! " << std::endl;
                                    break;
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
                        clock_t end = clock();

                        for (auto & addrAward : addrAwards)
                        {
                            std::cout << "account number ：" << addrAward.first << "   reward ：" << addrAward.second << std::endl;
                        }

                        std::cout << std::endl;

                        for (auto & item : addrSignNum)
                        {
                            std::cout << "account number ：" << item.first << "   reward ：" << item.second << std::endl;
                        }

                        if (addrAwards.size() == 0 || addrSignNum.size() == 0)
                        {
                            break;
                        }

                        std::vector<uint64_t> awards;  // Store all reward values 
                        std::vector<uint64_t> vecSignNum;  //Store all reward values 

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

                        uint64_t quarterNum = awards.size() * 0.25;
                        uint64_t threeQuarterNum = awards.size() * 0.75;

                        uint64_t signNumQuarterNum = vecSignNum.size() * 0.25;
                        uint64_t signNumThreeQuarterNum = vecSignNum.size() * 0.75;

                        if (quarterNum == threeQuarterNum || signNumQuarterNum == signNumThreeQuarterNum)
                        {
                            break;
                        }
                        
                        uint64_t quarterValue = awards[quarterNum];
                        uint64_t threeQuarterValue = awards[threeQuarterNum];

                        uint64_t signNumQuarterValue = vecSignNum[signNumQuarterNum];
                        uint64_t signNumThreeQuarterValue = vecSignNum[signNumThreeQuarterNum];

                        uint64_t diffValue = threeQuarterValue - quarterValue;
                        uint64_t upperLimitValue = threeQuarterValue + (diffValue * 1.5);

                        uint64_t signNumDiffValue = signNumThreeQuarterValue - signNumQuarterValue;
                        uint64_t signNumUpperLimitValue = signNumThreeQuarterValue + (signNumDiffValue * 1.5);

                        uint64_t abnormalCount = 0;
                        uint64_t signNumAbnormalCount = 0;

                        std::vector<std::string> awardList;
                        std::vector<std::string> signNumList;
                        for (auto & addrAward : addrAwards)
                        {
                            if (addrAward.second > (int64_t)upperLimitValue)
                            {
                                abnormalCount++;
                                awardList.push_back(addrAward.first);
                                std::cout << "********** Account with abnormal reward total value ：" << addrAward.first << "  Total reward  = " << addrAward.second << std::endl;
                            }
                        }

                        for (auto & addrSign : addrSignNum)
                        {
                            if (addrSign.second > signNumUpperLimitValue)
                            {
                                signNumAbnormalCount++;
                                signNumList.push_back(addrSign.first);
                                std::cout << "********** Account with abnormal number of signatures ：" << addrSign.first << "   Total times  = " << addrSign.second << std::endl;
                            }
                        }

                        std::vector<std::string> addrList;
                        set_union(awardList.begin(), awardList.end(), signNumList.begin(), signNumList.end(), std::back_inserter(addrList));


                        std::cout << "Total time ：" << ((double)end - start) / CLOCKS_PER_SEC << "秒" <<std::endl;
                        std::cout << "********** Total reward value---1/4  = " << quarterValue << std::endl;
                        std::cout << "********** Total reward value---3/4  = " << threeQuarterValue << std::endl;
                        std::cout << "********** Total reward value --- difference  = " << diffValue << std::endl;
                        std::cout << "********** Total reward value---upper limit  = " << upperLimitValue << std::endl;
                        std::cout << "********** Total reward value --- total number of accounts  = " << addrAwards.size() << std::endl;
                        std::cout << "********** Total reward value---the number of abnormal accounts  = " << abnormalCount << std::endl;
                        std::cout << "********** Total reward value---proportion of abnormal accounts  = " << ( (double)abnormalCount / addrAwards.size() ) * 100 << "%" << std::endl;

                        std::cout << "********** Total reward value---1/4 = " << signNumQuarterValue << std::endl;
                        std::cout << "********** Total reward value---3/4 = " << signNumThreeQuarterValue << std::endl;
                        std::cout << "********** Total reward value --- difference = " << signNumDiffValue << std::endl;
                        std::cout << "********** Total reward value---upper limit= " << signNumUpperLimitValue << std::endl;
                        std::cout << "********** Total reward value --- total number of accounts = " << addrAwards.size() << std::endl;
                        std::cout << "********** Number of signatures---number of abnormal accounts  = " << signNumAbnormalCount << std::endl;
                        std::cout << "********** Total reward value---proportion of abnormal accounts  = " << ( (double)signNumAbnormalCount / addrAwards.size() ) * 100 << "%" << std::endl;

                        std::cout << std::endl << "********** Total number of abnormal accounts = " << addrList.size() << std::endl;
                        std::cout << "********** Proportion of signatures  = " << (double)signNumAbnormalCount / addrList.size() * 100 << "%" << std::endl;
                        std::cout << "**********Proportion of total reward value  = " << (double)abnormalCount / addrList.size() * 100 << "%" << std::endl;
                        break;
                    }
                    case 21:
                    {
                        auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
                        Transaction* txn = pRocksDb->TransactionInit();
                        if (txn == NULL)
                        {
                            error("(GetAward) TransactionInit failed !");
                            break;
                        }

                        ON_SCOPE_EXIT{
                            pRocksDb->TransactionDelete(txn, true);
                        };

                        std::cout << "Tx Hash : ";
                        std::string txHash;
                        std::cin >> txHash;

                        std::string blockHash;
                        pRocksDb->GetBlockHashByTransactionHash(txn, txHash, blockHash);

                        if (blockHash.empty())
                        {
                            std::cout << "Error : GetBlockHashByTransactionHash failed !" << std::endl;
                            break;
                        }

                        std::string blockStr;
                        pRocksDb->GetBlockByBlockHash(txn, blockHash, blockStr);
                        CBlock block;
                        block.ParseFromString(blockStr);

                        std::cout << "Block Hash : " << blockHash << std::endl;
                        std::cout << "Block height : " << block.height() << std::endl;

                        break;
                    }
                    case 22:
                    {
                        std::cout << "Query the list of failed transactions ：" << std::endl;

                        std::string addr;
                        std::cout << "Query account ：";
                        std::cin >> addr;

                        std::string txHash;
                        std::cout << "Enter hash (Enter directly for the first time) ：";
                        std::cin.get();
                        std::getline(std::cin, txHash);

                        size_t count = 0;
                        std::cout << "Enter count ：";
                        std::cin >> count;

                        std::shared_ptr<GetTxFailureListReq> req = std::make_shared<GetTxFailureListReq>();
                        req->set_version(getVersion());
                        req->set_addr(addr);
                        req->set_txhash(txHash);
                        req->set_count(count);

                        GetTxFailureListAck ack;
                        HandleGetTxFailureListReq(req, ack);
                        if (ack.code() != 0)
                        {
                            error("get transaction failure failed!");
                            std::cout << "get transaction failure failed!!" << std::endl;
                            std::cout << "code: " << ack.code() << std::endl;
                            std::cout << "description: " << ack.description() << std::endl;
                            break;
                        }

                        std::cout << "failure total: " << ack.total() << std::endl;
                        std::cout << "last hash: " << ack.lasthash() << std::endl;
                        size_t size = (size_t)ack.list_size();
                        for (size_t i = 0; i < size; i++)
                        {
                            std::cout << "----- failure list " << i << " start -----" << std::endl;
                            const TxFailureItem& item = ack.list(i);
                            std::cout << "hash: " << item.txhash() << std::endl;
                            if (item.fromaddr_size() > 0)
                            {
                                std::cout << "from addr: " << item.fromaddr(0) << std::endl;
                            }
                            if (item.toaddr_size() > 0)
                            {
                                std::cout << "to addr: " << item.toaddr(0) << std::endl;
                            }
                            std::cout << "amout: " << item.amount() << std::endl;
                        }

                        break;
                    }
                    case 23:
                    {
                        std::cout << " -- List of nearby node ids  -- " << std::endl;
                        std::vector<Node> idInfos = Singleton<NodeCache>::get_instance()->get_nodelist();
                        std::vector<std::string> ids;
                        for (const auto & idInfo : idInfos)
                        {
                            ids.push_back(idInfo.id);
                        }

                        for (auto & id : ids)
                        {
                            std::cout << id << std::endl;
                        }

                        std::string id;
                        std::cout << "Query ID ：";
                        std::cin >> id;

                        CheckNodeHeightReq req;
                        req.set_version(getVersion());


                        net_register_callback<CheckNodeHeightAck>([] (const std::shared_ptr<CheckNodeHeightAck> & ack, const MsgData & msgdata){
                            
                            std::cout << std::endl;
                            std::cout << "code: " << ack->code() << std::endl;
                            std::cout << "total: " << ack->total() << std::endl;
                            std::cout << "percent: " << ack->percent() << std::endl;
                            std::cout << "height: " << ack->height() << std::endl;
                            std::cout << std::endl;
                            
                            std::cout << "End of query " << std::endl;
                        });

                        net_send_message<CheckNodeHeightReq>(id, req, net_com::Priority::kPriority_Middle_1);
                        
                        break;
                    }
                    case 111:
                    {
                        std::string fileName("account.txt");

                        ca_console redColor(kConsoleColor_Red, kConsoleColor_Black, true); 
                        std::cout << redColor.color() << "The output file is ：" << fileName << " Please use Courier New font to view " << redColor.reset() <<  std::endl;

                        ofstream file;
                        file.open(fileName);

                        file << "Please use Courier New font to view " << std::endl << std::endl;

                        for (auto & item : g_AccountInfo.AccountList)
                        {
                            file << "Base58 addr: " << item.first << std::endl;

                            char out_data[1024] = {0};
                            int data_len = sizeof(out_data);
                            mnemonic_from_data((const uint8_t*)item.second.sPriStr.c_str(), item.second.sPriStr.size(), out_data, data_len);    
                            file << "Mnemonic: " << out_data << std::endl;

                            std::string strPriHex = Str2Hex(item.second.sPriStr);
                            file << "Private key: " << strPriHex << std::endl;
                            
                            file << "QRCode:";

                            QRCode qrcode;
                            uint8_t qrcodeData[qrcode_getBufferSize(5)];
                            qrcode_initText(&qrcode, qrcodeData, 5, ECC_MEDIUM, strPriHex.c_str());

                            file << std::endl << std::endl;

                            for (uint8_t y = 0; y < qrcode.size; y++) 
                            {
                                file << "        ";
                                for (uint8_t x = 0; x < qrcode.size; x++) 
                                {
                                    file << (qrcode_getModule(&qrcode, x, y) ? "\u2588\u2588" : "  ");
                                }

                                file << std::endl;
                            }

                            file << std::endl << std::endl << std::endl << std::endl << std::endl << std::endl;
                        }

                        break;
                    }
                    case 222:
                    {
                        auto nodelist = Singleton<NodeCache>::get_instance()->get_nodelist();
                        if (nodelist.size() == 0)
                        {
                            std::cout << "Node cache is empty!" << std::endl;
                        }
                        for (const auto& node : nodelist)
                        {
                            std::cout << "id (" << node.id << ")    height (" << node.chain_height << ")" << std::endl;
                        }
                        break;
                    }
					default:
					{
						break;
					}
				}
				break;
			}
			case 10:
			{
				int num;
				std::cout << "1.exit node" << std::endl;
				std::cout << "2.get a node info" << std::endl;
				std::cout << "3.get all node info" << std::endl;
				std::cout << "Please Select ... -> ";
				std::cin >> num;
				if(num < 1 || num > 3)
				{
					break;
				}


				switch(num)
				{
					case 1:
					{
						break;	
					}

					case 2:
					{
                        string id;
                        cout << "Please input id of the node:" ;
                        cin >> id;
						
                        SendDevInfoReq(id);
						std::cout << "Data are being obtained..." << std::endl;
						int i = 0;
						while(i < 10)
						{
							if(g_nodeinfo.size() <= 0)
							{
								sleep(1);
								i++;
							}
							else
							{
								break;
							}
						}

						if(i == 10)
						{
							std::cout << "get node info failed !" << std::endl;
							break;
						}

                        g_NodeInfoLock.lock_shared();
						for(auto i : g_nodeinfo)
                        {
                            std::cout << "node id   :" << i.id() << std::endl;
                            std::cout << "height    :" << i.height() <<std::endl;
                            std::cout << "hash      :" << i.hash() <<std::endl;
                            std::cout << "base58addr:" << i.base58addr() <<std::endl;
                        }
                        g_NodeInfoLock.unlock_shared();
                        
						g_nodeinfo.clear();
						break;
					}
                    case 3:
                    {
                        std::cout << "Data are being obtained..." << std::endl;

                        std::vector<Node> nodeInfos = Singleton<NodeCache>::get_instance()->get_nodelist();
                        std::vector<std::string> nodes;
                        for (const auto & i : nodeInfos )
                        {
                            nodes.push_back(i.id);
                        }
                        if(nodes.size() <= 0)
                        {
                            std::cout << "Node not found ！" << std::endl;
                            break;
                        }

						std::ofstream fileStream;
						fileStream.open("node.txt");
						if(!fileStream)
						{
							std::cout << "fail to open the file ！" << std::endl;
							break;
						}

						for(auto i : nodes)
						{
							// Get block information of all nodes 
							SendDevInfoReq(i);
						}

                        for(int j = 0; j < 10; j++)
						{
							if( g_nodeinfo.size() < nodes.size() )
							{
								std::cout << "Gotten ：" << g_nodeinfo.size() << std::endl;
								sleep(1);
							}
							else
							{
								break;
							}
						}

                        std::cout << "Total number of nodes  ：" << nodes.size() << std::endl;
						std::cout << "Gotten  ：" << g_nodeinfo.size() << std::endl;
						std::cout << "Start writing file ！" << std::endl;

                        g_NodeInfoLock.lock_shared();
                        for(auto k : g_nodeinfo)
						{
                            std::cout << "id ：" << k.id() << std::endl;
                            std::cout << "height ：" << k.height() << std::endl;
                            std::cout << "hash：" << k.hash() << std::endl;
                            std::cout << "addr：" << k.base58addr() << std::endl << std::endl;

							// Write all nodes to the node.txt file 
							fileStream << "ID：";
							fileStream << k.id();
							fileStream << ",height ：";
							fileStream << k.height();
							fileStream << ",hash：";
							fileStream << k.hash();
							fileStream << ",base58addr：";
							fileStream << k.base58addr();
							fileStream << "\n";
						}
                        g_NodeInfoLock.unlock_shared();

                        std::cout << "the end ！" << std::endl;
						g_nodeinfo.clear();

						fileStream.close();

                        break;
                    }

					default:
						break;
				}
                break;		
			}
            case 11:
			{
                string id;
                cout << "please input id:";
                cin >> id;
                g_synch->verifying_node.id = id;
                int sync_cnt = Singleton<Config>::get_instance()->GetSyncDataCount();
                g_synch->DataSynch(id, sync_cnt);
                break;
            }
            case 12:
            {
                MagicSingleton<TxVinCache>::GetInstance()->Print();
                break;
            }
            case 13:
            {
                MagicSingleton<TxFailureCache>::GetInstance()->Print();
                break;
            }
            case 14:
            {
                MagicSingleton<TxVinCache>::GetInstance()->Clear();
                break;
            }
            default:
                break;
        }
    }
}

const char * ca_version()
{
    return kVersion;
}

void QuitInput(char *x) 
{
    while (*x != 'q') 
    {
        cin >> *x;
    }
    return;
}

void AutoTranscation()
{
    vector<string> accounts;
    for (auto iter = g_AccountInfo.AccountList.begin(); iter != g_AccountInfo.AccountList.end(); ++iter)
    {
        string accountName = iter->first;
        uint64_t amount = CheckBalanceFromRocksDb(accountName.c_str());
        if (amount < (1 * DECIMAL_NUM))
        {
            error("Account balance is less than 1");
            cout << "Account balance is less than 1." << endl;
            continue ;
        }
        accounts.push_back(accountName);
    }
    if (accounts.size() < 2)
    {
        cout << "The Account number is less than 2." << endl;
        return ;
    }

    int sleepTime;
    cout << "Enter sleep for interval of transaction(second, > 30): ";
    cin >> sleepTime;
    cout << endl;
    sleepTime = std::max(sleepTime, 30);
    sleepTime *= 1000000;
    cout << "Sleep time:" << sleepTime << endl;

    char sign = 's';
    thread inputThread(QuitInput, &sign);

    std::default_random_engine generator(time(0));
    std::uniform_real_distribution<double> distribution(0.001, 1.0);
    int i = 0;

    while (true)
    {
        if (sign == 'q')
        {
            cout << "Exit for automation transaction." << endl;
            break;
        }

        int from = i % accounts.size();
        int to = (i + 1) % accounts.size();
        i = (i + 1) % accounts.size();

        double amount = distribution(generator);
        std::ostringstream stream;
        stream << std::setprecision(3) << amount;
        string strAmount = stream.str();
        //CreateTx(account_list[a1].c_str(), account_list[a2].c_str(), "1", NULL, g_MinNeedVerifyPreHashCount, d_gas);
        string minerGas("0.001");
        CreateTx(accounts[from].c_str(), accounts[to].c_str(), strAmount.c_str(), NULL, g_MinNeedVerifyPreHashCount, minerGas);
        cout << accounts[from] << " to " << accounts[to] << " value " << strAmount << endl;

        usleep(sleepTime);
    }

    inputThread.join();
}

// Get the height and hash of the highest block of a node 
void SendDevInfoReq(const std::string id)
{
    GetDevInfoReq getDevInfoReq;
    getDevInfoReq.set_version( getVersion() );

    std::string ownID = net_get_self_node_id();
    getDevInfoReq.set_id( ownID );

    net_send_message<GetDevInfoReq>(id, getDevInfoReq);
}

void HandleGetDevInfoReq( const std::shared_ptr<GetDevInfoReq>& msg, const MsgData& msgdata )
{
    debug("recv HandleGetDevInfoReq!");

    // Determine whether the version is compatible 
	if( 0 != Util::IsVersionCompatible( msg->version() ) )
	{
		error("HandleExitNode IsVersionCompatible");
		return ;
	}

    // version 
    GetDevInfoAck getDevInfoAck;
    getDevInfoAck.set_version( getVersion() );

    std::string ownID = net_get_self_node_id();
    getDevInfoAck.set_id( ownID );

    // Get a transaction for reading the database 
    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if( txn == NULL )
	{
		error("HandleGetDevInfoReq TransactionInit failed !");
		return ;
	}

    // Exit domain destruction transaction 
    ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, true);
	};

    // height 
    unsigned int blockHeight = 0;
    auto status = pRocksDb->GetBlockTop(txn, blockHeight);
    if( status != 0)
    {
		error("HandleGetDevInfoReq GetBlockTop failed !");
        return ;
    }
    getDevInfoAck.set_height( blockHeight );

    // bestchain
    std::string blockHash;
    status = pRocksDb->GetBestChainHash(txn, blockHash);
    if( status != 0)
    {
		error("HandleGetDevInfoReq GetBestChainHash failed !");
        return ;
    }
    getDevInfoAck.set_hash( blockHash );

    // base58 address 
    getDevInfoAck.set_base58addr( g_AccountInfo.DefaultKeyBs58Addr );

    net_send_message<GetDevInfoAck>(msg->id(), getDevInfoAck);
}

void HandleGetDevInfoAck( const std::shared_ptr<GetDevInfoAck>& msg, const MsgData& msgdata )
{
    // Version judgment 
    if( 0 != Util::IsVersionCompatible( msg->version() ) )
	{
		error("HandleExitNode IsVersionCompatible");
		return ;
	}

    g_NodeInfoLock.lock();
    g_nodeinfo.push_back( *msg );
    g_NodeInfoLock.unlock();
}

void ca_print_basic_info()
{
    std::string version = getVersion();
	std::string base58 = g_AccountInfo.DefaultKeyBs58Addr;

    uint64_t balance = CheckBalanceFromRocksDb(base58);

	std::string macMd5 = net_data::get_mac_md5();

    uint64_t minerFee = 0;
    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
    Transaction* txn = pRocksDb->TransactionInit();
    ON_SCOPE_EXIT{
        pRocksDb->TransactionDelete(txn, true);
    };
    pRocksDb->GetDeviceSignatureFee(minerFee);

    uint64_t packageFee = 0;
    pRocksDb->GetDevicePackageFee(packageFee);

    unsigned int blockHeight = 0;
    pRocksDb->GetBlockTop(txn, blockHeight);

    std::string bestChainHash;
    pRocksDb->GetBestChainHash(txn, bestChainHash);

    ca_console infoColor(kConsoleColor_Green, kConsoleColor_Black, true);
    cout << infoColor.color();
	cout << "********************************************************" << endl;
	cout << "Version: " << version << endl;
	cout << "Base58: " << base58 << endl;
	cout << "Balance: " << balance << endl;
	cout << "Mac md5: " << macMd5 << endl;
	cout << "Signature Fee: " << minerFee << endl;
	cout << "Package Fee: " << packageFee << endl;
    cout << "Block top: " << blockHeight << endl;
    cout << "Top block hash: " << bestChainHash << endl;
	cout << "********************************************************" << endl;
    cout << infoColor.reset();
}

int set_device_signature_fee(uint64_t minerFee)
{
    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
    Transaction* txn = pRocksDb->TransactionInit();

    ON_SCOPE_EXIT{
        pRocksDb->TransactionDelete(txn, true);
    };

    auto status = pRocksDb->SetDeviceSignatureFee(minerFee);
    if (status == 0) {
        net_update_fee_and_broadcast(minerFee);
    }
    return status;
}

int set_device_package_fee(uint64_t packageFee)
{
    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
    Transaction* txn = pRocksDb->TransactionInit();

    ON_SCOPE_EXIT{
        pRocksDb->TransactionDelete(txn, true);
    };

    int result = pRocksDb->SetDevicePackageFee(packageFee);
    if (result == 0)
    {
        net_update_package_fee_and_broadcast(packageFee);
    }
    return result;
}

int get_device_package_fee(uint64_t& packageFee)
{
    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
    Transaction* txn = pRocksDb->TransactionInit();

    ON_SCOPE_EXIT{
        pRocksDb->TransactionDelete(txn, true);
    };

    int result = pRocksDb->GetDevicePackageFee(packageFee);                
    return result;
}

void handle_transaction()
{
    std::string FromAddr, ToAddr, amt;
    std::cout<<"input FromAddr :"<<std::endl;
    std::cin>>FromAddr;

    if (FromAddr == "t")
    {
        AutoTranscation();
        return;
    }

    int needVerifyPreHashCount = 0;
    std::string minerFees;

    std::cout<<"input ToAddr :"<<std::endl;
    std::cin>>ToAddr;
    std::cout<<"input amount :"<<std::endl;
    std::cin>>amt;
    std::cout<<"input needVerifyPreHashCount :"<<std::endl;
    std::cin>>needVerifyPreHashCount;
    std::cout<<"input minerFees :"<<std::endl;
    std::cin>>minerFees;

    if( !isInteger( std::to_string(needVerifyPreHashCount) ) || needVerifyPreHashCount < g_MinNeedVerifyPreHashCount )
    {
        std::cout << "input needVerifyPreHashCount error ! " << std::endl;
        return;
    }

    if( std::stod(minerFees) <= 0 )
    {
        std::cout << "input minerFees error ! " << std::endl;
        return;
    }
    if(std::stod(minerFees) < 0.0001 || std::stod(minerFees) > 0.1)
    {
        std::cout << "The minerFees is not within reasonable range ! " << std::endl;
        return;
    }
    if (std::stod(amt) <= 0 || std::stod(amt) < 0.000001)
    {
        std::cout << "input amount error ! " << std::endl;
        return;
    }
    
    if(!FromAddr.compare(ToAddr))
    {
        std::cout << "The FromAddr is equal with Toaddr ! " << std::endl;
        return;
    } 

    if(1 == FromAddr.size())
    {
        CreateTx(NULL, ToAddr.c_str(), amt.c_str(), NULL, needVerifyPreHashCount, minerFees);
    }
    else
    {
        if(FromAddr.size() < 33 && FromAddr.size() > 34 &&  ToAddr.size() < 33 && ToAddr.size() > 34)
        {
            error("input Error !!!  ");
            return;
        }
        CreateTx(FromAddr.c_str(), ToAddr.c_str(), amt.c_str(), NULL, needVerifyPreHashCount, minerFees);
    }

}

void handle_pledge()
{
    std::string  fromAddrstr;
    //cout << "Please enter the account number to be pledged ："<< endl;
    //std::cin >> fromAddrstr;
    fromAddrstr = g_AccountInfo.DefaultKeyBs58Addr; //lzg
    cout<<"Default account ： "<< fromAddrstr << endl;  //lzg
    
    std::string pledge;
    cout << "Please enter the pledged amount ："<< endl;
    std::cin >> pledge;

    std::string GasFee;
    cout << "Please enter GasFee :" << endl;
    std::cin >> GasFee;

    std::string password;
    std::cout << "password" << endl;
    std::cin >> password;
    
    std::string txHash;
    int r = CreatePledgeTransaction(fromAddrstr, pledge, g_MinNeedVerifyPreHashCount, GasFee, password, PLEDGE_NET_LICENCE, txHash);
    if (r == -104)
    {
        std::cout << "password error!!!" << std::endl;
    }
}

void handle_redeem_pledge()
{
    // Get the signature information of all current blocks 
    // GetAllAccountSignNum();
    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
    Transaction* txn = pRocksDb->TransactionInit();

    ON_SCOPE_EXIT{
        pRocksDb->TransactionDelete(txn, true);
    };

    std::string addr;
    std::cout << "addr：";
    std::cin >> addr;

    std::vector<string> utxos;
    pRocksDb->GetPledgeAddressUtxo(txn, addr, utxos);
    std::reverse(utxos.begin(), utxos.end());

    std::cout << "-- Currently pledged  -- " << std::endl;
    for (auto & utxo : utxos)
    {
        std::cout << "utxo: " << utxo << std::endl;
    }
    std::cout << std::endl;

    std::string utxo;
    std::cout << "utxo：";
    std::cin >> utxo;

    std::string GasFee;
    std::cout << "GasFee：";
    std::cin >> GasFee;

    std::string password;
    std::cout << "password";
    std::cin >> password;

    std::string txHash;

    if (utxos.size() > 0)
    {
        int r = CreateRedeemTransaction(addr, g_MinNeedVerifyPreHashCount, GasFee, utxo, password, txHash);
        if (r == -9)
        {
            std::cout << "CreateRedeemTransaction error!!!" << std::endl;
        }
    }
}

/**
Class A address ：10.0.0.0 - 10.255.255.255 
Class B address ：172.16.0.0 - 172.31.255.255
Class C address ：192.168.0.0 -192.168.255.255 
 */
bool isPublicIp(const string& ip)
{
    static const char* ip192 = "192.168.";
    static const char* ip10 = "10.";
    static const char* ip172 = "172.";
    static const char* ipself = "127.0.0.1";
    if (strncmp(ip.c_str(), ipself, strlen(ipself)) == 0)
    {
        return false;
    }
    if (strncmp(ip.c_str(), ip192, strlen(ip192)) == 0)
    {
        return false;
    }
    else if (strncmp(ip.c_str(), ip10, strlen(ip10)) == 0)
    {
        return false;
    }
    else if (strncmp(ip.c_str(), ip172, strlen(ip172)) == 0)
    {
        int first = ip.find(".");
        int second = ip.find(".", first + 1);
        if (first >= 0 && second >= 0)
        {
            string strIp2 = ip.substr(first + 1, second - first - 1);
            int nIp2 = atoi(strIp2.c_str());
            if (nIp2 >= 16 && nIp2 <= 31)
            {
                return false;
            }
        }   
    }

    return true;
}

// Declare: update the public node to config file, create: 20201109   LiuMingLiang
int UpdatePublicNodeToConfigFile()
{
    std::vector<Node> nodes = net_get_all_public_node();
    std::vector<Node> publicNodes;
    
    // Add Self if it is public
    if (Singleton<Config>::get_instance()->GetIsPublicNode())
    {
        std::string localIp = Singleton<Config>::get_instance()->GetLocalIP();
        if (isPublicIp(localIp))
        {
            Node selfNode = net_get_self_node();
            cout << "Add self node: ^^^^^VVVVVV " << selfNode.base58address << ",Fee " << selfNode.fee << endl;
            nodes.push_back(selfNode);
        }
        else
        {
            cout << "Self node is not public ip ^^^^^VVVVVV " << localIp << endl;
        }
    }

    // Verify pledge and fees
    for (auto node : nodes)
    {
        if (node.public_ip == node.local_ip && isPublicIp(IpPort::ipsz(node.public_ip)))
        {
            if (node.public_port != SERVERMAINPORT)
            {
                std::cout << "if (node.public_port != SERVERMAINPORT)" << __LINE__ << std::endl;
                continue ;
            }
            uint64_t amount = 0;
            if (SearchPledge(node.base58address, amount) != 0)
            {
                cout << "No pledge " << IpPort::ipsz(node.public_ip) << endl;
                continue ;
            }
            if (amount < g_TxNeedPledgeAmt)
            {
                cout << "Less mount of pledge " << IpPort::ipsz(node.public_ip) << endl;
                // Insufficient pledge deposit 
                continue ;
            }

            if (node.fee > 0)
            {
                auto iter = publicNodes.begin();
                for (; iter != publicNodes.end(); ++iter)
                {
                    if (iter->public_ip == node.public_ip && iter->local_ip == node.local_ip)
                    {
                        break;
                    }
                }
                if (iter == publicNodes.end())
                {
                    publicNodes.push_back(node);
                    std::sort(publicNodes.begin(), publicNodes.end(),Compare(false));
		            publicNodes.erase(unique(publicNodes.begin(), publicNodes.end()), publicNodes.end());
                    cout << "New public node ^^^^^VVVVV " << IpPort::ipsz(node.public_ip) << endl;
                }
                else
                {
                    cout << "Duplicate node ^^^^^^VVVVV " << IpPort::ipsz(node.public_ip) << endl;
                }
            }
        }
        else
        {
            cout << "Not public ip ^^^^^VVVVV " << IpPort::ipsz(node.public_ip) << "," << IpPort::ipsz(node.local_ip) << endl;
        }
    }

    if (publicNodes.empty())
    {
        cout << "Update node is empty ^^^^^^VVVVVV" << endl;
        return 0;
    }
   
    if (publicNodes.size() < 5)
    {
        cout << "Update node is less than 5 ^^^^^VVVVV" << endl;
        cout << "Update node is less than 5 Node: " << publicNodes.size()  << endl;
        return 1;
    }

    Singleton<Config>::get_instance()->ClearNode();
    Singleton<Config>::get_instance()->ClearServer();
    // Add node to config, and write to config file
    for (auto& node : publicNodes)
    {
        string ip(IpPort::ipsz(node.public_ip));
       
        u16 port = node.public_port;
        string name(ip);
        string enable("1");

        node_info info/*{name, ip, port, enable}*/;
        info.ip = ip;
        info.port = port;
        info.name = name;
        info.enable = enable;
        Singleton<Config>::get_instance()->AddNewNode(info);
        Singleton<Config>::get_instance()->AddNewServer(ip, port);
    }
    Singleton<Config>::get_instance()->WriteServerToFile();
    cout << "Node: " << publicNodes.size() << " update to config!" << endl;

    return 0;
}

unsigned int get_chain_height()
{
    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
    Transaction* txn = pRocksDb->TransactionInit();
    if (txn == nullptr)
    {
        return 0;
    }
    ON_SCOPE_EXIT{
        pRocksDb->TransactionDelete(txn, true);
    };

    unsigned int chainHeight = 0;
	if (pRocksDb->GetBlockTop(txn, chainHeight) != 0)
	{
        return 0;
	}

    return chainHeight;
}

void net_register_chain_height_callback()
{
    net_callback::register_chain_height_callback(get_chain_height);
}


/**
 * @description: Register callback function 
 * @param {*} no 
 * @return {*} no 
 */
void RegisterCallback()
{
    // Block synchronization related 
    net_register_callback<SyncGetnodeInfoReq>(HandleSyncGetnodeInfoReq);
    net_register_callback<SyncGetnodeInfoAck>(HandleSyncGetnodeInfoAck);
    net_register_callback<SyncBlockInfoReq>(HandleSyncBlockInfoReq);
    net_register_callback<SyncBlockInfoAck>(HandleSyncBlockInfoAck);
    // net_register_callback<VerifyReliableNodeReq>(HandleVerifyReliableNodeReq);
    // net_register_callback<VerifyReliableNodeAck>(HandleVerifyReliableNodeAck);
    net_register_callback<SyncLoseBlockReq>(HandleSyncLoseBlockReq);
    net_register_callback<SyncLoseBlockAck>(HandleSyncLoseBlockAck);
    net_register_callback<SyncGetPledgeNodeReq>(HandleSyncGetPledgeNodeReq);
    net_register_callback<SyncGetPledgeNodeAck>(HandleSyncGetPledgeNodeAck);
    net_register_callback<SyncVerifyPledgeNodeReq>(HandleSyncVerifyPledgeNodeReq);
    net_register_callback<SyncVerifyPledgeNodeAck>(HandleSyncVerifyPledgeNodeAck);


    // Transaction interface 
    // PC transaction transaction flow 
    net_register_callback<TxMsg>(HandleTx);

    // One-to-one transaction with mainnet account on mobile terminal (old version) 
    net_register_callback<CreateTxMsgReq>(HandleCreateTxInfoReq); // Step 1
    net_register_callback<TxMsgReq>(HandlePreTxRaw); // Step 2 

    // Initiation of pledge on mobile mainnet account 
    net_register_callback<CreatePledgeTxMsgReq>(HandleCreatePledgeTxMsgReq); // Step 1
    net_register_callback<PledgeTxMsgReq>(HandlePledgeTxMsgReq);// Step 2 

    // The mobile phone mainnet account initiates the release of the pledge 
    net_register_callback<CreateRedeemTxMsgReq>(HandleCreateRedeemTxMsgReq); // Step 1
    net_register_callback<RedeemTxMsgReq>(HandleRedeemTxMsgReq);// Step 2 

    // Initiate multiple transactions on mobile mainnet accounts 
    net_register_callback<CreateMultiTxMsgReq>(HandleCreateMultiTxReq); // Step 1
    net_register_callback<MultiTxMsgReq>(HandleMultiTxReq); // Step 2 


    // Mobile phone connected to miner one-to-one transaction 
    net_register_callback<VerifyDevicePasswordReq>(HandleVerifyDevicePassword);
    net_register_callback<CreateDeviceTxMsgReq>(HandleCreateDeviceTxMsgReq);

    // The mobile terminal connects to the mining machine account to initiate multiple transactions 
    net_register_callback<CreateDeviceMultiTxMsgReq>(HandleCreateDeviceMultiTxMsgReq); 

    // The mobile terminal connects to the miner to initiate a pledge transaction 
    net_register_callback<CreateDevicePledgeTxMsgReq>(HandleCreateDevicePledgeTxMsgReq); 
    // 手机端连接矿机发起解除质押交易
    net_register_callback<CreateDeviceRedeemTxReq>(HandleCreateDeviceRedeemTxMsgReq); 
    
    
    // Auxiliary interface 
    // Test interface registration 
    net_register_callback<GetDevInfoAck>(HandleGetDevInfoAck);
    net_register_callback<GetDevInfoReq>(HandleGetDevInfoReq);

    // Scan the mining machine to obtain the mining machine information interface 
    net_register_callback<GetMacReq>(HandleGetMacReq);


    // broadcast 
    // Block broadcast 
    net_register_callback<BuileBlockBroadcastMsg>(HandleBuileBlockBroadcastMsg);
    // Transaction pending broadcast 
    net_register_callback<TxPendingBroadcastMsg>(HandleTxPendingBroadcastMsg);

    // Confirm whether the transaction is successful  20210309
    net_register_callback<ConfirmTransactionReq>(HandleConfirmTransactionReq);
    net_register_callback<ConfirmTransactionAck>(HandleConfirmTransactionAck);
    
    net_register_chain_height_callback();
}

/**
 * @description: Initialization fee and packaging fee 
 * @param {*} no 
 * @return {*} no 
 */
void InitFee()
{ 
	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();

    uint64_t publicNodePackageFee = 0;
    if( pRocksDb->GetDevicePackageFee(publicNodePackageFee) != 0 )
    {
        assert( pRocksDb->SetDevicePackageFee(publicNodePackageFee) == 0 );
    }
    //publicNodePackageFee = 20000;
    net_set_self_package_fee(publicNodePackageFee);
    uint64_t minfee = 0;
	pRocksDb->GetDeviceSignatureFee( minfee );
    if(minfee == 0)
    {
        minfee = 0;
        pRocksDb->SetDeviceSignatureFee(minfee);
    }
    //mineSignatureFee = 10000;
    net_set_self_fee(minfee); 

    // Register the master account address with the network layer 
    net_set_self_base58_address(g_AccountInfo.DefaultKeyBs58Addr);
    ca_console RegisterColor(kConsoleColor_Yellow, kConsoleColor_Black, true);
    std::cout << RegisterColor.color().c_str() << "RegisterCallback bs58Addr : " << g_AccountInfo.DefaultKeyBs58Addr << RegisterColor.reset().c_str() << std::endl;
}


void TestCreateTx(const std::vector<std::string> & addrs, const int & sleepTime, const std::string & signFee)
{
#if 0
    bIsCreateTx = true;
    while(1)
    {
        if (bStopTx)
        {
            break;
        }
        srand(time(NULL));
        int intPart = rand() % 10;
        double decPart = (double)(rand() % 100) / 100;
        double amount = intPart + decPart;
        std::string amountStr = std::to_string(amount);

        std::cout << std::endl << std::endl << std::endl << "=======================================================================" << std::endl;
        CreateTx("1vkS46QffeM4sDMBBjuJBiVkMQKY7Z8Tu", "18RM7FNtzDi41QEU5rAnrFdVaGBHvhTTH1", amountStr.c_str(), NULL, 6, "0.01");
        std::cout << "=====Transaction initiator ：1vkS46QffeM4sDMBBjuJBiVkMQKY7Z8Tu" << std::endl;
        std::cout << "=====Transaction recipient ：18RM7FNtzDi41QEU5rAnrFdVaGBHvhTTH1" << std::endl;
        std::cout << "=====Transaction amount   ：" << amountStr << std::endl;
        std::cout << "=======================================================================" << std::endl << std::endl << std::endl << std::endl;

        sleep(sleepTime);
    }
    bIsCreateTx = false;

#endif

#if 1
    bIsCreateTx = true;
    while(1)
    {
        if (bStopTx)
        {
            std::cout << "End the transaction ！" << std::endl;
            break;
        }
        srand(time(NULL));
        int intPart = rand() % 10;
        double decPart = (double)(rand() % 100) / 100;
        double amount = intPart + decPart;
        std::string amountStr = std::to_string(amount);

        std::cout << std::endl << std::endl << std::endl << "=======================================================================" << std::endl;
        CreateTx(addrs[0].c_str(), addrs[1].c_str(), amountStr.c_str(), NULL, 6, signFee);
        std::cout << "=====Transaction initiator ：" << addrs[0] << std::endl;
        std::cout << "=====Transaction recipient ：" << addrs[1] << std::endl;
        std::cout << "=====Transaction amount   ：" << amountStr << std::endl;
        std::cout << "=======================================================================" << std::endl << std::endl << std::endl << std::endl;

        sleep(sleepTime);

        std::cout << std::endl << std::endl << std::endl << "=======================================================================" << std::endl;
        CreateTx(addrs[1].c_str(), addrs[0].c_str(), amountStr.c_str(), NULL, 6, signFee);
        std::cout << "=====Transaction initiator ：" << addrs[1] << std::endl;
        std::cout << "=====Transaction recipient ：" << addrs[0] << std::endl;
        std::cout << "=====Transaction amount   ：" << amountStr << std::endl;
        std::cout << "=======================================================================" << std::endl << std::endl << std::endl << std::endl;
        sleep(sleepTime);
    }
    bIsCreateTx = false;
#endif
}