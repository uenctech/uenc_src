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
#include "ca_txvincache.h"
#include "ca_txfailurecache.h"

const char *kVersion = "1.0.1";
std::shared_mutex g_NodeInfoLock;

//test--------
void testExchangeInterface(); //q 配置文件自动化随机交易测试
void testExchangeInterface2(); //a 配置文件自动化交易双向测试
void testExchangeInterface3(); //w 智能自动化交易测试
void testExchangeInterface4(); //e 并发交易测试
int testInterfaceConfig(std::map<int, char*>&, int&, int&);
const char *TEST_JSON_NAME = "test_json_interface.json";

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

    MagicSingleton<TxVinCache>::GetInstance()->Process();

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

// 获取某个节点的最高块的高度和hash
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

    // 判断版本是否兼容
	if( 0 != IsVersionCompatible( msg->version() ) )
	{
		error("HandleExitNode IsVersionCompatible");
		return ;
	}

    // 版本
    GetDevInfoAck getDevInfoAck;
    getDevInfoAck.set_version( getVersion() );

    std::string ownID = net_get_self_node_id();
    getDevInfoAck.set_id( ownID );

    // 获取一个事务用于读取数据库
    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if( txn == NULL )
	{
		error("HandleGetDevInfoReq TransactionInit failed !");
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

    // base58地址
    getDevInfoAck.set_base58addr( g_AccountInfo.DefaultKeyBs58Addr );

    net_send_message<GetDevInfoAck>(msg->id(), getDevInfoAck);
}

void HandleGetDevInfoAck( const std::shared_ptr<GetDevInfoAck>& msg, const MsgData& msgdata )
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
    NodeSort sortFuc;
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
                    std::sort(publicNodes.begin(), publicNodes.end(),sortFuc);
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