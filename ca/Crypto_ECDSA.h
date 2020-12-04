#ifndef __CRYPTO_ECDSA_H__
#define __CRYPTO_ECDSA_H__
#include <assert.h>
#include <iostream>
#include <map>
#include "ca_base58.h"
//using std::cout;
using std::endl;

#include <string>
using std::string;

#include "../crypto/cryptopp/osrng.h"
using CryptoPP::AutoSeededRandomPool;

#include "../crypto/cryptopp/aes.h"
using CryptoPP::AES;

#include "../crypto/cryptopp/integer.h"
using CryptoPP::Integer;

#include "../crypto/cryptopp/sha.h"
using CryptoPP::SHA1;

#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include "../crypto/cryptopp/md5.h"

#include "../crypto/cryptopp/filters.h"
using CryptoPP::StringSource;
using CryptoPP::StringSink;
using CryptoPP::ArraySink;
using CryptoPP::SignerFilter;
using CryptoPP::SignatureVerificationFilter;

#include "../crypto/cryptopp/files.h"
using CryptoPP::FileSource;
using CryptoPP::FileSink;

#include "../crypto/cryptopp/eccrypto.h"
using CryptoPP::ECDSA;
using CryptoPP::ECP;
using CryptoPP::DL_GroupParameters_EC;

#include "../crypto/cryptopp/oids.h"
using CryptoPP::OID;
#include "../crypto/cryptopp/hex.h"
using CryptoPP::HexDecoder;
using CryptoPP::HexEncoder;

bool GeneratePrivateKey( const OID& oid, ECDSA<ECP, SHA1>::PrivateKey& key );
bool GeneratePublicKey( const ECDSA<ECP, SHA1>::PrivateKey& privateKey, ECDSA<ECP, SHA1>::PublicKey& publicKey );

void SavePrivateKey( const string& filename, const ECDSA<ECP, SHA1>::PrivateKey& key );
void SavePublicKey( const string& filename, const ECDSA<ECP, SHA1>::PublicKey& key );
void LoadPrivateKey( const string& filename, ECDSA<ECP, SHA1>::PrivateKey& key );
void LoadPublicKey( const string& filename, ECDSA<ECP, SHA1>::PublicKey& key );

void GetPrivateKey(const ECDSA<ECP, SHA1>::PrivateKey& key, string& sPriStr );
void GetPublicKey(const ECDSA<ECP, SHA1>::PublicKey& key, string& sPubStr );

void SetPrivateKey(ECDSA<ECP, SHA1>::PrivateKey& key, const string& sPriStr);
void SetPublicKey(ECDSA<ECP, SHA1>::PublicKey& key, const string& sPubStr );

void PrintDomainParameters( const ECDSA<ECP, SHA1>::PrivateKey& key );
void PrintDomainParameters( const ECDSA<ECP, SHA1>::PublicKey& key );
void PrintDomainParameters( const DL_GroupParameters_EC<ECP>& params );
void PrintPrivateKey( const ECDSA<ECP, SHA1>::PrivateKey& key );
void PrintPublicKey( const ECDSA<ECP, SHA1>::PublicKey& key );

bool SignMessage( const ECDSA<ECP, SHA1>::PrivateKey& key, const string& message, string& signature );
bool VerifyMessage( const ECDSA<ECP, SHA1>::PublicKey& key, const string& message, const string& signature );

int test_Crypto_ECDSA();

class account
{
public:
    ECDSA<ECP, SHA1>::PrivateKey privateKey;
    ECDSA<ECP, SHA1>::PublicKey publicKey;
    std::string PrivateKeyName;
    std::string PublicKeyName;
    std::string sPriStr; 
    std::string sPubStr;
    std::string Bs58Addr;
};

class accountinfo
{
public:
	std::string path;
    account CurrentUser;
    std::string DefaultKeyBs58Addr;
    std::map <std::string, account> AccountList;
    bool GenerateKey();
    bool GenerateKey(const char *Bs58Addr, bool bFlag);
    bool DeleteAccount(const char *Bs58Addr);
    bool Sign(const char *Bs58Addr, const string& message, string& signature);
    bool GetPriKeyStr(const char *Bs58Addr, std::string &PriKey);
    bool GetPubKeyStr(const char *Bs58Addr, std::string &PubKey);
	bool SetKeyByBs58Addr(ECDSA<ECP, SHA1>::PrivateKey &privateKey, ECDSA<ECP, SHA1>::PublicKey &publicKey, const char *Bs58Addr);
	bool isExist(const char *Bs58Addr);
    bool FindKey(const char *Bs58Addr, account & acc);
	void print_AddrList();
    size_t GetAccSize(void);

	int  GetMnemonic(const char *Bs58Addr, char *out, int outLen);
	bool GenerateKeyFromMnemonic(const char *mnemonic);

    bool GetPrivateKeyHex(const char *Bs58Addr, char *pridata, unsigned int iLen);
	bool ImportPrivateKeyHex(const char *pridata);

    int  GetKeyStore(const char *Bs58Addr, const char *pass, char *buf, unsigned int bufLen);
	bool ImportKeyStore(const char *keystore, const char *pass);


    bool KeyFromPrivate(const char *pridata, int iLen);
	void ClearCurrentUser();    
private:
    std::map<std::string, account>::iterator iter;
    std::map<std::string, account>::iterator saveit;
};

#define PRIVATE "/1"
#define PUBLIC "/2"
int GeneratePairKey(const char *path);
int Encry(const char *path, const char *data, unsigned char **result, bool isHex);
int Decry(const char *path, const unsigned char *EncryData, int EncryDataLen, char **result, bool isHex);
std::string getsha1hash(std::string text);
std::string getsha256hash(std::string text);
std::string getMD5hash(std::string text);

#endif
