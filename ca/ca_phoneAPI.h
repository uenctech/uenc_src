#include "../crypto/cryptopp/default.h"

#include <assert.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <stdio.h>

#include "ca_base58.h"

using std::endl;

#include <string>
using std::string;

#include "../crypto/cryptopp/osrng.h"
using CryptoPP::AutoSeededRandomPool;

#include "../crypto/cryptopp/aes.h"
using CryptoPP::AES;

#include "../crypto/cryptopp/integer.h"
using CryptoPP::Integer;

#include "../crypto/cryptopp/modes.h"

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

namespace ca_phoneAPI {
    void LoadPrivateKey(const string& filename, ECDSA<ECP, SHA1>::PrivateKey& key);

    void GetPrivateKey(const ECDSA<ECP, SHA1>::PrivateKey& key, string& sPriStr);

    void GetPublicKey(const ECDSA<ECP, SHA1>::PublicKey& key, string& sPubStr );

    bool GeneratePublicKey( const ECDSA<ECP, SHA1>::PrivateKey& privateKey, ECDSA<ECP, SHA1>::PublicKey& publicKey );

    bool GeneratePrivateKey( const OID& oid, ECDSA<ECP, SHA1>::PrivateKey& key);

    bool SignMessage( const ECDSA<ECP, SHA1>::PrivateKey& key, const string& message, string& signature );

    bool VerifyMessage( const ECDSA<ECP, SHA1>::PublicKey& key, const string& message, const string& signature );

    int genPairKey(char *out_pri_key, int *out_pri_len, char *out_pub_key, int *out_pub_len);

    int genBs58Addr(const char *pub_key, const int pub_len, char *bs58_addr, int *bs58_len, int ver);

    int CPPgenPairKey(std::string &out_pri_key, std::string &out_pub_key);

    void SetPrivateKey(ECDSA<ECP, SHA1>::PrivateKey& key, const string& sPriStr);

    int setPriKey(const string& sPriStr);

    /* aes加解密 */
    void initKV();
    string encrypt(string plainText);
    string decrypt(string cipherTextHex);
    void te();
}


