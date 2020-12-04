#ifndef _CRYPTOUTIL_H_
#define _CRYPTOUTIL_H_


enum AESKeyLength
{
    AES_KEY_LENGTH_16 = 16, AES_KEY_LENGTH_24 = 24, AES_KEY_LENGTH_32 = 32  
};

class CryptoUtil
{
    
public:
    CryptoUtil() = default;
    ~CryptoUtil() = default;
    static std::string aesIV;//128 bits  16位
	static std::string encryptAes(std::string sKey, std::string sIV, const char *plainText);
    static std::string decryptAes(std::string sKey, std::string sIV, const char *cipherText);
    static std::string getAesKey();

};


#endif