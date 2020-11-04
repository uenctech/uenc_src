#include "ca_global.h"
#include "ca_coredefs.h"
#include "Crypto_ECDSA.h"
#include "ca_synchronization.h"
#include "../utils/time_task.h"
#include <mutex>

int g_testflag = 1;

std::string g_LinuxCompatible   = "0.3";
std::string g_WindowsCompatible = "0.3";
std::string g_IOSCompatible = "3.0.14";
std::string g_AndroidCompatible = "3.0.15";

char BLKDB_DATA_FILE[256];
char BLKDB_DATA_PATH[256];

struct chain_info g_chain_metadata = { CHAIN_BITCOIN, "bitcoin", PUBKEY_ADDRESS, SCRIPT_ADDRESS, "\xf9\xbe\xb4\xd9", "0x000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f" };
ECDSA<ECP, SHA1>::PrivateKey g_privateKey;
ECDSA<ECP, SHA1>::PublicKey g_publicKey;
accountinfo g_AccountInfo;
bool g_phone;
char g_ip[NETWORKID_LEN];

int g_MinNeedVerifyPreHashCount = 3;  
int g_SyncDataCount = 500;
Sync* g_synch = new Sync();
std::vector<TestGetNodeHeightHashBase58AddrAck> g_nodeinfo;

std::unordered_set<std::string> g_blockCacheList;

const char * build_time = "20200706-10:52";
const char * build_commit_hash = "8d939e9";

Timer g_blockpool_timer; 
Timer g_synch_timer; 
Timer g_deviceonline_timer;
std::mutex g_mu_tx;

int g_VerifyPasswordCount = 0;
int minutescount = 7200;
bool g_ready = false;

uint64_t g_TxNeedPledgeAmt = 500000000;  

std::string g_InitAccount;

uint64_t g_minPledgeNodeNum = 10;

string  getVersion()
{
    string versionNum = "0.3";
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
        version = version + "_" + "m";
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
