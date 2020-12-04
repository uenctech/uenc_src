#include "ca_phoneAPI.h"
using namespace std;




namespace ca_phoneAPI {

CryptoPP::byte key[CryptoPP::AES::DEFAULT_KEYLENGTH], iv[ CryptoPP::AES::BLOCKSIZE];

void initKV() {
    memset(key, 0x00, CryptoPP::AES::DEFAULT_KEYLENGTH);
    memset(iv, 0x00, CryptoPP::AES::BLOCKSIZE);
}
 
string encrypt(string plainText) {
    string cipherText;

    CryptoPP::AES::Encryption aesEncryption(key, CryptoPP::AES::DEFAULT_KEYLENGTH);
    CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryption(aesEncryption, iv);
    CryptoPP::StreamTransformationFilter stfEncryptor(cbcEncryption, new CryptoPP::StringSink(cipherText));
    stfEncryptor.Put(reinterpret_cast<const unsigned char*>(plainText.c_str()), plainText.length() + 1);
    stfEncryptor.MessageEnd();
 
    string cipherTextHex;
    for (int i = 0; i < (int)cipherText.size(); i++) {
        char ch[3] = {0};
        sprintf(ch, "%02x",  static_cast<CryptoPP::byte>(cipherText[i]));
        cipherTextHex += ch;
    }
 
    return cipherTextHex;
}

string decrypt(string cipherTextHex) {
    string cipherText;
    string decryptedText;
    int i = 0;
    while (true) {
        char c;
        int x;
        stringstream ss;
        ss << hex << cipherTextHex.substr(i, 2).c_str();
        ss >> x;
        c = (char)x;
        cipherText += c;
        if(i >= (int)cipherTextHex.length() - 2) break;
        i += 2;
    }

    CryptoPP::AES::Decryption aesDecryption(key, CryptoPP::AES::DEFAULT_KEYLENGTH);
    CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption(aesDecryption, iv);
    CryptoPP::StreamTransformationFilter stfDecryptor(cbcDecryption, new CryptoPP::StringSink(decryptedText));
    stfDecryptor.Put(reinterpret_cast<const unsigned char*>( cipherText.c_str() ), cipherText.size());
 
    stfDecryptor.MessageEnd();
 
    return decryptedText;
}

void te() {
    string text = "cccnnnd";
    cout << "text : " << text << endl;
 
    initKV();
    string cipherHex = encrypt(text);
    cout << "encrypt : " << cipherHex << endl;

    string dec_str = decrypt(cipherHex);
    cout << "decrypt : " << dec_str << endl;
}



void LoadPrivateKey(const string& filename, ECDSA<ECP, SHA1>::PrivateKey& key) {
    key.Load( FileSource( filename.c_str(), true ).Ref() );
}

void GetPrivateKey(const ECDSA<ECP, SHA1>::PrivateKey& key, string& sPriStr) {
    const Integer& x = key.GetPrivateExponent();

    for(int i = x.ByteCount() - 1; i >= 0; i--) {
        sPriStr += x.GetByte(i);
    }
}

void GetPublicKey(const ECDSA<ECP, SHA1>::PublicKey& key, string& sPubStr ) {
    const ECP::Point& q = key.GetPublicElement();
    const Integer& qx = q.x;
    const Integer& qy = q.y;

	char c1 = qx.ByteCount() ;
    char c2 = qy.ByteCount() ;
    sPubStr += c1;
    sPubStr += c2;
    for(int i = qx.ByteCount() - 1; i >= 0; i--)
    {
        sPubStr += qx.GetByte(i);
    }
    for(int i = qy.ByteCount() - 1; i >= 0; i--)
    {
        sPubStr += qy.GetByte(i);
    }
}

bool GeneratePublicKey( const ECDSA<ECP, SHA1>::PrivateKey& privateKey, ECDSA<ECP, SHA1>::PublicKey& publicKey ) {
    AutoSeededRandomPool prng;

    assert( privateKey.Validate( prng, 3 ) );

    privateKey.MakePublicKey(publicKey);
    assert( publicKey.Validate( prng, 3 ) );

    return publicKey.Validate( prng, 3 );
}

bool GeneratePrivateKey( const OID& oid, ECDSA<ECP, SHA1>::PrivateKey& key) {
    AutoSeededRandomPool prng;

    key.Initialize( prng, oid );
    assert( key.Validate( prng, 3 ) );
     
    return key.Validate( prng, 3 );
}

bool SignMessage( const ECDSA<ECP, SHA1>::PrivateKey& key, const string& message, string& signature ) {
    AutoSeededRandomPool prng;
    
    signature.erase();    

    StringSource( message, true, new SignerFilter( prng, ECDSA<ECP,SHA1>::Signer(key), new StringSink( signature )) ); 
    
    return !signature.empty();
}

bool VerifyMessage( const ECDSA<ECP, SHA1>::PublicKey& key, const string& message, const string& signature ) {
    bool result = false;

    StringSource( signature+message, true, new SignatureVerificationFilter( ECDSA<ECP,SHA1>::Verifier(key), new ArraySink( (CryptoPP::byte*)&result, sizeof(result) ))); 

    return result;
}

void SetPrivateKey(ECDSA<ECP, SHA1>::PrivateKey& key, const string& sPriStr)
{
    HexDecoder decoder;
    decoder.Put((CryptoPP::byte*)&sPriStr[0], sPriStr.size());
    decoder.MessageEnd();
    
    Integer x;
    x.Decode(decoder, decoder.MaxRetrievable());

    key.Initialize(CryptoPP::ASN1::secp256r1(), x);
}

int setPriKey(std::string& sPriStr) {
    ECDSA<ECP, SHA1>::PrivateKey pri_key;

    SetPrivateKey(pri_key, sPriStr);


    return 1;
}

/* 生成公私钥和bs58地址 参数返回 */
int genPairKey(char *out_pri_key, int *out_pri_len, char *out_pub_key, int *out_pub_len) {
    ECDSA<ECP, SHA1>::PrivateKey pri_key;
    GeneratePrivateKey(CryptoPP::ASN1::secp256r1(), pri_key);

    ECDSA<ECP, SHA1>::PublicKey pub_key;
    GeneratePublicKey(pri_key, pub_key);

    string str_pri;
    GetPrivateKey(pri_key, str_pri);
    memcpy(out_pri_key, str_pri.c_str(), str_pri.size());
    *out_pri_len = str_pri.size();

    string str_pub;
    GetPublicKey(pub_key, str_pub);
    memcpy(out_pub_key, str_pub.c_str(), str_pub.size());
    *out_pub_len = strlen(out_pub_key);

// std::cout << "str_pub " << str_pub.size() << " out_pub_len " << *out_pub_len << std::endl;

    return 0;
}

int genBs58Addr(const char *pub_key, const int pub_len, char *bs58_addr, int *bs58_len, int ver) {
    char buf[2048] = {0};
    size_t buf_len = sizeof(buf);

    GetBase58Addr(buf, &buf_len, uint8_t(ver), pub_key, pub_len);
    memcpy(bs58_addr, buf, buf_len - 1);
    *bs58_len = buf_len - 1;

// std::cout << "bs58 " << buf << " bs58_len " << *bs58_len << std::endl;

    return 0;
}


int CPPgenPairKey(std::string &out_pri_key, std::string &out_pub_key) {
    ECDSA<ECP, SHA1>::PrivateKey pri_key;
    GeneratePrivateKey(CryptoPP::ASN1::secp256r1(), pri_key);

    ECDSA<ECP, SHA1>::PublicKey pub_key;
    GeneratePublicKey(pri_key, pub_key);

    string str_pri;
    GetPrivateKey(pri_key, str_pri);
    out_pri_key = str_pri;

    string str_pub;
    GetPublicKey(pub_key, str_pub);
    out_pub_key = str_pub;

    return 0;
}

}
