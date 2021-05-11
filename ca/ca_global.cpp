/*
 * @Author: your name
 * @Date: 2021-01-21 14:54:56
 * @LastEditTime: 2021-03-27 14:29:22
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \ebpc\ca\ca_global.cpp
 */
#include "ca_global.h"
#include "ca_coredefs.h"
#include "Crypto_ECDSA.h"
#include "ca_synchronization.h"
#include <mutex>
#include "../common/version.h"


const int64_t g_compatMinHeight = 50000;

char BLKDB_DATA_FILE[256];
char BLKDB_DATA_PATH[256];

struct chain_info g_chain_metadata = { CHAIN_BITCOIN, "bitcoin", PUBKEY_ADDRESS, SCRIPT_ADDRESS, "\xf9\xbe\xb4\xd9", "0x000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f" };
ECDSA<ECP, SHA1>::PrivateKey g_privateKey;
ECDSA<ECP, SHA1>::PublicKey g_publicKey;
accountinfo g_AccountInfo;
char g_ip[NETWORKID_LEN];

const int g_MinNeedVerifyPreHashCount = 6;  //最小共识数
int g_SyncDataCount = 500;
Sync* g_synch = new Sync();
std::vector<GetDevInfoAck> g_nodeinfo;

const uint64_t g_minSignFee = 1000;
const uint64_t g_maxSignFee = 100000;

const char * build_time = "20200706-10:52";
string  build_commit_hash = "a018f71";

CTimer g_blockpool_timer; 
CTimer g_synch_timer; 
CTimer g_deviceonline_timer;
CTimer g_public_node_refresh_timer; // public node refresh to config


std::vector<Node> g_localnode;
//bool g_proxy;
std::mutex mu_return;
std::atomic_int64_t echo_counter = 5; 

uint64_t g_TxNeedPledgeAmt = 500000000;  // 500 * DECIMAL_NUM

uint64_t g_MaxAwardTotal = 14.025 * DECIMAL_NUM;

std::string g_InitAccount;

uint64_t g_minPledgeNodeNum = 10;

std::mutex rollbackLock;
int rollbackCount = 0;
TransactionConfirmTimer g_RpcTransactionConfirm;

