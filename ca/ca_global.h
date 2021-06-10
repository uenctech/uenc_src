#ifndef __CA_GLOBAL_H__
#define __CA_GLOBAL_H__
#include "Crypto_ECDSA.h"
#include <unordered_set>

#include "../common/global.h"
#include "proto/ca_protomsg.pb.h"
#include "../utils/CTimer.hpp"
#include "./net/peer_node.h"
#include "ca_txconfirmtimer.h"

#define NETWORKID_LEN 129

class Sync;

extern const int64_t g_compatMinHeight;

extern struct chain_info g_chain_metadata;
extern ECDSA<ECP, SHA1>::PrivateKey g_privateKey;
extern ECDSA<ECP, SHA1>::PublicKey g_publicKey;
extern accountinfo g_AccountInfo;
extern char g_ip[NETWORKID_LEN];
extern const int g_MinNeedVerifyPreHashCount;
extern Sync* g_synch;
extern std::vector<GetDevInfoAck> g_nodeinfo;

extern const char * build_time;
extern string build_commit_hash;
extern CTimer g_blockpool_timer; 
extern CTimer g_synch_timer; 
extern CTimer g_deviceonline_timer;
extern CTimer g_public_node_refresh_timer;

extern std::vector<Node> g_localnode;

extern std::mutex mu_return;
extern std::atomic_int64_t echo_counter ; 

extern const uint64_t g_minSignFee;
extern const uint64_t g_maxSignFee;

extern uint64_t g_TxNeedPledgeAmt;   // The amount of pledged assets required for account transactions 
extern std::string g_InitAccount;    // Initial account 

extern uint64_t g_MaxAwardTotal;

extern uint64_t g_minPledgeNodeNum;   // The minimum number of pledges in the entire network, after reaching this number, ordinary nodes can no longer sign pledge transactions 

extern std::mutex rollbackLock;
extern int rollbackCount;

extern TransactionConfirmTimer g_RpcTransactionConfirm;

#ifdef __cplusplus
extern "C" {
#endif

#define TX "tx"
#define BUILDBLOCK "buildblock"
#define VERIFYBLOCK "verifyblock"
#define REQBLOCK "req"
#define RESBLOCK "res"

#define GETBLOCK "getblock"
#define GETTX "gettx"


#define CREATETXINFO "cttxinfo"
#define REQTXRAW "reqtx"
#define PRETXRAW "pretx"

#define GETTXINFO "gettxinfo"
#define GETAMT "getamt"
#define GETMONEY "getmoney"
#define GETCLIENTINFO "getCntInfo"
#define GETDEVPRIKEY "getdevpri"
#define CHANGEDEVPRIKEY "chgdevpri"
#define UPDATEREQUEST "update"
#define GETBLKINFO "getblkinfo"
#define GETBLKHASH "getblkhash"
#define BESTINFO "bestinfo"
#define GETDEVINFO "getdevinfo"

// Transaction Type 
#define TXTYPE_TX "tx"           // Normal transaction 
#define TXTYPE_PLEDGE "pledge"   // Pledge transaction 
#define TXTYPE_REDEEM "redeem"   // Depledge transaction 

// Pledge transaction type 
#define PLEDGE_NET_LICENCE "netLicence"       //Into the network pledge 


//#define BLKDB_DATA_FILE "./data/blk.seq"
extern char BLKDB_DATA_FILE[256];

//#define BLKDB_DATA_PATH "./data"
extern char BLKDB_DATA_PATH[256];

#define OWNER_CERT_PATH  "./cert/"
#define OWNER_CERT_DEFAULT  "default."

#define DECIMAL_NUM 1000000
#define PRE_BLOCK_VALID_HEIGHT 0
#define ORGIGINAL_BLOCK_VALID_HEIGHT 6
#define FIX_DOUBLE_MIN_PRECISION 0.0000005 // Revised value for 6 digits after the decimal point 

#define ENCOURAGE_TX 1

#define TXHASHDB_SWITCH true

#define PRIVATE_SUFFIX ".private.key"
#define PUBLIC_SUFFIX ".public.key"

#define COIN_BASE_TX_SIGN_HASH "EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE"
#define COIN_BASE_TX_SIGN_HASH_LEN 64

#define FEE_SIGN_STR "FEE                                                             "
#define FEE_SIGN_STR_LEN 64

//Additional rewards 
#define EXTRA_AWARD_SIGN_STR "AWARD                                                           "
#define EXTRA_AWARD_SIGN_STR_LEN 64

#define GENESIS_BASE_TX_SIGN "GENESIS                                                         "
#define GENESIS_BASE_TX_SIGN_LEN 64

#define OTHER_COIN_BASE_TX_SIGN "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB"
#define OTHER_COIN_BASE_TX_SIGN_LEN 64

#define BASE_BLOCK_DATA "f9beb4d972657300000000000000000037010000407a8032fd3401f9beb4d9766572696679626c6f636b001c0100001bd49de300000000000000000000000000000000000000000000000000000000000000000000000015bfeec8724d3465cf77ca7e455c502670aaec08f936e2d63d24b90d740621024ed5e264f292050000000000000000000101000000010000000000000000000000000000000000000000000000000000000000000000ffffffff4043434343434343434343434343434343434343434343434343434343434343434343434343434343434343434343434343434343434343434343434343434343ffffffff0100b08ef01b000000223136707352697037385176557275517239664d7a7238456f6d74465331625661586b0f32845d00223136707352697037385176557275517239664d7a7238456f6d74465331625661586b0000000000"

#ifdef __cplusplus
}
#endif

// Special trading virtual account 
#define VIRTUAL_ACCOUNT_PLEDGE "0000000000000000000000000000000000"



#endif
