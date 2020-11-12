#ifndef __CA_GLOBAL_H__
#define __CA_GLOBAL_H__
#include "Crypto_ECDSA.h"
#include "../utils/time_task.h"
#include <unordered_set>

#include "proto/ca_protomsg.pb.h"

#define NETWORKID_LEN 129


class Sync;

extern std::string g_LinuxCompatible;
extern std::string g_WindowsCompatible;
extern std::string g_IOSCompatible;
extern std::string g_AndroidCompatible;

typedef enum CAVERSION
{
    kUnknown = 0,
    kLINUX   = 1,        
    kWINDOWS = 2,        
    kIOS     = 3,        
    kANDROID = 4,        
} Version;

extern struct chain_info g_chain_metadata;
extern ECDSA<ECP, SHA1>::PrivateKey g_privateKey;
extern ECDSA<ECP, SHA1>::PublicKey g_publicKey;
extern accountinfo g_AccountInfo;
extern bool g_phone;
extern char g_ip[NETWORKID_LEN];
extern const int g_MinNeedVerifyPreHashCount;
extern Sync* g_synch;
extern std::vector<TestGetNodeHeightHashBase58AddrAck> g_nodeinfo;

extern std::unordered_set<std::string> g_blockCacheList;

extern const char * build_time;
extern const char * build_commit_hash;
extern Timer g_blockpool_timer; 
extern Timer g_synch_timer; 
extern Timer g_deviceonline_timer;

extern int g_VerifyPasswordCount;
extern int minutescount ;
extern bool g_ready ;
extern std::mutex g_mu_tx;
extern int g_testflag;

extern const uint64_t g_minSignFee;
extern const uint64_t g_maxSignFee;

extern uint64_t g_TxNeedPledgeAmt;   
extern std::string g_InitAccount;    

extern uint64_t g_MaxAwardTotal;

extern uint64_t g_minPledgeNodeNum;   


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


#define TXTYPE_TX "tx"           
#define TXTYPE_PLEDGE "pledge"   
#define TXTYPE_REDEEM "redeem"   


#define PLEDGE_NET_LICENCE "netLicence"       



extern char BLKDB_DATA_FILE[256];


extern char BLKDB_DATA_PATH[256];

#define OWNER_CERT_PATH  "./cert/"
#define OWNER_CERT_DEFAULT  "default."

#define DECIMAL_NUM 1000000
#define PRE_BLOCK_VALID_HEIGHT 0
#define ORGIGINAL_BLOCK_VALID_HEIGHT 6
#define FIX_DOUBLE_MIN_PRECISION 0.0000005 

#define ENCOURAGE_TX 1

#define TXHASHDB_SWITCH true

#define PRIVATE_SUFFIX ".private.key"
#define PUBLIC_SUFFIX ".public.key"

#define COIN_BASE_TX_SIGN_HASH "EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE"
#define COIN_BASE_TX_SIGN_HASH_LEN 64

#define FEE_SIGN_STR "FEE                                                             "
#define FEE_SIGN_STR_LEN 64


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


#define VIRTUAL_ACCOUNT_PLEDGE "0000000000000000000000000000000000"
/* ====================================================================================  
 # @description:  获取版本号
 # @param  NONE
 # @return 返回版本
 # @Mark 返回结构为3部分组成，以下划线分割，第一部分为系统号，第二部分为版本号，第三部分为运行环境
 # 例如：1_0.3_t,
 # 系统号：1为linux，2为windows，3为iOS，4为Android
 # 版本号： 两级版本号如1.0
 # 运行环境：m为主网，t为测试网
 ==================================================================================== */
string getVersion();
/* ====================================================================================  
 # @description: 获取系统类型
 # @param  NONE
 # @return 返回为系统号
 # @mark
 # 系统号：1为linux，2为windows，3为iOS，4为Android
 ==================================================================================== */
Version getSystem();
#endif
