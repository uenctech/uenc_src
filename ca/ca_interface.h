#ifndef _EBPC_CA_INTERFACE_H_
#define _EBPC_CA_INTERFACE_H_
#include "../include/cJSON.h"
#include "../net/msg_queue.h"

#include "ca_global.h"


using namespace std;
// Log in  Log out  Related 
/**
 * @description: initialization   Specify the path of the public and private key file 
 * @param path Initialization path 
 * @return: no 
 */
void interface_Init(const char *path);

/**
 * @description: Generate public and private key files for registration 
 * @param no 
 * @return: no 
 */
bool interface_GenerateKey();

// Wallet public and private key related 
/**
 * @description: Export mnemonic 
 * @param: bs58address Wallet address, if null is passed, the mnemonic phrase will be exported with the address set by interface_SetKey 
 * @param: out Output string 
 * @param: outLen Output string length 
 * @return: If the export is successful, the number of mnemonic words will be returned. Out is a string of mnemonic words separated by spaces. If it returns 0, the export failed. 
 */
int interface_GetMnemonic(const char *bs58address, char *out, int outLen);

/**
 * @description: Export private key 
 * @param: bs58address Wallet address, if null is passed, the private key will be exported with the address set by interface_SetKey 
 * @param: pridata Output string 
 * @param: iLen Output string length 
 * @return: Return true on success, false on failure 
 */
bool interface_GetPrivateKeyHex(const char *bs58address, char *pridata, unsigned int iLen);

/**
 * @description: Import private key 
 * @param: pridata Hexadecimal string of the private key 
 * @return: Return true on success, false on failure 
 */
bool interface_ImportPrivateKeyHex(const char *pridata);

/**
 * @description: Export keystore 
 * @param: bs58address Wallet address, if null is passed, the keystore will be exported with the address set by interface_SetKey 
 * @param: pass password 
 * @param: buf Output string 
 * @param: bufLen Output string length 
 * @return: Return true on success, false on failure 
 */
int  interface_GetKeyStore(const char *bs58address, const char *pass, char *buf, unsigned int bufLen);

/**
 * @description: Get wallet address 
 * @param  bs58address Wallet address 
 * @param iLen Wallet address length 
 * @mark The result of the function will be placed in the bs58address address 
 * @return: Return true on success, false on failure 
 */
bool interface_GetBase58Address(char *bs58address, unsigned int iLen);

/**
 * @description: Get the amount of the node master account 
 * @param bs58Address User wallet address 
 * @param amount Give money 
 * @param outdata Return data pointer 
 * @param outdataLen Return data length 
 * @mark The return data pointer out_data needs to be released by calling free 
 * @return: If the operation succeeds, it returns true, and if it fails, it returns false. 
 */
bool interface_ShowMeTheMoney(const char * bs58Address, double amount, char *outdata, size_t *outdataLen);

// misc
typedef enum emInterfaceClientType
{
    kInterfaceClientType_PC,
    kInterfaceClientType_iOS,
    kInterfaceClientType_Android,
}InterfaceClientType;

typedef enum emInterfaceClientLanguage
{
    kInterfaceClientLanguage_zh_CN,
    kInterfaceClientLanguage_en_US,

}InterfaceClientLanguage;

/**
 * @description: Callback function to get the current progress and total number in real time 
 * @param: ip The ip address being scanned 
 * @param: current The serial number of the current ip 
 * @param: total Total number of scanned ip 
 */
typedef void (* ScanPortProc)(const char * ip, unsigned int current, unsigned int total);

/**
 * @description: Get the IP list of the device (json format) 
 * @param: ip IP address of the mobile phone 
 * @param: mask Subnet mask 
 * @param: port Port number, (default 10091) 
 * @param: outdata Return the ip list in json format 
 * @param: outdatalen Returns the length of the ip list 
 * @param: spp Callback function to get the current progress and total number in real time 
 * @return: Return true if the operation is successful, otherwise false 
 */
bool interface_ScanPort(const char *ip, const char *mask, unsigned int port, char *outdata, size_t *outdatalen, ScanPortProc spp,int i,map<int64_t,string> &i_str);


void interface_NetMessage(const void *inData, void **outData, int *outDataLen);

int CreateTx(const char* From, const char * To, const char * amt, const char *ip, uint32_t needVerifyPreHashCount, std::string minerFees);

int free_buf(char **buf);

void interface_testdevice_mac();
// net_pack packTmp;






#endif