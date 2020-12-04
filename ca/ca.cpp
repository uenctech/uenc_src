#include "./ca.h"
#include "ca_buffer.h"
#include "ca_transaction.h"
#include "ca_global.h"
#include "ca_interface.h"
#include "ca_test.h"
#include "unistd.h"
#include "ca_hexcode.h"
#include "../include/cJSON.h"
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


const char *kVersion = "1.0.1";
std::shared_mutex g_NodeInfoLock;

//test--------
void testExchangeInterface(); //q 配置文件自动化随机交易测试
void testExchangeInterface2(); //a 配置文件自动化交易双向测试
void testExchangeInterface3(); //w 智能自动化交易测试
void testExchangeInterface4(); //e 并发交易测试
int testInterfaceConfig(std::map<int, char*>&, int&, int&);
const char *TEST_JSON_NAME = "test_json_interface.json";

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
		std::cout << "打开文件 sign.txt 失败！" << std::endl;
		return -2;
	}

	unsigned int blockHeight = 0;
	auto db_status = pRocksDb->GetBlockTop(txn, blockHeight);
	if (db_status != 0)
	{
		std::cout << "GetBlockTop 失败！" << std::endl;
		bRollback = true;
        return -3;
    }

	// 本地节点高度为0时
	if(blockHeight <= 0)
	{
		std::cout << "高度为0！" << std::endl;
		return -4;
	}

	for(unsigned int height = 1; height <= blockHeight; height++)
	{
		std::vector<std::string> hashs;
		db_status = pRocksDb->GetBlockHashsByBlockHeight(txn, height, hashs);
		if(db_status != 0) 
		{
			std::cout << "GetBlockHashsByBlockHeight 失败！" << std::endl;
			bRollback = true; 
			return -3;
		}

		for(auto blockHash : hashs)
		{
			std::string blockHeaderStr;
			db_status = pRocksDb->GetBlockByBlockHash(txn, blockHash, blockHeaderStr);
			if(db_status != 0)
			{
				std::cout << "GetBlockByBlockHash 失败！" << std::endl;
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

	file << std::string("账号总数：");
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
    //判断没有输入的情况
    if(str=="")
    {
        return false;
    }
    else 
    {
        //有输入的情况况
        for (size_t i = 0; i < str.size();i++) 
        {
            if (str.at(i) == '-' && str.size() > 1)  // 有可能出现负数
                continue;
            // 数值在ascii码（编码）的‘0’-‘9’之间
            if (str.at(i) > '9' || str.at(i) < '0')
                return false;
        }
        return true;
    }
}

void ca_initConfig()
{
	std::string logLevel = Singleton<Config>::get_instance()->GetLogLevel();
	if (logLevel.length() == 0)
	{
		logLevel = "TRACE";
	}
	setloglevel(logLevel);

	std::string logFilename = Singleton<Config>::get_instance()->GetLogFilename();
	if (logFilename.length() == 0)
	{
		logFilename = "log.txt";
	}
	setlogfile(logFilename);
}

int InitRockdb()
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
		//  初始化0块, 这里简化处理直接读取关键信息保存到数据库中
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
    cout << "请输入开始日期(格式 2020-01-01): ";
    cin >> date_s;
    date_s += " 00:00:00";
    strptime(date_s.c_str(), "%Y-%m-%d %H:%M:%S", &tmp_time);
    uint64_t t_s = (uint64_t)mktime(&tmp_time);
    cout << t_s << endl;

    cout << "请输入结束日期(格式 2020-01-07): ";
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

void ex_count_init() {
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
    //加块线程
    g_blockpool_timer.AsyncLoop(1000, [](){
		MagicSingleton<BlockPoll>::GetInstance()->Process();
	});


	//区块同步线程
	int sync_time = Singleton<Config>::get_instance()->GetSyncDataPollTime();
    g_synch_timer.AsyncLoop(sync_time * 1000 , [](){
		g_synch->Process();
	});
    
    //获取矿机在线时长
    g_deviceonline_timer.AsyncLoop(5 * 60 * 1000, GetOnLineTime);

    // Public node refresh to config file, 20201110
    g_public_node_refresh_timer.AsyncLoop(60 * 60 * 1000, [](){ UpdatePublicNodeToConfigFile(); });

    //g_device2pubnet_timer.AsyncLoop(1 * 60 * 1000, HandleUpdateProgramRequst);

    return 0;
}

bool ca_init()
{
	// 初始化数据库
	InitRockdb();
    //手机端接口注册
    MuiltipleApi();

    InitAccount(&g_AccountInfo, "./cert");
	Init("./data");

    // 初始化交易统计
    ex_count_init();

    net_register_callback<GetMacReq>(GetDeviceInfo);


	if(!g_AccountInfo.GetAccSize())
    {
        g_AccountInfo.GenerateKey();
    }


    // 启动定时器任务
    ca_startTimerTask();
	
	return true;
}

int ca_endTimerTask()
{
    g_blockpool_timer.Cancel();

    g_synch_timer.Cancel();
    
    g_deviceonline_timer.Cancel();

    g_public_node_refresh_timer.Cancel();

    g_device2pubnet_timer.Cancel();
    
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
                std::cout << "请输入要生成的账户数量：";
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
           
               
               // GetDevPrivateKeyReq getDevPrivateKeyReq;
                // GetServiceInfoReq getServiceInfoReq;
                // getServiceInfoReq.set_version(getVersion());
                // int number;
                // cout<<"请输入序列号："<<endl;
                // cin >> number;
                // getServiceInfoReq.set_sequence_number(number);
                // // std::string base58addr; 
                // // cout<<"请输入base58addr"<<endl;
                // // cin >> base58addr;
                // // getDevPrivateKeyReq.set_bs58addr(base58addr);
                // // std::string passwd; 
                // // cout<<"请输入passwd"<<endl;
                // // cin >> passwd;
                // // getDevPrivateKeyReq.set_password(passwd);
               
                // std::string id ;
                // cout<<"请输入id账号"<<endl;
                // cin >> id;
	     
                // //net_send_message<GetReleasePledgeTxReq>(id, getServiceInfoReq);
                // net_send_message<GetServiceInfoReq>(id, getServiceInfoReq);

                // GetServiceInfoReqTestHandle(const std::shared_ptr<ServiceInfoReq>& msg,const MsgData& msgdata);
                //std::shared_ptr<ServiceInfoReq> req = std::make_shared<ServiceInfoReq>();
                // ServiceInfoReq req;
                // req.set_version(getVersion());
                // req.set_sequence_number(1);
                // std::string id ;
                // cout<<"请输入id账号"<<endl;
                // cin >> id;
                // net_send_message<ServiceInfoReq>(id, req);
                // GetServiceInfoAck ack;
                // m_api::HandleGetPledgeListReq(req, ack);
               // m_api::HandleGetServiceInfoReqTest(req,);
                TestSetOnLineTime();
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
                std::set<std::string> options {"1. 获取全部交易笔数", "2. 获取交易块详细信息",
                "3.查看当区块奖励和总区块块奖励","4.查看所有区块里边签名账号的累计金额"};
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
                        cout<<"第"<<top<<"个区块奖励金额:" <<SingleTotal<<endl;
                        top--;
                    }
                    cout<<"所有区块奖励金额:"<<totalaward<<endl;
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
                                            cout<<"第一次放入map的第"<<i <<"区块的签名额外奖励"<<txout.scriptpubkey()<< "->"<<txout.value()<<endl;
                                            
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
                                                cout<<"地址相同时第"<<i <<"区块的签名额外奖励"<<txout.scriptpubkey()<< "->"<<newvalue<<endl; 
                                                
                                            }
                                            else
                                            {
                                                addrvalue.insert(pair<string,int64_t>(txout.scriptpubkey(),txout.value()));
                                                cout<<"地址不同时第"<<i <<"区块的签名额外奖励"<<txout.scriptpubkey()<< "->"<<txout.value()<<endl;
                                               
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
                }
                default:
                        break;
                }
                break;
			}
			case 6:
			{
				unsigned int height = 0;
				std::cout << "Rollback block to Height: " ;
				std::cin >> height;

				RollbackToHeight(height);

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
				std::cout << "2. 模拟质押资产." << std::endl;
				std::cout << "3. 得到本机设置的燃料费和全网节点最大最小平均值燃料费" << std::endl;
				std::cout << "4. 根据UTXO查询余额" << std::endl;
				std::cout << "5. 设置节点签名费" << std::endl;
                std::cout << "8. 模拟解质押资产" << std::endl;
                std::cout << "9. 查询账号质押资产额度" << std::endl;
                std::cout << "10.多账号交易" << std::endl;
                std::cout << "11.查询交易列表" << std::endl;
                std::cout << "12.查询交易详情" << std::endl;
                std::cout << "13.查询区块列表" << std::endl;
                std::cout << "14.查询区块详情" << std::endl;
                std::cout << "15.查询所有质押地址" << std::endl;
                std::cout << "16.获取5分钟内总块数" << std::endl;
                std::cout << "17.设置节点打包费用" << std::endl;
                std::cout << "18.获得所有公网节点打包费用" << std::endl;
                std::cout << "19.自动乱序交易(简化版)" << std::endl;
                std::cout << "20.测试获取前100块各个账号奖励" << std::endl;

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

                        std::cout << "查询地址：" ;
                        std::string addr;
                        std::cin >> addr;
                        std::vector<std::string> utxoHashs;
                        pRocksDb->GetUtxoHashsByAddress(txn, addr, utxoHashs);
                        uint64_t total = 0;
                        std::cout << "账户:" << addr << std::endl << "utxo:" << std::endl;
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

                        std::cout << "UTXO 总值：" << total << std::endl;
						break;
					}
                    case 5 :
                    {
                        uint64_t service_fee = 0, show_service_fee;
                        cout << "输入矿费: ";
                        cin >> service_fee;

                        auto rdb_ptr = MagicSingleton<Rocksdb>::GetInstance();
                        auto status = rdb_ptr->SetDeviceSignatureFee(service_fee);
                        if (!status) {
                            net_update_fee_and_broadcast(service_fee);
                        }

                        rdb_ptr->GetDeviceSignatureFee(show_service_fee);
                        cout << "矿费---------------------------------------------------------->>" << show_service_fee << endl;
                        break;
                    }
                    case 6 :
                    {
                        std::vector<std::string> test_list;
                        test_list.push_back("1. 获得最高块hash和高度");
                        test_list.push_back("2. 获得mac");

                        for (auto v : test_list) {
                            std::cout << v << std::endl;
                        }

                        std::string node_id;
                        std::cout << "输入id: ";
                        std::cin >> node_id;

                        uint32_t test_num {0};
                        std::cout << "输入选项号: ";
                        std::cin >> test_num;

                        CaTestInfoReq ca_test_req;
                        ca_test_req.set_test_num(test_num);

                        net_send_message<CaTestInfoReq>(node_id, ca_test_req);
                    }
                    case 7:
                    {
                        const std::string addr = "16psRip78QvUruQr9fMzr8EomtFS1bVaXk";
                        CTransaction tx;

                        uint64_t time = Singleton<TimeUtil>::get_instance()->getTimestamp();
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
                        std::cout << "查询账号质押资产额度：" << std::endl;
                        { // 查询质押

                            std::string addr;
                            std::cout << "查询账号：";
                            std::cin >> addr;

                            std::shared_ptr<GetPledgeListReq> req = std::make_shared<GetPledgeListReq>();
                            req->set_version(getVersion());
                            req->set_addr(addr);
                            req->set_index(0);
                            req->set_count(100);

                            GetPledgeListAck ack;
                            m_api::HandleGetPledgeListReq(req, ack);


                            if (ack.code() != 0)
                            {
                                error("get pledge failed!");
                                std::cout << "get pledge failed!" << std::endl;
                                break;
                            }

                            std::cout << "pledge total: " << ack.total() << std::endl;

                            size_t size = (size_t)ack.list_size();
                            for (size_t i = 0; i < size; i++)
                            {
                                std::cout << "----- pledge " << i << " start -----" << std::endl;
                                const PledgeItem item = ack.list(i);
                                std::cout << "blockhash: " << item.blockhash() << std::endl;
                                std::cout << "blockheight: " << item.blockheight() << std::endl;
                                std::cout << "utxo: " << item.utxo() << std::endl;
                                std::cout << "amount: " << item.amount() << std::endl;
                                std::cout << "time: " << item.time() << std::endl;
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
                        std::cout << "发起方账号个数:";
                        std::cin >> addrCount;
                        for (int i = 0; i < addrCount; ++i)
                        {
                            std::string addr;
                            std::cout << "发起账号" << i + 1 << ": ";
                            std::cin >> addr;
                            fromAddr.push_back(addr);
                        }

                        std::cout << "接收方账号个数:";
                        std::cin >> addrCount;
                        for (int i = 0; i < addrCount; ++i)
                        {
                            std::string addr;
                            double amt = 0; 
                            std::cout << "接收账号" << i + 1 << ": ";
                            std::cin >> addr;
                            std::cout << "金额: ";
                            std::cin >> amt;
                            toAddr.insert(make_pair(addr, amt * DECIMAL_NUM));
                        }

                        std::cout << "签名数:";
                        std::cin >> needVerifyPreHashCount;

                        std::cout << "手续费:";
                        std::cin >> minerFees;

                        TxHelper::DoCreateTx(fromAddr, toAddr, needVerifyPreHashCount, minerFees * DECIMAL_NUM);
                        break;
                    }
                    case 11:
                    {

                        { // 交易列表

                            std::string addr;
                            std::cout << "查询账号：";
                            std::cin >> addr;

                            size_t index = 0;
                            std::cout << "输入index：";
                            std::cin >> index;

                            size_t count = 0;
                            std::cout << "输入count：";
                            std::cin >> count;

                            std::shared_ptr<GetTxInfoListReq> req = std::make_shared<GetTxInfoListReq>();
                            req->set_version(getVersion());
                            req->set_addr(addr);
                            req->set_index(index);
                            req->set_count(count);

                            GetTxInfoListAck ack;
                            m_api::HandleGetTxInfoListReq(req, ack);

                            if (ack.code() != 0)
                            {
                                error("get pledge failed!");
                                return;
                            }

                            std::cout << "total:" << ack.total() << std::endl;
                            std::cout << "index:" << ack.index() << std::endl;

                            for (size_t i = 0; i < (size_t)ack.list_size(); i++)
                            {
                                std::cout << "----- txInfo " << i << " start -----" << std::endl;
                                const TxInfoItem item = ack.list(i);

                                std::cout << "txhash: " << item.txhash() << std::endl;
                                std::cout << "time: " << item.time() << std::endl;
                                std::cout << "amount: " << item.amount() << std::endl;

                                TxInfoType type = item.type();
                                std::string strType;
                                if (type == TxInfoType_Originator)
                                {
                                    strType = "交易发起方";
                                }
                                else if (type == TxInfoType_Receiver)
                                {
                                    strType = "交易接收方";
                                }
                                else if (type == TxInfoType_Gas)
                                {
                                    strType = "手续费奖励";
                                }
                                else if (type == TxInfoType_Award)
                                {
                                    strType = "区块奖励";
                                }
                                else if (type == TxInfoType_Pledge)
                                {
                                    strType = "质押";
                                }
                                else if (type == TxInfoType_Redeem)
                                {
                                    strType = "解除质押";
                                }
                                else if (type == TxInfoType_PledgedAndRedeemed)
                                {
                                    strType = "质押但已解除";
                                }

                                std::cout << "type: " << strType << std::endl;
                                std::cout << "----- txInfo " << i << " end -----" << std::endl;
                            }

                        }
                        break;
                    }
                    case 12:
                    {
                        { // 查询交易
                            std::string txHash;
                            std::cout << "查询交易哈希：";
                            std::cin >> txHash;

                            std::string addr;
                            std::cout << "查询地址: " ;
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
                            std::cout << "    time: " << ack.time() << std::endl;

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
                        std::cout << "查询index：";
                        std::cin >> index;

                        size_t count = 0;
                        std::cout << "查询count：" ;
                        std::cin >> count;
                        
                        GetBlockInfoListReq req;
                        req.set_version(getVersion());
                        req.set_index(index);
                        req.set_count(count);

                        std::cout << " -- 附近节点id列表 -- " << std::endl;
                        std::vector<std::string> ids = net_get_node_ids();
                        for (auto & id : ids)
                        {
                            std::cout << id << std::endl;
                        }

                        std::string id;
                        std::cout << "查询ID：";
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
                                
                                char blockTime[256] = {};
                                time_t s = (time_t)(item.time() / 1000000);
                                struct tm * gm_date = localtime(&s);
                        		sprintf(blockTime, "%d-%d-%d %d:%d:%d (%zu) \n", gm_date->tm_year + 1900, gm_date->tm_mon + 1, gm_date->tm_mday, gm_date->tm_hour, gm_date->tm_min, gm_date->tm_sec, item.time());
                                std::cout << "time: " << blockTime << std::endl;

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

                            std::cout << "查询结束" << std::endl;
                        });

                        net_send_message<GetBlockInfoListReq>(id, req);

                        break;
                    }
                    case 14:
                    {
                        std::string hash;
                        std::cout << "查询 block hash：" ;
                        std::cin >> hash;

                        GetBlockInfoDetailReq req;
                        req.set_version(getVersion());
                        req.set_blockhash(hash);

                        std::cout << " -- 附近节点id列表 -- " << std::endl;
                        std::vector<std::string> ids = net_get_node_ids();
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
                            
                            char blockTime[256] = {};
                            time_t s = (time_t)(ack->time() / 1000000);
                            struct tm * gm_date = localtime(&s);
                            sprintf(blockTime, "%d-%d-%d %d:%d:%d (%zu) \n", gm_date->tm_year + 1900, gm_date->tm_mon + 1, gm_date->tm_mday, gm_date->tm_hour, gm_date->tm_min, gm_date->tm_sec, ack->time());
                            std::cout << "time: " << blockTime << std::endl;
                            
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

                            std::cout << "查询结束" << std::endl;
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

                        std::cout << std::endl << "---- 已质押地址 start ----" << std::endl;
                        for (auto & addr : addressVec)
                        {
                            uint64_t pledgeamount = 0;
                            SearchPledge(addr, pledgeamount);
                            std::cout << addr << " : " << pledgeamount << std::endl;
                        }
                        std::cout << "---- 已质押地址数量:" << addressVec.size() << " ----" << std::endl << std::endl;

                        std::cout << "---- 已质押地址 end  ----" << std::endl << std::endl;

                        break;
                    }
                    case 16:
                    {
                        std::cout << "获取5分钟内总块数" << std::endl;
                        a_award::AwardAlgorithm awardAlgorithm;

                        uint64_t blockSum = 0;
                        awardAlgorithm.GetBlockNumInUnitTime(blockSum);
                        std::cout << "总块数：" << blockSum << std::endl;
                        break;
                    }
                    case 17:
                    {
                        uint64_t packageFee = 0;
                        get_device_package_fee(packageFee);

                        std::cout << "当前打包费用: " << packageFee << std::endl;
                        std::cout << "设置打包费用(必须为正整数): ";
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
                            std::cout << "请输入正确范围内的打包费(0-100000000)" << std::endl;
                            break ;
                        }
                        
                        int result = set_device_package_fee(newPackageFee);
                        if (result == 0)
                        {
                            std::cout << "设置打包费成功 " << newPackageFee << std::endl;
                        }
                        else
                        {
                            std::cout << "设置打包费失败" << std::endl;
                        }

                        break;
                    }
                    case 18:
                    {
                        std::cout << " -- 附近节点id列表 -- " << std::endl;
                        std::vector<std::string> ids = net_get_node_ids();
                        for (auto & id : ids)
                        {
                            std::cout << id << std::endl;
                        }

                        std::string id;
                        std::cout << "查询ID：";
                        std::cin >> id;


                        net_register_callback<GetNodeInfoAck>([] (const std::shared_ptr<GetNodeInfoAck> & ack, const MsgData &msgdata){

                            std::cout << "显示公网节点" << std::endl;
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
                        int addrNum = 0;
                        std::cout << "账号数量：";
                        std::cin >> addrNum;

                        std::vector<std::string> addrs;
                        for (int i = 0; i < addrNum; ++i)
                        {
                            std::string tmpAddr;
                            std::cout << "账号" << i << ": ";
                            std::cin >> tmpAddr;
                            addrs.push_back(tmpAddr);
                        }

                        std::string signFee;
                        std::cout << "手续费：";
                        std::cin >> signFee;

                        int sleepTime = 0;
                        std::cout << "间隔时间(秒)：";
                        std::cin >> sleepTime;

                        while(1)
                        {
                            srand(time(NULL));
                            int intPart = rand() % 10;
                            double decPart = (double)(rand() % 100) / 100;
                            double amount = intPart + decPart;
                            std::string amountStr = std::to_string(amount);

                            std::cout << std::endl << std::endl << std::endl << "=======================================================================" << std::endl;
                            CreateTx(addrs[0].c_str(), addrs[1].c_str(), amountStr.c_str(), NULL, 6, signFee);
                            std::cout << "=====交易发起方：" << addrs[0] << std::endl;
                            std::cout << "=====交易接收方：" << addrs[1] << std::endl;
                            std::cout << "=====交易金额  ：" << amountStr << std::endl;
                            std::cout << "=======================================================================" << std::endl << std::endl << std::endl << std::endl;

                            sleep(sleepTime);

                            std::cout << std::endl << std::endl << std::endl << "=======================================================================" << std::endl;
                            CreateTx(addrs[1].c_str(), addrs[0].c_str(), amountStr.c_str(), NULL, 6, signFee);
                            std::cout << "=====交易发起方：" << addrs[1] << std::endl;
                            std::cout << "=====交易接收方：" << addrs[0] << std::endl;
                            std::cout << "=====交易金额  ：" << amountStr << std::endl;
                            std::cout << "=======================================================================" << std::endl << std::endl << std::endl << std::endl;
                            sleep(sleepTime);
                        }
                        break;
                    }
                    case 20: 
                    {
                        int blockNum = 500;

                        // 起始时间
                        clock_t start = clock();
                        // 获取前500块各个账号收益平均值
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

                        // 存放账号和前500高度总奖励
                        std::map<std::string, int64_t> addrAwards;

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
                                        }
                                    }
                                }
                            }
                        }
                        clock_t end = clock();

                        for (auto & addrAward : addrAwards)
                        {
                            std::cout << "账号：" << addrAward.first << "   奖励：" << addrAward.second << std::endl;
                        }

                        std::cout << "总耗时：" << ((double)end - start) / CLOCKS_PER_SEC << "秒" <<std::endl;

                        if (addrAwards.size() == 0)
                        {
                            break;
                        }

                        std::vector<uint64_t> awards;  // 存放所有奖励值
                        for (auto & addrAward : addrAwards)
                        {
                            awards.push_back(addrAward.second);
                        }
                        std::sort(awards.begin(), awards.end());
                        uint64_t quarterNum = awards.size() * 0.25;
                        uint64_t threeQuarterNum = awards.size() * 0.75;
                        if (quarterNum == threeQuarterNum)
                        {
                            break;
                        }
                        uint64_t quarterValue = awards[quarterNum];
                        uint64_t threeQuarterValue = awards[threeQuarterNum];
                        uint64_t diffValue = threeQuarterValue - quarterValue;
                        uint64_t upperLimitValue = threeQuarterValue + (diffValue * 1.5);

                        std::cout << "********** 1/4 = " << quarterValue << std::endl;
                        std::cout << "********** 3/4 = " << threeQuarterValue << std::endl;
                        std::cout << "********** 差值 = " << diffValue << std::endl;
                        std::cout << "********** 上限 = " << upperLimitValue << std::endl;

                        for (auto & addrAward : addrAwards)
                        {
                            if ((uint64_t)addrAward.second > upperLimitValue)
                            {
                                // std::cout << "********** 异常账号：" << addrAward.first << "   总奖励 = " << addrAward.second << std::endl;
                            }
                        }
                        break;
                    }
                    case 111:
                    {
                        std::string fileName("account.txt");

                        ca_console redColor(kConsoleColor_Red, kConsoleColor_Black, true); 
                        std::cout << redColor.color() << "输出文件为：" << fileName << " 请用Courier New字体查看" << redColor.reset() <<  std::endl;

                        ofstream file;
                        file.open(fileName);

                        file << "请用Courier New字体查看" << std::endl << std::endl;

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
                        string id;
                        cout << "Please input id of the node:" ;
                        cin >> id;

						SendExitNode(id);
						break;	
					}

					case 2:
					{
                        string id;
                        cout << "Please input id of the node:" ;
                        cin >> id;
						
                        SendGetNodeHeightHashBase58AddrReq(id);
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

                        std::vector<std::string> nodes = net_get_node_ids();;
                        if(nodes.size() <= 0)
                        {
                            std::cout << "未找到节点！" << std::endl;
                            break;
                        }

						std::ofstream fileStream;
						fileStream.open("node.txt");
						if(!fileStream)
						{
							std::cout << "打开文件失败！" << std::endl;
							break;
						}

						for(auto i : nodes)
						{
							// 获取所有节点的块信息
							SendGetNodeHeightHashBase58AddrReq(i);
						}

                        for(int j = 0; j < 10; j++)
						{
							if( g_nodeinfo.size() < nodes.size() )
							{
								std::cout << "已获取到 ：" << g_nodeinfo.size() << std::endl;
								sleep(1);
							}
							else
							{
								break;
							}
						}

                        std::cout << "节点总数 ：" << nodes.size() << std::endl;
						std::cout << "已获取到 ：" << g_nodeinfo.size() << std::endl;
						std::cout << "开始写文件！" << std::endl;

                        g_NodeInfoLock.lock_shared();
                        for(auto k : g_nodeinfo)
						{
                            std::cout << "id ：" << k.id() << std::endl;
                            std::cout << "高度  ：" << k.height() << std::endl;
                            std::cout << "hash：" << k.hash() << std::endl;
                            std::cout << "addr：" << k.base58addr() << std::endl << std::endl;

							// 所有节点写入node.txt 文件
							fileStream << "ID：";
							fileStream << k.id();
							fileStream << ",高度：";
							fileStream << k.height();
							fileStream << ",hash：";
							fileStream << k.hash();
							fileStream << ",base58addr：";
							fileStream << k.base58addr();
							fileStream << "\n";
						}
                        g_NodeInfoLock.unlock_shared();

                        std::cout << "结束！" << std::endl;
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
                g_synch->DataSynch(id);
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

//**测试交易接口配置 写JSON入文件
//**返回账号总数
int testInterfaceConfig(std::map<int, char*> &account_map, int &sleeps, int &dis_asset) {
    using namespace std;

    const int num = 2;
    cJSON *root, *root_son, *account;
    cJSON *json_account;

    fstream test_json;

    test_json.open(TEST_JSON_NAME, ofstream::in);
    if (test_json.is_open()) { //文件存在则读取文件
        test_json.open(TEST_JSON_NAME, ifstream::out);

        stringstream buffer;
        buffer << test_json.rdbuf();
        string json_content = buffer.str();

        auto json_parse = cJSON_Parse(json_content.c_str());
        auto json_interface = cJSON_GetObjectItem(json_parse, "interface");
        auto json_arr = cJSON_GetObjectItem(json_interface, "account");
        auto json_accountNum = cJSON_GetArraySize(json_arr);
        auto json_sleeps = cJSON_GetObjectItem(json_interface, "sleep");
        auto json_asset = cJSON_GetObjectItem(json_interface, "dis_asset");

        sleeps = json_sleeps->valueint;
        dis_asset = json_asset->valueint;

        for (auto i = 0; i < json_accountNum; ++i) {
            json_account = cJSON_GetArrayItem(json_arr, i);

            account_map.insert({i, json_account->valuestring});
        }
        cout << cJSON_Print(cJSON_GetObjectItem(json_interface, "sleep")) << endl;
        return json_accountNum;
    }
    //创建配置文件
    test_json.open(TEST_JSON_NAME, ofstream::out | ofstream::app);

    root = cJSON_CreateObject();
    root_son = cJSON_CreateObject();
    //配置文件初始化
    const char* numbers[num] {"account1", "account2"};
    cJSON_AddItemToObject(root, "interface", root_son);

    cJSON_AddNumberToObject(root_son, "dis_asset", 10000);

    cJSON_AddNumberToObject(root_son, "sleep", 3000);

    account = cJSON_CreateStringArray(numbers, num);

    cJSON_AddItemToObject(root_son, "account", account);

    auto json = cJSON_Print(root);
    cout << json << endl;

    test_json << json;
    return 0;
}

//**测试交易接口
void testExchangeInterface() {
    using namespace std;
    const auto EXCHANGE_COUNT = 10000;

    auto account_num {0};
    map<int, char*>account_map;

    auto sleeps = 3000;
    auto dis_asset = 10000;

    account_num = testInterfaceConfig(account_map, sleeps, dis_asset);
    sleeps *= 1000;
    if (!account_num) {
        return;
    }

    //总账号初始化交易
    char asset[10];
    sprintf(asset, "%d", dis_asset);

    for (auto i = 0; i < account_num; ++i) {
        cout << account_map[i] << endl;
        CreateTx("16psRip78QvUruQr9fMzr8EomtFS1bVaXk", account_map[i], asset, NULL, g_MinNeedVerifyPreHashCount, "0.1");
        usleep(sleeps);
    }

    //随机
    uniform_int_distribution<unsigned> rand_engine(0, account_num - 1);
    default_random_engine rand(time(0));

    string asset_random = "1";
    for (auto i = 1; i <= EXCHANGE_COUNT; ++i) {
        auto compare1 = rand_engine(rand);
        auto compare2 = rand_engine(rand);

        //去重
        while (true) {
            if (compare1 == compare2) {
                compare2 = rand_engine(rand);
            } else {
                break;
            }
        }

        int re = CreateTx(account_map[compare1], account_map[compare2], asset_random.c_str(), NULL, g_MinNeedVerifyPreHashCount, "0.1");
        if (re == 0) {
            cout << "success" << endl;
        } else {
            cout << "err" << endl;
            break;
        }

        cout << "account1: " << account_map[compare1] << endl;
        cout << "account2: " << account_map[compare2] << endl;
        cout << endl;
        usleep(sleeps);
    }
}

//**测试交易接口2
void testExchangeInterface2() {
    using namespace std;

    auto account_num {0};
    map<int, char*>account_map;

    auto sleeps = 3000;
    auto dis_asset = 10000;

    account_num = testInterfaceConfig(account_map, sleeps, dis_asset);
    sleeps *= 1000;
    if (!account_num) {
        return;
    }

    //总账号初始化交易
    char asset[10];
    sprintf(asset, "%d", dis_asset);

    cout << account_map[0] << endl;
    CreateTx("16psRip78QvUruQr9fMzr8EomtFS1bVaXk", account_map[0], asset, NULL, g_MinNeedVerifyPreHashCount, "0.1");

    //随机
    uniform_int_distribution<unsigned> rand_engine(0, account_num - 1);
    default_random_engine rand(time(0));

    string asset_random = "1";

    while (true) {
        int re = CreateTx(account_map[0], account_map[1], asset_random.c_str(), NULL, g_MinNeedVerifyPreHashCount, "0.1");
        if (re == 0) {
            cout << "success" << endl;
        } else {
            cout << "err" << endl;
            break;
        }

        cout << "account1: " << account_map[0] << endl;
        cout << "account2: " << account_map[1] << endl;
        cout << endl;
        usleep(sleeps);
    }
}

void test_pth(char *x) {
    while (*x != 'q') {
        cin >> *x;
    }
    return;
}

//**测试交易接口3
void testExchangeInterface3() {
    using namespace std;

    unsigned top {0};
    unsigned temp_top {0};
    double sleeps {2000};
    unsigned dis_asset {1000};
    int account_num;
    char sign;
    std::vector<std::string> account_list;

    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if( txn == NULL )
	{
		std::cout << "(testExchangeInterface3) TransactionInit failed !" << std::endl;
		return ;
	}

	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, true);
	};

    pRocksDb->GetBlockTop(txn, top);
    temp_top = top;
    sleeps *= 1000;

    cout << "请输入生成账号数量(0为不生成): ";
    cin >> account_num;
    cout << endl;

    cout << "请输入修改间隔秒数: ";
    cin >> sleeps;
    sleeps *= 1000000;
    cout << endl;

    uint8_t is_rand {0};
    cout << "是否随机: (1为随机 0为不随机)";
    cin >> is_rand;

    auto i_sleeps = int(sleeps);

    if (account_num) {
        for (auto i = 0; i <= account_num; ++i) {
            g_AccountInfo.GenerateKey();
            // string preg(".");
            // int first_point;
            // int end_length = (g_AccountInfo.CurrentUser.PublicKeyName).length();
            // string key_name = (g_AccountInfo.CurrentUser.PublicKeyName).substr(7, end_length-1);
            // first_point = key_name.find(preg);
            // key_name = key_name.substr(0, first_point);
            // test_account_list.push_back(key_name);
        }
    }

    auto iter = g_AccountInfo.AccountList.begin();
    while (iter != g_AccountInfo.AccountList.end()) {
        string key_name = iter->first;
        if (key_name != "16psRip78QvUruQr9fMzr8EomtFS1bVaXk") {
            std::cout << key_name << std::endl;
            account_list.push_back(key_name);
        }
        iter++;
    }

    //总账号初始化交易
    char asset[10];
    sprintf(asset, "%d", dis_asset);

    for (auto v : account_list) {
        uint64_t amount = CheckBalanceFromRocksDb(v.c_str());
        if (amount < 200*1000) {
            CreateTx("16psRip78QvUruQr9fMzr8EomtFS1bVaXk", v.c_str(), asset, NULL, g_MinNeedVerifyPreHashCount, "0.1");
            cout << "16psRip78QvUruQr9fMzr8EomtFS1bVaXk------->>" << v.c_str() << endl;
            pRocksDb->GetBlockTop(txn, top);
            cout << "top-->" << top << endl;
            usleep(i_sleeps);
        } else {
            cout  << "\033[32m" << v.c_str() << "已分配资产" << "\033[1m"<< endl;
        }
    }

    thread first(test_pth, &sign);

    if (is_rand) {
        string d_gas;

        default_random_engine rand_list(time(0));
        while (true) {
            auto node_fee = net_get_node_ids_and_fees();
            if (node_fee.size() > 1) {

                std::vector<uint64_t> node_fee_list;
                for (auto v : node_fee) {
                    node_fee_list.push_back(v.second);
                }

                sort(node_fee_list.begin(), node_fee_list.end());
                uint32_t fee_list_count = static_cast<uint32_t>(node_fee_list.size());

                // std::cout << "fee_list_count0--" << node_fee_list[0] << std::endl;
                // std::cout << "fee_list_count e--" << node_fee_list[fee_list_count-1] << std::endl;
                // std::cout << "fee_list_count" << fee_list_count << std::endl;
                //0.51 - 1
                uint32_t list_begin = fee_list_count * 0.51;
                uint32_t list_end = fee_list_count;

                list_begin = list_begin == 0 ? 1 : list_begin;
                list_end = list_end == 0 ? 1 : list_end;
                // cout << "51-------" << list_begin - 1 << endl;
                // cout << "100-------" << list_end - 1 << endl;
                //51 - 100 随机

                uniform_int_distribution<uint64_t> rand_engine_list(list_begin - 1, list_end - 1);
                // cout << "time " << time(0) << endl;

                auto gas_num = rand_engine_list(rand_list);
                uint64_t gas = node_fee_list[gas_num];
                string d_gas = to_string((double)gas/DECIMAL_NUM);
                // cout << "gas_num-------" << gas_num << endl;
                // cout << "d_gas-------" << d_gas << endl;
            } else {
                cout << "全网燃料费节点不足一个!!!!!!!" << endl;
                usleep(i_sleeps);
                break;
            }
            //随机交易
            uniform_int_distribution<unsigned> rand_engine(0, account_list.size() - 1);
            default_random_engine rand(time(0));

            if (sign == 'q') {
                cout << "sign--" << sign << endl;
                break;
            }
            auto compare1 = rand_engine(rand);
            auto compare2 = rand_engine(rand);

            //去重
            while (true) {
                if (compare1 == compare2) {
                    compare2 = rand_engine(rand);
                } else {
                    break;
                }
            }
            cout << account_list[compare1] << endl;
            cout << account_list[compare2] << endl;

            CreateTx(account_list[compare1].c_str(), account_list[compare2].c_str(), "1", NULL, g_MinNeedVerifyPreHashCount, d_gas);
            usleep(i_sleeps);
        }

    } else {
        for (auto i = 0; i < int(account_list.size() - 2); ++i) {
            for (auto i_i = 0; i_i < 100; ++i_i) {
                cout << "!!!!!!!" << endl;
                if (sign == 'q') {
                    cout << "sign--" << sign << endl;
                    break;
                }
                std::vector<std::string> utxoHashs;
                pRocksDb->GetUtxoHashsByAddress(txn, account_list[i], utxoHashs);
                cout << "utxo hash size---" << utxoHashs.size() <<endl;
                if (utxoHashs.size() > 66) {
                    cout << "utxo hash next---" <<endl;
                    continue;
                }

                CreateTx(account_list[i].c_str(), account_list[i+1].c_str(), "1", NULL, g_MinNeedVerifyPreHashCount, "3");
                cout << "bus-----1 " << account_list[i] << endl;
                cout << "bus-----2 " << account_list[i+1] << endl;
                ++temp_top;
                cout << "temp_top--" << temp_top << endl;
                cout << "top--" << top << endl;
                usleep(i_sleeps);
                pRocksDb->GetBlockTop(txn, top);
            }
            if (sign == 'q') {
                cout << "sign--" << sign << endl;
                break;
            }
        }
    }

    first.join();
    cout << "!!!!!!!!!!!!!!!!!!!! " << top << endl;
    return;
}



//**测试交易接口4
void testExchangeInterface4() 
{
    using namespace std;

    unsigned top {0};
    double sleeps {2000};
    unsigned dis_asset {10000};
    int account_num;
    char sign;
    std::vector<std::string> account_list;
    string testnetstr = "1vkS46QffeM4sDMBBjuJBiVkMQKY7Z8Tu";
    string mainnetstr = "16psRip78QvUruQr9fMzr8EomtFS1bVaXk";
    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if( txn == NULL )
	{
		std::cout << "(testExchangeInterface3) TransactionInit failed !" << std::endl;
		return ;
	}

	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, false);
	};

    pRocksDb->GetBlockTop(txn, top);
    sleeps *= 1000;

    cout << "请输入生成账号数量(0为不生成): ";
    cin >> account_num;
    cout << endl;

    cout << "请输入修改间隔秒数: ";
    cin >> sleeps;
    sleeps *= 1000000;
    cout << endl;

    auto i_sleeps = int(sleeps);

    if (account_num) 
    {
        for (auto i = 0; i < account_num; ++i) 
        {
            g_AccountInfo.GenerateKey();
        }
    }

    if(g_testflag == 1)
    {
        auto iter = g_AccountInfo.AccountList.begin();
        while (iter != g_AccountInfo.AccountList.end()) 
        {
            string key_name = iter->first;
            if (key_name != testnetstr) 
            {
                std::cout << key_name << std::endl;
                account_list.push_back(key_name);
            }
            iter++;
        }
    }
    else
    {
        auto iter = g_AccountInfo.AccountList.begin();
        while (iter != g_AccountInfo.AccountList.end()) 
        {
            string key_name = iter->first;
            if (key_name != mainnetstr) 
            {
                std::cout << key_name << std::endl;
             account_list.push_back(key_name);
            }
            iter++;
        }
    }
    
    //总账号初始化交易
    char asset[10];
    sprintf(asset, "%d", dis_asset);
	cout << "初始化资产begin==========" << endl;
    for (auto &v : account_list) 
    {
        uint64_t amount = CheckBalanceFromRocksDb(v.c_str());
        if (amount < dis_asset * 1000 * 1000) 
        {
            if(g_testflag == 1)
            {
                CreateTx(testnetstr.c_str(), v.c_str(), "10000", NULL, g_MinNeedVerifyPreHashCount, "3");
                cout << "1vkS46QffeM4sDMBBjuJBiVkMQKY7Z8Tu------->>" << v.c_str() << endl;
            }
            else
            {
                CreateTx(mainnetstr.c_str(), v.c_str(), "10000", NULL, g_MinNeedVerifyPreHashCount, "3");
                cout << "16psRip78QvUruQr9fMzr8EomtFS1bVaXk------->>" << v.c_str() << endl;
            }
            usleep(13*1000*1000);
        } 
        else 
        {
            cout  << "\033[32m" << v.c_str() << "已分配资产" << "\033[1m"<< endl;
        }
    }
	cout << "初始化资产end==========" << endl;

	cout << "自动交易开始==========" << endl;
    thread first(test_pth, &sign);
    default_random_engine rand_list(time(0));
    string d_gas;
	for (int i = 0; i < 10000*10000; i++)
	{
        auto node_fee = net_get_node_ids_and_fees();
        if (node_fee.size() > 1) 
        {
            std::vector<uint64_t> node_fee_list;
            for (auto v : node_fee) 
            {
                node_fee_list.push_back(v.second);
            }

            sort(node_fee_list.begin(), node_fee_list.end());
            uint32_t fee_list_count = static_cast<uint32_t>(node_fee_list.size());

            // std::cout << "fee_list_count0--" << node_fee_list[0] << std::endl;
            // std::cout << "fee_list_count e--" << node_fee_list[fee_list_count-1] << std::endl;
            // std::cout << "fee_list_count" << fee_list_count << std::endl;
            //0.51 - 1
            uint32_t list_begin = fee_list_count * 0.51;
            uint32_t list_end = fee_list_count;

            list_begin = list_begin == 0 ? 1 : list_begin;
            list_end = list_end == 0 ? 1 : list_end;
            // cout << "51-------" << list_begin - 1 << endl;
            // cout << "100-------" << list_end - 1 << endl;
            //51 - 100 随机

            uniform_int_distribution<uint64_t> rand_engine_list(list_begin - 1, list_end - 1);
            // cout << "time " << time(0) << endl;

            auto gas_num = rand_engine_list(rand_list);
            uint64_t gas = node_fee_list[gas_num];
            d_gas = to_string((double)gas/DECIMAL_NUM);
            // cout << "gas_num-------" << gas_num << endl;
            // cout << "d_gas-------" << d_gas << endl;
        } 
        else 
        {
            cout << "全网燃料费节点不足一个!!!!!!!" << endl;
            usleep(i_sleeps);
            break;
        }
		if (sign == 'q') 
        {
			cout << "sign--" << sign << endl;
			break;
		}	
		int a1 = i % account_list.size();
		int a2 = (i + 1) % account_list.size();
		CreateTx(account_list[a1].c_str(), account_list[a2].c_str(), "1", NULL, g_MinNeedVerifyPreHashCount, d_gas);
		// CreateTx(account_list[a1].c_str(), account_list[a2].c_str(), "1", NULL, 3, "3");
		cout << account_list[a1] << " ---> " << account_list[a2] << " : " << 1 << endl;
        usleep(i_sleeps);
	}
	
    first.join();
    return;
}

void SendExitNode(const std::string id)
{	
	cout << "进入SendExitNode" << endl;
    
    TestSendExitNodeReq testSendExitNodeReq;
    testSendExitNodeReq.set_version( getVersion() );

    net_send_message<TestSendExitNodeReq>(id, testSendExitNodeReq);
}

void HandleExitNode( const std::shared_ptr<TestSendExitNodeReq>& msg, const MsgData& msgdata )
{
    debug("recv HandleExitNode!");

    // 判断版本是否兼容
	if( 0 != IsVersionCompatible( msg->version() ) )
	{
		error("HandleExitNode IsVersionCompatible");
		return ;
	}

    ExitGuardian();

    exit(0);
}

// 获取某个节点的最高块的高度和hash
void SendGetNodeHeightHashBase58AddrReq(const std::string id)
{
    TestGetNodeHeightHashBase58AddrReq testGetNodeHeightHashBase58AddrReq;
    testGetNodeHeightHashBase58AddrReq.set_version( getVersion() );

    std::string ownID = net_get_self_node_id();
    testGetNodeHeightHashBase58AddrReq.set_id( ownID );

    net_send_message<TestGetNodeHeightHashBase58AddrReq>(id, testGetNodeHeightHashBase58AddrReq);
}

void handleGetNodeHeightHashBase58AddrReq( const std::shared_ptr<TestGetNodeHeightHashBase58AddrReq>& msg, const MsgData& msgdata )
{
    debug("recv handleGetNodeHeightHashBase58AddrReq!");

    // 判断版本是否兼容
	if( 0 != IsVersionCompatible( msg->version() ) )
	{
		error("HandleExitNode IsVersionCompatible");
		return ;
	}

    // 版本
    TestGetNodeHeightHashBase58AddrAck testGetNodeHeightHashBase58AddrAck;
    testGetNodeHeightHashBase58AddrAck.set_version( getVersion() );

    std::string ownID = net_get_self_node_id();
    testGetNodeHeightHashBase58AddrAck.set_id( ownID );

    // 获取一个事务用于读取数据库
    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if( txn == NULL )
	{
		error("handleGetNodeHeightHashBase58AddrReq TransactionInit failed !");
		return ;
	}

    // 退出域销毁事务
    ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, true);
	};

    // 高度
    unsigned int blockHeight = 0;
    auto status = pRocksDb->GetBlockTop(txn, blockHeight);
    if( status != 0)
    {
		error("handleGetNodeHeightHashBase58AddrReq GetBlockTop failed !");
        return ;
    }
    testGetNodeHeightHashBase58AddrAck.set_height( blockHeight );

    // bestchain
    std::string blockHash;
    status = pRocksDb->GetBestChainHash(txn, blockHash);
    if( status != 0)
    {
		error("handleGetNodeHeightHashBase58AddrReq GetBestChainHash failed !");
        return ;
    }
    testGetNodeHeightHashBase58AddrAck.set_hash( blockHash );

    // base58地址
    testGetNodeHeightHashBase58AddrAck.set_base58addr( g_AccountInfo.DefaultKeyBs58Addr );

    net_send_message<TestGetNodeHeightHashBase58AddrAck>(msgdata, testGetNodeHeightHashBase58AddrAck);
}

void handleGetNodeHeightHashBase58AddrAck( const std::shared_ptr<TestGetNodeHeightHashBase58AddrAck>& msg, const MsgData& msgdata )
{
    // 版本判断
    if( 0 != IsVersionCompatible( msg->version() ) )
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

    // if (FromAddr == "q")
    // {
    // 	testExchangeInterface();
    // 	return;
    // } 
    // else if (FromAddr == "a") 
    // {
    //     testExchangeInterface2();
    //     return;
    // } 
    // else if (FromAddr == "w") 
    // {
    //     testExchangeInterface3();
    //     return;
    // } 
    // else if (FromAddr == "e") 
    // {
    //     testExchangeInterface4();
    //     return;
    // }

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
    cout << "请输入要质押的账号："<< endl;
    std::cin >> fromAddrstr;

    std::string pledge;
    cout << "请输入要质押金额："<< endl;
    std::cin >> pledge;

    std::string GasFee;
    cout << "请输入GasFee:" << endl;
    std::cin >> GasFee;

    std::string password;
    std::cout << "password" << endl;
    std::cin >> password;
    
    int r = CreatePledgeTransaction(fromAddrstr, pledge, g_MinNeedVerifyPreHashCount, GasFee, password);
    if (r == -104)
    {
        std::cout << "password error!!!" << std::endl;
    }
}

void handle_redeem_pledge()
{
    // 获取当前所有块的签名信息
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

    std::cout << "-- 目前已有质押 -- " << std::endl;
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

    if (utxos.size() > 0)
    {
        int r = CreateRedeemTransaction(addr, g_MinNeedVerifyPreHashCount, GasFee, utxo, password);
        if (r == -9)
        {
            std::cout << "password error!!!" << std::endl;
        }
    }
}

/**
A类地址：10.0.0.0 - 10.255.255.255 
B类地址：172.16.0.0 - 172.31.255.255
C类地址：192.168.0.0 -192.168.255.255 
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
    std::vector<Node> nodes = net_get_public_node();
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
            uint64_t amount = 0;
            if (SearchPledge(node.base58address, amount) != 0)
            {
                cout << "No pledge " << IpPort::ipsz(node.public_ip) << endl;
                continue ;
            }
            if (amount < g_TxNeedPledgeAmt)
            {
                cout << "Less mount of pledge " << IpPort::ipsz(node.public_ip) << endl;
                // 质押金额不足
                continue ;
            }

            if (node.fee > 0)
            {
                publicNodes.push_back(node);
                cout << "New public node ^^^^^VVVVV " << IpPort::ipsz(node.public_ip) << endl;
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
        cout << endl << "Update node is less than 5 ^^^^^VVVVV" << endl;
        return 1;
    }

    // std::vector<Node> newPublicNodes;
    // std::vector<node_info> configNodes = Singleton<Config>::get_instance()->GetNodeInfo_s();
    // for (auto& node : publicNodes)
    // {
    //     bool isNew = true;
    //     for (auto& configNode : configNodes)
    //     {
    //         string nodeIp(IpPort::ipsz(node.public_ip));
    //         if (nodeIp == configNode.ip && node.public_port == configNode.port)
    //         {
    //             isNew = false;
    //             break;
    //         }
    //     }
    //     if (isNew)
    //     {
    //         newPublicNodes.push_back(node);
    //     }
    // }
    // if (newPublicNodes.empty())
    // {
    //     return 0;
    // }

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
    Singleton<Config>::get_instance()->WriteFile();
    cout << "Node: " << publicNodes.size() << " update to config!" << endl;

    return 0;
}