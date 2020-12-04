#include <iostream>
#include <string>
#include <cstdlib>
#include "./crypto_util.h"
#include "../crypto/cryptopp/aes.h"
#include "../crypto/cryptopp/hex.h"
#include "../crypto/cryptopp/files.h"
#include "../crypto/cryptopp/default.h"
#include "../crypto/cryptopp/filters.h"
#include "../crypto/cryptopp/osrng.h"
#include "../include/ADVobfuscator/Log.h"
#include "../include/ADVobfuscator/MetaString.h"

using namespace CryptoPP;
using namespace andrivet::ADVobfuscator;

std::string CryptoUtil::aesIV = OBFUSCATED("1k3cmv1b9h8we1n3");
std::string CryptoUtil::encryptAes(std::string sKey, std::string sIV, const char *plainText)
{
    std::string outstr;
   
    //填key    
    SecByteBlock key(AES::MAX_KEYLENGTH);
    memset(key, 0x30, key.size());
    sKey.size() <= AES::MAX_KEYLENGTH ? memcpy(key, sKey.c_str(), sKey.size()) : memcpy(key, sKey.c_str(), AES::MAX_KEYLENGTH);
    //填iv    c++17 waring可能和std::byte冲突 
    byte iv[AES::BLOCKSIZE];
    memset(iv, 0x30, AES::BLOCKSIZE);
    sIV.size() <= AES::BLOCKSIZE ? memcpy(iv, sIV.c_str(), sIV.size()) : memcpy(iv, sIV.c_str(), AES::BLOCKSIZE);

    AES::Encryption aesEncryption((byte *)key, AES::MAX_KEYLENGTH);

    CBC_CTS_Mode_ExternalCipher::Encryption cbcctsEncryption(aesEncryption, iv);

    StreamTransformationFilter cbcctsEncryptor(cbcctsEncryption, new HexEncoder(new StringSink(outstr)));
    cbcctsEncryptor.Put((byte *)plainText, strlen(plainText));
    cbcctsEncryptor.MessageEnd();

    return outstr;
}

std::string CryptoUtil::decryptAes(std::string sKey, std::string sIV, const char *cipherText)
{
    std::string outstr;

    //填key    
    SecByteBlock key(AES::MAX_KEYLENGTH);
    memset(key, 0x30, key.size());
    sKey.size() <= AES::MAX_KEYLENGTH ? memcpy(key, sKey.c_str(), sKey.size()) : memcpy(key, sKey.c_str(), AES::MAX_KEYLENGTH);

    //填iv    
    byte iv[AES::BLOCKSIZE];
    memset(iv, 0x30, AES::BLOCKSIZE);
    sIV.size() <= AES::BLOCKSIZE ? memcpy(iv, sIV.c_str(), sIV.size()) : memcpy(iv, sIV.c_str(), AES::BLOCKSIZE);


    CBC_CTS_Mode<AES >::Decryption cbcctsDecryption((byte *)key, AES::MAX_KEYLENGTH, iv);

    HexDecoder decryptor(new StreamTransformationFilter(cbcctsDecryption, new StringSink(outstr)));
    decryptor.Put((byte *)cipherText, strlen(cipherText));
    decryptor.MessageEnd();

    return outstr;
}


std::string CryptoUtil::getAesKey()
{
	std::string text = OBFUSCATED("E969F240AE43C9CF7BDF8093A7DC7C05AF");
    std::string hash;
	SHA256 sha256;
	HashFilter hashfilter(sha256);
	hashfilter.Attach(new HexEncoder(new StringSink(hash), false));
	hashfilter.Put(reinterpret_cast<const unsigned char*>(text.c_str()), text.length());
	hashfilter.MessageEnd();
	return hash;
}