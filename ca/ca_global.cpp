#include "ca_global.h"
#include "ca_coredefs.h"
#include "Crypto_ECDSA.h"
#include "ca_synchronization.h"
#include "../utils/time_task.h"
#include <mutex>

int g_testflag = 0;

std::string g_LinuxCompatible   = "1.0";
std::string g_WindowsCompatible = "1.0";
std::string g_IOSCompatible = "4.0.3";
std::string g_AndroidCompatible = "3.0.17";

char BLKDB_DATA_FILE[256];
char BLKDB_DATA_PATH[256];

struct chain_info g_chain_metadata = { CHAIN_BITCOIN, "bitcoin", PUBKEY_ADDRESS, SCRIPT_ADDRESS, "\xf9\xbe\xb4\xd9", "0x000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f" };
ECDSA<ECP, SHA1>::PrivateKey g_privateKey;
ECDSA<ECP, SHA1>::PublicKey g_publicKey;
accountinfo g_AccountInfo;
bool g_phone;
char g_ip[NETWORKID_LEN];

const int g_MinNeedVerifyPreHashCount = 6;  //最小共识数
int g_SyncDataCount = 500;
Sync* g_synch = new Sync();
std::vector<TestGetNodeHeightHashBase58AddrAck> g_nodeinfo;

const uint64_t g_minSignFee = 1000;
const uint64_t g_maxSignFee = 100000;


const char * build_time = "20200706-10:52";
string  build_commit_hash = "a018f71";

CTimer g_blockpool_timer; 
CTimer g_synch_timer; 
CTimer g_deviceonline_timer;
CTimer g_public_node_refresh_timer; // public node refresh to config
CTimer g_device2pubnet_timer;

int g_VerifyPasswordCount = 0;
int minutescount = 7200;
bool g_ready = false;

std::vector<string>   g_random_normal_node;
std::vector<string>   g_random_public_node;
std::mutex mu_return;
std::atomic_int64_t echo_counter = 5; 

uint64_t g_TxNeedPledgeAmt = 500000000;  // 500 * DECIMAL_NUM

uint64_t g_MaxAwardTotal = 14.025 * DECIMAL_NUM;

std::string g_InitAccount;

uint64_t g_minPledgeNodeNum = 10;

string getEbpcVersion()
{
    static string version = "1.1";
    return version;
}

string  getVersion()
{
    string versionNum = getEbpcVersion();
    std::ostringstream ss;
    ss << getSystem();
    std::string version = ss.str() + "_" + versionNum ; 
    if(g_testflag == 1)
    {
        g_InitAccount = "1vkS46QffeM4sDMBBjuJBiVkMQKY7Z8Tu";
        version = version + "_" + "t";
    }
    else 
    {
        g_InitAccount = "16psRip78QvUruQr9fMzr8EomtFS1bVaXk";
        version = version + "_" + "p";
    }
    return version;
}

Version getSystem()
{
#if WINDOWS
    return kWINDOWS;
#else
    return kLINUX;
#endif 
}
