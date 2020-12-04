#include "Crypto_ECDSA.h"
#include "ca_global.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "ca_bip39.h"
#include "ca_pbkdf2.h"
#include "../crypto/cryptopp/default.h" 
#include "../crypto/cryptopp/cryptlib.h"

#include "ca_sha3_256.h"
#include "../include/cJSON.h"
#include "ca_uuid4.h"
#include "ca_hexcode.h"
#include "../include/logging.h"
#include <iostream>
using std::cout;
using std::endl;

#include <utility>
using std::make_pair;
#include <string>
using std::string;
using namespace CryptoPP;

std::mutex SetKeyLock;        // SetKeyByBs58Addr时防止竞争

int test_Crypto_ECDSA()
{
#if 0
    // Scratch result
    bool result = false;   
    
    // Private and Public keys
    ECDSA<ECP, SHA1>::PrivateKey privateKey;
    ECDSA<ECP, SHA1>::PublicKey publicKey;
    
    /////////////////////////////////////////////
    // Generate Keys
#endif
#if 0
    result = GeneratePrivateKey( CryptoPP::ASN1::secp256r1(), privateKey );
    assert( true == result );
    if( !result ) { return -1; }

    result = GeneratePublicKey( privateKey, publicKey );
    assert( true == result );
    if( !result ) { return -2; }
#endif
#if 1

    unsigned char *p;
	bool isHex = true;
    GeneratePairKey("./why");
    int len = Encry("./why", "Yoda said, Do or do not. There is no try. ------------================", &p, isHex);
    for(int i = 0; i < len; i++)
    {
		if(isHex)
			printf("%c", p[i]);
		else
			printf("%02x", p[i]);
    }
    printf("\n");
    char *t;
    len = Decry("./why", p, len, &t, isHex);
    info("len:%d  [%s]\n", len, t);
    free(p);
    free(t);
#endif
#if 0
    CryptoPP::ECIES <CryptoPP::ECP>::Encryptor Encryptor (publicKey);
    CryptoPP::ECIES <CryptoPP::ECP>::Decryptor Decryptor (privateKey);

    std::string plainText = "Yoda said, Do or do not. There is no try.";
    size_t plainTextLength = plainText.length () + 1;
    size_t cipherTextLength = Encryptor.CiphertextLength (plainTextLength);
	cout << "Cipher text length is ";
	cout << cipherTextLength << endl;
    CryptoPP::byte * cipherText = new CryptoPP::byte[cipherTextLength];
    memset (cipherText, 0x00, cipherTextLength);

    CryptoPP::AutoSeededRandomPool rng;
    Encryptor.Encrypt (rng, reinterpret_cast < const CryptoPP::byte * > (plainText.data ()), plainTextLength, cipherText);

    for(unsigned int i = 0; i < cipherTextLength; i++)
    {
        printf("%02x", (unsigned char)cipherText[i]);
    }
	printf("\n");
    size_t recoveredTextLength = Decryptor.MaxPlaintextLength (cipherTextLength);
    char * recoveredText = new char[recoveredTextLength];
    memset (recoveredText, 0xBB, recoveredTextLength);
    Decryptor.Decrypt (rng, cipherText, cipherTextLength, reinterpret_cast <CryptoPP::byte * >(recoveredText));
    cout << "Recovered text: " << recoveredText << endl;
    cout << "Recovered text length is " << recoveredTextLength << endl;
#endif
    
    /////////////////////////////////////////////
#if 0
    string sPriStr, sPubStr;
    GetPrivateKey(privateKey, sPriStr);
    GetPublicKey(publicKey, sPubStr);
    for(int i = 0; i < sPriStr.length(); i++)
    {
        printf("%02x", ((unsigned char *)sPriStr.c_str())[i]);
    }
    printf("\n");
    for(int i = 0; i < sPubStr.length(); i++)
    {
        printf("%02x", ((unsigned char *)sPubStr.c_str())[i]);
    }
#endif    

    /////////////////////////////////////////////
    // Save key in PKCS#9 and X.509 format    
    //SavePrivateKey( "ec.private.key", privateKey );
    //SavePublicKey( "ec.public.key", publicKey );
    
    /////////////////////////////////////////////
    // Load key in PKCS#9 and X.509 format     
    //LoadPrivateKey( "ec.private.key", privateKey );
    //LoadPublicKey( "ec.public.key", publicKey );

    /////////////////////////////////////////////
    // Print Domain Parameters and Keys    
    //PrintDomainParameters( publicKey );
    //PrintPrivateKey( privateKey );
    //PrintPublicKey( publicKey );
        
#if 0
    /////////////////////////////////////////////
    string sPriStr1 = "83efac60f5589022a149a751ac2cdfc57272279bf7bdd1031a6ddc4a988893a8"; 
    string sPubStr1 = "eba6efd665c539d64d2185faedf34482e2bdc9d9d610af2bdd0c3a22e7293b416e657f124af3b7bea690471e6b66582a72bfcf01b714c5f6840513c41818e92a";
    SetPrivateKey(privateKey, sPriStr1);
    SetPublicKey(publicKey, sPubStr1);
#endif
#if 0

    /////////////////////////////////////////////
    // Sign and Verify a message      
    string message = "Yoda said, Do or do not. There is no try.";
    string signature;
#endif

#if 0
    result = SignMessage( privateKey, message, signature );
    assert( true == result );

    printf("signature\n");
    for(unsigned int i = 0; i < signature.size(); i++)
    {
        printf("%02x", ((unsigned char *)signature.c_str())[i]);
    }
    result = VerifyMessage( publicKey, message, signature );
    assert( true == result );

    std::cout<<std::endl;
#endif

#if 0
	accountinfo t;
	t.GenerateKey("15B7Mfj9HzQHvyZ28v5563hL2b55Wjorp4", false);
	t.GenerateKey("1Cx6zKhpEuwnRzW5EzV3uHqXYakDHWPpvJ", false);
	t.GenerateKey("1FQTiUz1qgELTWSGS8vwf3DYzcXiWki1BA", false);
	printf("%zu\n", t.AccountList.size());
	result = t.Sign("1FQTiUz1qgELTWSGS8vwf3DYzcXiWki1BA", message, signature);
    printf("signature\n");
    for(unsigned int i = 0; i < signature.size(); i++)
    {
        printf("%02x", ((unsigned char *)signature.c_str())[i]);
    }
    result = VerifyMessage( t.CurrentUser.publicKey, message, signature );
    assert( true == result );
#endif
    return 0;
}

int Decry(const char *path, const unsigned char *EncryData, int EncryDataLen, char **result, bool isHex)
{
    if(NULL == path || NULL == EncryData)
    {
        return -1;
    }
    ECDSA<ECP, SHA1>::PrivateKey privateKey;
    ECDSA<ECP, SHA1>::PublicKey publicKey;

    std::string sPri = path;
    sPri += PRIVATE;
    std::string sPub = path;
    sPub += PUBLIC;
    LoadPrivateKey( sPri.c_str(), privateKey );
    LoadPublicKey( sPub.c_str(), publicKey );

    CryptoPP::ECIES <CryptoPP::ECP>::Encryptor Encryptor (publicKey);
    CryptoPP::ECIES <CryptoPP::ECP>::Decryptor Decryptor (privateKey);
    CryptoPP::AutoSeededRandomPool rng;
	CryptoPP::byte *pEncryData = NULL;
	size_t pEncryDataLen = 0;
	unsigned char *sbuf = NULL;
	
	if(isHex)
	{
		int iLen = EncryDataLen/2;
		sbuf = (unsigned char *)malloc(iLen);
		memset(sbuf, 0x00 , iLen);
		decode_hex(sbuf, iLen, (char *)EncryData, &pEncryDataLen);
		pEncryData = (CryptoPP::byte *)sbuf;
	}
	else
	{
		pEncryData = (CryptoPP::byte *)EncryData;
		pEncryDataLen = EncryDataLen;
	}

    size_t recoveredTextLength = Decryptor.MaxPlaintextLength (pEncryDataLen);
    char * recoveredText = new char[recoveredTextLength];
    memset (recoveredText, 0xBB, recoveredTextLength);
    Decryptor.Decrypt (rng, (CryptoPP::byte *)pEncryData, pEncryDataLen, reinterpret_cast <CryptoPP::byte * >(recoveredText));
	free(sbuf);

    *result = recoveredText;
    return recoveredTextLength;
}

int Encry(const char *path, const char *data, unsigned char **result, bool isHex)
{
    if(NULL == path || NULL == data)
    {
        return -1;
    }
    ECDSA<ECP, SHA1>::PrivateKey privateKey;
    ECDSA<ECP, SHA1>::PublicKey publicKey;

    std::string sPri = path;
    sPri += PRIVATE;
    std::string sPub = path;
    sPub += PUBLIC;
    LoadPrivateKey( sPri.c_str(), privateKey );
    LoadPublicKey( sPub.c_str(), publicKey );

    CryptoPP::ECIES <CryptoPP::ECP>::Encryptor Encryptor (publicKey);
    CryptoPP::ECIES <CryptoPP::ECP>::Decryptor Decryptor (privateKey);

    std::string plainText = data;
    size_t plainTextLength = plainText.length () + 1;
    size_t cipherTextLength = Encryptor.CiphertextLength (plainTextLength);
    CryptoPP::byte * cipherText = new CryptoPP::byte[cipherTextLength];
    memset (cipherText, 0x00, cipherTextLength);

    CryptoPP::AutoSeededRandomPool rng;
    Encryptor.Encrypt (rng, reinterpret_cast < const CryptoPP::byte * > (plainText.data ()), plainTextLength, cipherText);

	if(isHex)
	{
		int iLen = cipherTextLength * 2 + 1;
		*result = (unsigned char *)malloc(iLen);
		memset(*result, 0x00, iLen);
		encode_hex((char *)*result, (unsigned char *)cipherText, cipherTextLength);
		return iLen;
	}
	else
	{
		*result = (unsigned char *)cipherText;
		return cipherTextLength;
	}
}

int GeneratePairKey(const char *path)
{
    bool result;
    if(NULL == path)
    {
        return -1;
    }

    if(access(path, F_OK))
    {
        if(mkdir(path, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH))
        {
            return -2;
        }
    }

    ECDSA<ECP, SHA1>::PrivateKey privateKey;
    ECDSA<ECP, SHA1>::PublicKey publicKey;

    result = GeneratePrivateKey( CryptoPP::ASN1::secp256r1(), privateKey );
    if( !result ) { return -3; }

    result = GeneratePublicKey( privateKey, publicKey );
    if( !result ) { return -4; }

    std::string sPri = path;
    sPri += PRIVATE;
    std::string sPub = path;
    sPub += PUBLIC;
    SavePrivateKey( sPri.c_str(), privateKey );
    SavePublicKey( sPub.c_str(), publicKey );

    return 0;
}

void SetPublicKey(ECDSA<ECP, SHA1>::PublicKey& key, const string& sPubStr )
{
	cstring *s = hex2str(sPubStr.c_str());
    char c1 = s->str[0];
    char c2 = s->str[1];
    cstr_free(s, true);

    HexDecoder decoder1;
    decoder1.Put((CryptoPP::byte*)&sPubStr[4], sPubStr.size()-4);
    decoder1.MessageEnd();

    ECP::Point qt;
    size_t len = decoder1.MaxRetrievable();

	if((size_t)(c1 + c2) == len)
	{
		qt.identity = false;
		qt.x.Decode(decoder1, c1);
		qt.y.Decode(decoder1, c2);
		key.Initialize( CryptoPP::ASN1::secp256r1(), qt );
	}
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

void GetPrivateKey(const ECDSA<ECP, SHA1>::PrivateKey& key, string& sPriStr )
{
    const Integer& x = key.GetPrivateExponent();

    for(int i = x.ByteCount() - 1; i >= 0; i--)
    {
        sPriStr += x.GetByte(i);
    }
}

void GetPublicKey(const ECDSA<ECP, SHA1>::PublicKey& key, string& sPubStr )
{
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

bool GeneratePrivateKey( const OID& oid, ECDSA<ECP, SHA1>::PrivateKey& key )
{
    AutoSeededRandomPool prng;

    key.Initialize( prng, oid );
    assert( key.Validate( prng, 3 ) );
     
    return key.Validate( prng, 3 );
}

bool GeneratePublicKey( const ECDSA<ECP, SHA1>::PrivateKey& privateKey, ECDSA<ECP, SHA1>::PublicKey& publicKey )
{
    AutoSeededRandomPool prng;

    assert( privateKey.Validate( prng, 3 ) );

    privateKey.MakePublicKey(publicKey);
    assert( publicKey.Validate( prng, 3 ) );

    return publicKey.Validate( prng, 3 );
}

void PrintDomainParameters( const ECDSA<ECP, SHA1>::PrivateKey& key )
{
    PrintDomainParameters( key.GetGroupParameters() );
}

void PrintDomainParameters( const ECDSA<ECP, SHA1>::PublicKey& key )
{
    PrintDomainParameters( key.GetGroupParameters() );
}

void PrintDomainParameters( const DL_GroupParameters_EC<ECP>& params )
{
    cout << endl;
 
    cout << "Modulus:" << endl;
    cout << " " << params.GetCurve().GetField().GetModulus() << endl;
    
    cout << "Coefficient A:" << endl;
    cout << " " << params.GetCurve().GetA() << endl;
    
    cout << "Coefficient B:" << endl;
    cout << " " <<params.GetCurve().GetB() << endl;
    
    cout << "Base Point:" << endl;
    cout << " X: " << params.GetSubgroupGenerator().x << endl; 
    cout << " Y: " << params.GetSubgroupGenerator().y << endl;
    
    cout << "Subgroup Order:" << endl;
    cout << " " << params.GetSubgroupOrder() << endl;
    
    cout << "Cofactor:" << endl;
    cout << " " << params.GetCofactor() << endl;    
}

void PrintPrivateKey( const ECDSA<ECP, SHA1>::PrivateKey& key )
{   
    const Integer& x = key.GetPrivateExponent();
    cout << endl;
    cout << "Private_Len " << x.ByteCount() << endl; 
    cout << "Private Exponent:" << endl;
    cout << " " << std::hex << x  << endl; 
}

void PrintPublicKey( const ECDSA<ECP, SHA1>::PublicKey& key )
{   
    cout << endl;
    cout << "Public Element:" << endl;
    cout << " X: " <<std::hex << key.GetPublicElement().x << endl; 
    cout << " Y: " <<std::hex << key.GetPublicElement().y << endl;
}

void SavePrivateKey( const string& filename, const ECDSA<ECP, SHA1>::PrivateKey& key )
{
    key.Save( FileSink( filename.c_str(), true ).Ref() );
}

void SavePublicKey( const string& filename, const ECDSA<ECP, SHA1>::PublicKey& key )
{   
    key.Save( FileSink( filename.c_str(), true ).Ref() );
}

void LoadPrivateKey( const string& filename, ECDSA<ECP, SHA1>::PrivateKey& key )
{   
    key.Load( FileSource( filename.c_str(), true ).Ref() );
}

void LoadPublicKey( const string& filename, ECDSA<ECP, SHA1>::PublicKey& key )
{
    key.Load( FileSource( filename.c_str(), true ).Ref() );
}

bool SignMessage( const ECDSA<ECP, SHA1>::PrivateKey& key, const string& message, string& signature )
{
    AutoSeededRandomPool prng;
    
    signature.erase();    

    StringSource( message, true, new SignerFilter( prng, ECDSA<ECP,SHA1>::Signer(key), new StringSink( signature )) ); 
    
    return !signature.empty();
}

bool VerifyMessage( const ECDSA<ECP, SHA1>::PublicKey& key, const string& message, const string& signature )
{
    bool result = false;

    StringSource( signature+message, true, new SignatureVerificationFilter( ECDSA<ECP,SHA1>::Verifier(key), new ArraySink( (CryptoPP::byte*)&result, sizeof(result) ))); 

    return result;
}

bool accountinfo::GenerateKey(const char *Bs58Addr, bool bFlag)
{
    bool result = true;
	ClearCurrentUser();
	CurrentUser.PrivateKeyName.append(Bs58Addr, strlen(Bs58Addr));
	CurrentUser.PrivateKeyName += PRIVATE_SUFFIX;
    CurrentUser.PrivateKeyName.insert(0, path.c_str());
	CurrentUser.PublicKeyName.append(Bs58Addr, strlen(Bs58Addr));
	CurrentUser.PublicKeyName += PUBLIC_SUFFIX;
    CurrentUser.PublicKeyName.insert(0, path.c_str());

    LoadPrivateKey( CurrentUser.PrivateKeyName.c_str(), CurrentUser.privateKey);
    LoadPublicKey( CurrentUser.PublicKeyName.c_str(), CurrentUser.publicKey);

    GetPrivateKey(CurrentUser.privateKey, CurrentUser.sPriStr);
    GetPublicKey(CurrentUser.publicKey, CurrentUser.sPubStr);

    char buf[2048] = {0};
    size_t buf_len = sizeof(buf);
    GetBase58Addr(buf, &buf_len, 0x00, CurrentUser.sPubStr.c_str(), CurrentUser.sPubStr.length());

    CurrentUser.Bs58Addr.append(buf, buf_len - 1 );

    AccountList.insert(make_pair(CurrentUser.Bs58Addr.c_str(), CurrentUser));

    if(true == bFlag)
    {
        DefaultKeyBs58Addr = CurrentUser.Bs58Addr;
		SetKeyByBs58Addr(g_privateKey, g_publicKey, DefaultKeyBs58Addr.c_str());
    }
    return result;
}

int accountinfo::GetMnemonic(const char *Bs58Addr, char *out, int outLen)
{
    account acc;
    bool bSuccess = FindKey(Bs58Addr, acc);

    if (!bSuccess)
    {
        g_AccountInfo.SetKeyByBs58Addr(g_privateKey, g_publicKey, g_AccountInfo.DefaultKeyBs58Addr.c_str());
        acc = CurrentUser;
    }
    
    if(acc.sPriStr.size() <= 0)
    {
        return 0;
    }
        
    return  mnemonic_from_data((const uint8_t*)acc.sPriStr.c_str(), acc.sPriStr.size(), out, outLen);    
}

bool accountinfo::KeyFromPrivate(const char *pridata, int iLen)
{
	char mnemonic_hex[65] = {0};
	encode_hex(mnemonic_hex, pridata, iLen);

    bool result;
	ClearCurrentUser();

	std::string mnemonic_key;
	mnemonic_key.append(mnemonic_hex, iLen * 2);

	SetPrivateKey(CurrentUser.privateKey, mnemonic_key);

    result = GeneratePublicKey( CurrentUser.privateKey, CurrentUser.publicKey );
    if( !result ) { return false; }

    GetPrivateKey(CurrentUser.privateKey, CurrentUser.sPriStr);
    GetPublicKey(CurrentUser.publicKey, CurrentUser.sPubStr);

    char buf[2048] = {0};
    size_t buf_len = sizeof(buf);
    GetBase58Addr(buf, &buf_len, 0x00, CurrentUser.sPubStr.c_str(), CurrentUser.sPubStr.length());

    CurrentUser.Bs58Addr.append(buf, buf_len - 1 );

    AccountList.insert(make_pair(CurrentUser.Bs58Addr.c_str(), CurrentUser));

    CurrentUser.PrivateKeyName = CurrentUser.Bs58Addr + PRIVATE_SUFFIX;
    CurrentUser.PublicKeyName = CurrentUser.Bs58Addr + PUBLIC_SUFFIX;
    if(1 == AccountList.size())
    {
        CurrentUser.PrivateKeyName.insert(0, OWNER_CERT_DEFAULT);
        CurrentUser.PublicKeyName.insert(0, OWNER_CERT_DEFAULT);
    }
    CurrentUser.PrivateKeyName.insert(0, path.c_str());
    CurrentUser.PublicKeyName.insert(0, path.c_str());
    SavePrivateKey( CurrentUser.PrivateKeyName, CurrentUser.privateKey );
    SavePublicKey( CurrentUser.PublicKeyName, CurrentUser.publicKey );
    if(1 == AccountList.size())
    {
        DefaultKeyBs58Addr = CurrentUser.Bs58Addr;
		SetKeyByBs58Addr(g_privateKey, g_publicKey, DefaultKeyBs58Addr.c_str());
    }

    return result;
}

bool accountinfo::GenerateKeyFromMnemonic(const char *mnemonic)
{
	char out[33] = {0};
    int outLen = 0;
	if(!mnemonic_check((char *)mnemonic, out, &outLen))
		return false;

    return KeyFromPrivate(out, outLen);
}

bool accountinfo::GenerateKey()
{
    bool result;
	ClearCurrentUser();
    result = GeneratePrivateKey( CryptoPP::ASN1::secp256r1(), CurrentUser.privateKey );
    if( !result ) { return false; }

    result = GeneratePublicKey( CurrentUser.privateKey, CurrentUser.publicKey );
    if( !result ) { return false; }


    GetPrivateKey(CurrentUser.privateKey, CurrentUser.sPriStr);
    GetPublicKey(CurrentUser.publicKey, CurrentUser.sPubStr);

    char buf[2048] = {0};
    size_t buf_len = sizeof(buf);
    GetBase58Addr(buf, &buf_len, 0x00, CurrentUser.sPubStr.c_str(), CurrentUser.sPubStr.length());

    CurrentUser.Bs58Addr.append(buf, buf_len - 1 );

    AccountList.insert(make_pair(CurrentUser.Bs58Addr.c_str(), CurrentUser));

    CurrentUser.PrivateKeyName = CurrentUser.Bs58Addr + PRIVATE_SUFFIX;
    CurrentUser.PublicKeyName = CurrentUser.Bs58Addr + PUBLIC_SUFFIX;
    if(1 == AccountList.size())
    {
        CurrentUser.PrivateKeyName.insert(0, OWNER_CERT_DEFAULT);
        CurrentUser.PublicKeyName.insert(0, OWNER_CERT_DEFAULT);
    }
    CurrentUser.PrivateKeyName.insert(0, path.c_str());
    CurrentUser.PublicKeyName.insert(0, path.c_str());
    SavePrivateKey( CurrentUser.PrivateKeyName, CurrentUser.privateKey );
    SavePublicKey( CurrentUser.PublicKeyName, CurrentUser.publicKey );
    if(1 == AccountList.size())
    {
        DefaultKeyBs58Addr = CurrentUser.Bs58Addr;
		SetKeyByBs58Addr(g_privateKey, g_publicKey, DefaultKeyBs58Addr.c_str());
    }

    return result;
}

bool accountinfo::GetPriKeyStr(const char *Bs58Addr, std::string &PriKey)
{
    bool result = false;
	const char *pBs58Addr;
	if(Bs58Addr)
	{
		pBs58Addr = Bs58Addr;
	}
	else
	{
		pBs58Addr = DefaultKeyBs58Addr.c_str();
	}
    iter = AccountList.begin();
    while(iter != AccountList.end())
    {
        if(strlen(pBs58Addr) == iter->first.length() &&!memcmp(iter->first.c_str(), pBs58Addr, iter->first.length()))
        {
            saveit = iter;
            PriKey = saveit->second.sPriStr; 
            result = true;
            break;
        }
        iter++;
    }
    return result;
}

bool accountinfo::GetPubKeyStr(const char *Bs58Addr, std::string &PubKey)
{
    bool result = false;
	const char *pBs58Addr;
	if(Bs58Addr)
	{
		pBs58Addr = Bs58Addr;
	}
	else
	{
		pBs58Addr = DefaultKeyBs58Addr.c_str();
	}
    iter = AccountList.begin();
    while(iter != AccountList.end())
    {
        if(strlen(pBs58Addr) == iter->first.length() &&!memcmp(iter->first.c_str(), pBs58Addr, iter->first.length()))
        {
            saveit = iter;
            PubKey = saveit->second.sPubStr; 
            result = true;
            break;
        }
        iter++;
    }
    return result;
}

bool accountinfo::DeleteAccount(const char *Bs58Addr)
{
    bool result = false;
    iter = AccountList.begin();
    while(iter != AccountList.end())
    {
        if(strlen(Bs58Addr) == iter->first.length() &&!memcmp(iter->first.c_str(), Bs58Addr, iter->first.length()))
        {
            saveit = iter;
            AccountList.erase(saveit);
            result = true;
            break;
        }
        iter++;
    }
    return result;
}

bool accountinfo::Sign(const char *Bs58Addr, const string& message, string& signature)
{
    bool result = false;
    iter = AccountList.begin();
    while(iter != AccountList.end())
    {
        if(strlen(Bs58Addr) == iter->first.length() &&!memcmp(iter->first.c_str(), Bs58Addr, iter->first.length()))
        {
            saveit = iter;
            break;
        }
        iter++;
    }
    if(iter == AccountList.end())
    {
        return false;
    }
    result = SignMessage( saveit->second.privateKey, message, signature );
    return result;
}

void accountinfo::ClearCurrentUser()
{
	CurrentUser.PrivateKeyName.clear();
	CurrentUser.PublicKeyName.clear();
	CurrentUser.sPriStr.clear();
	CurrentUser.sPubStr.clear();
	CurrentUser.Bs58Addr.clear();
	CurrentUser.PrivateKeyName = "";
	CurrentUser.PublicKeyName = "";
	CurrentUser.sPriStr = "";
	CurrentUser.sPubStr = "";
	CurrentUser.Bs58Addr = "";
}

bool accountinfo::SetKeyByBs58Addr(ECDSA<ECP, SHA1>::PrivateKey &privateKey, ECDSA<ECP, SHA1>::PublicKey &publicKey, const char *Bs58Addr)
{
    std::lock_guard<std::mutex> lock(SetKeyLock);
	std::string PubKey, PriKey;
	const char *pBs58Addr;
	if(Bs58Addr)
	{
		pBs58Addr = Bs58Addr;
	}
	else
	{
		pBs58Addr = DefaultKeyBs58Addr.c_str();
	}

    account retAccount;
    bool bSuccess = FindKey(pBs58Addr, retAccount);

    if (bSuccess)
    {
        ClearCurrentUser();
        CurrentUser = retAccount;
        privateKey = CurrentUser.privateKey;
        publicKey = CurrentUser.publicKey;
    }

	return bSuccess;
}

void accountinfo::print_AddrList()
{
    iter = AccountList.begin();
    while(iter != AccountList.end())
    {
        std::cout<<iter->first.c_str()<<std::endl;
        iter++;
    }
}

bool accountinfo::isExist(const char *Bs58Addr)
{
    auto find = AccountList.find(Bs58Addr);
    if(find == AccountList.end())
    {
        return false;
    }else
    {
        return true; 
    }
}

bool accountinfo::FindKey(const char *Bs58Addr, account & acc)
{
    if (Bs58Addr == nullptr || strlen(Bs58Addr) == 0)
    {
        return false;
    }

    if (!isExist(Bs58Addr))
    {
        return false;
    }

    std::string PubKey, PriKey;
    ECDSA<ECP, SHA1>::PrivateKey privateKey;
    ECDSA<ECP, SHA1>::PublicKey publicKey;

    iter = AccountList.begin();
    while(iter != AccountList.end())
    {
        if(strlen(Bs58Addr) == iter->first.length() &&!memcmp(iter->first.c_str(), Bs58Addr, iter->first.length()))
        {
            saveit = iter;

            PriKey = saveit->second.sPriStr; 
            PubKey = saveit->second.sPubStr; 

			cstring *sPri = str2hex(PriKey.c_str(), PriKey.length());
			cstring *sPub = str2hex(PubKey.c_str(), PubKey.length());

			std::string sPriStr1 = sPri->str;
			std::string sPubStr1 = sPub->str;

			SetPrivateKey(privateKey, sPriStr1);
			SetPublicKey(publicKey, sPubStr1);

			cstr_free(sPri, true);
			cstr_free(sPub, true);
            break;
        }
        iter++;
    }

    acc.privateKey = privateKey;
    acc.publicKey = publicKey;
    
    GetPrivateKey(acc.privateKey, acc.sPriStr);
    GetPublicKey(acc.publicKey, acc.sPubStr);

    char buf[2048] = {0};
    size_t buf_len = sizeof(buf);
    GetBase58Addr(buf, &buf_len, 0x00, acc.sPubStr.c_str(), acc.sPubStr.length());
    acc.Bs58Addr.append(buf, buf_len - 1 );

    acc.PrivateKeyName = acc.Bs58Addr + PRIVATE_SUFFIX;
    acc.PublicKeyName = acc.Bs58Addr + PUBLIC_SUFFIX;

    return true;
}

size_t accountinfo::GetAccSize(void)
{
    return AccountList.size();
}

std::string getsha1hash(std::string text) 
{
	std::string hash;
	CryptoPP::SHA1 sha1;
	CryptoPP::HashFilter hashfilter(sha1);
	hashfilter.Attach(new CryptoPP::HexEncoder(new CryptoPP::StringSink(hash), false));
	hashfilter.Put(reinterpret_cast<const unsigned char*>(text.c_str()), text.length());
	hashfilter.MessageEnd();
	return hash;
}

std::string getsha256hash(std::string text)
{
    std::string hash;
	CryptoPP::SHA256 sha256;
	CryptoPP::HashFilter hashfilter(sha256);
	hashfilter.Attach(new CryptoPP::HexEncoder(new CryptoPP::StringSink(hash), false));
	hashfilter.Put(reinterpret_cast<const unsigned char*>(text.c_str()), text.length());
	hashfilter.MessageEnd();
	return hash;
}

std::string getMD5hash(std::string text)
{
    std::string hash;
	CryptoPP::Weak::MD5 md5;
	CryptoPP::HashFilter hashfilter(md5);
	hashfilter.Attach(new CryptoPP::HexEncoder(new CryptoPP::StringSink(hash), false));
	hashfilter.Put(reinterpret_cast<const unsigned char*>(text.c_str()), text.length());
	hashfilter.MessageEnd();
	return hash;
}

int accountinfo::GetKeyStore(const char *Bs58Addr, const char *pass, char *buf, unsigned int bufLen)
{
    if(buf == NULL)
    {
        return -1;
    }

	AutoSeededRandomPool rng;
	unsigned char iv[17] = {0};
	rng.GenerateBlock((CryptoPP::byte *)iv, 16);

	int json_version = 1;
	int kdfparams_c = 10240;
	int kdfparams_dklen = 32;
	int kdfparams_salt_len = 32;
	const char *kdfparams_prf = "hmac-sha256";
	const char *cipher = "aes-128-ctr";
	const char *kdf = "pbkdf2";
	unsigned char salt[33] = {0};
	char key[33] = {0};
	rng.GenerateBlock((CryptoPP::byte *)salt, 32);

	pbkdf2_hmac_sha256((const uint8_t *)pass, strlen(pass), (uint8_t*)salt, kdfparams_salt_len, kdfparams_c, (uint8_t*)key, kdfparams_dklen, NULL);	

	unsigned char encKey[17] = {0};
	int keysize = 16;
	memcpy(encKey, key, 16);


    account acc;
    bool bSuccess = FindKey(Bs58Addr, acc);
    if (!bSuccess)
    {
        acc = CurrentUser;
    }

	string  strEncTxt;
    string  strDecTxt;
	string message = acc.sPriStr;
	
	if(message.size()<=0)	
		return -2;

    CTR_Mode<AES>::Encryption  Encryptor(encKey,keysize,iv);
	StringSource ss1(message, true, new StreamTransformationFilter( Encryptor, new StringSink( strEncTxt )));

/**
    string  cipher;
	StringSource ss2( strEncTxt, true, new HexEncoder( new StringSink( cipher)));
    cout << "cipher text: " << cipher << endl;

**/
	
	string macstr;
	macstr.append(key, 32);
	macstr.append(strEncTxt.c_str(), strEncTxt.size());

	sha3_256_t sha3_256;
    sha3_256_t::sha3_256_item_t sha3_256_item;
    sha3_256.open(&sha3_256_item);
    sha3_256.update(macstr.c_str(), macstr.size());
    sha3_256.close();
	
	std::string json_address = acc.Bs58Addr;	

	cstring * json_iv = str2hex(iv, 16);	

	cstring * json_ciphertext = str2hex(strEncTxt.c_str(), strEncTxt.size());	

	cstring * json_salt = str2hex(salt, kdfparams_salt_len);	
	cstring * json_mac = str2hex(&sha3_256_item, sizeof(sha3_256_item));	

	char json_uuid[UUID4_LEN] = {0};
	uuid4_init();
	uuid4_generate(json_uuid);
/** json **/
	cJSON * root =  NULL;
	cJSON * crypto =  NULL;
	cJSON * cipherparams =  NULL;
	cJSON * kdfparams =  NULL;
	root =  cJSON_CreateObject();
	crypto =  cJSON_CreateObject();
	cipherparams =  cJSON_CreateObject();
	kdfparams =  cJSON_CreateObject();
	cJSON_AddItemToObject(root, "crypto", crypto);
	cJSON_AddItemToObject(crypto, "cipherparams", cipherparams);
	cJSON_AddItemToObject(crypto, "kdfparams", kdfparams);
	//root
	cJSON_AddItemToObject(root, "address", cJSON_CreateString(json_address.c_str()));
	cJSON_AddItemToObject(root, "version", cJSON_CreateNumber(json_version));
	cJSON_AddItemToObject(root, "id", cJSON_CreateString(json_uuid));

	//crypto
	cJSON_AddItemToObject(crypto, "cipher", cJSON_CreateString(cipher));
	cJSON_AddItemToObject(crypto, "ciphertext", cJSON_CreateString(json_ciphertext->str));
	cJSON_AddItemToObject(crypto, "kdf", cJSON_CreateString(kdf));
	cJSON_AddItemToObject(crypto, "mac", cJSON_CreateString(json_mac->str));

	//cipherparams
	cJSON_AddItemToObject(cipherparams, "iv", cJSON_CreateString(json_iv->str));

	//kdfparams
	cJSON_AddItemToObject(kdfparams, "salt", cJSON_CreateString(json_salt->str));
	cJSON_AddItemToObject(kdfparams, "prf", cJSON_CreateString(kdfparams_prf));
	cJSON_AddItemToObject(kdfparams, "c", cJSON_CreateNumber(kdfparams_c));
	cJSON_AddItemToObject(kdfparams, "dklen", cJSON_CreateNumber(kdfparams_dklen));
	

	cstring *json_keystore = cstr_new(cJSON_PrintUnformatted(root));
	cJSON_Delete(root);

    if(bufLen < json_keystore->len)
        return -3;
    memcpy(buf, json_keystore->str, json_keystore->len);
    int iReturnLen = json_keystore->len;
	cstr_free(json_keystore, true);
/** json **/
	cstr_free(json_iv, true);
	cstr_free(json_ciphertext, true);
	cstr_free(json_salt, true);
	cstr_free(json_mac, true);
    return iReturnLen;
}

bool accountinfo::ImportKeyStore(const char *keystore, const char *pass)
{
	int version = 1;
	int kdfparams_c = 0;
	int kdfparams_dklen = 0;
	const char *kdfparams_prf = "hmac-sha256";
	const char *cipher = "aes-128-ctr";
	const char *kdf = "pbkdf2";
	char key[33] = {0};

	unsigned char encKey[17] = {0};
	int keysize = 16;

	string macstr;
	sha3_256_t sha3_256;
    sha3_256_t::sha3_256_item_t sha3_256_item;

	string  strEncTxt;
    string  strDecTxt;

	cstring * cipherparams_iv =  NULL;
	cstring * ciphertext =  NULL;
	cstring * kdfparams_salt =  NULL;
	cstring * mac =  NULL;
	cstring * address =  NULL;

	cJSON * root = NULL;	
	cJSON * crypto =  NULL;
	cJSON * cipherparams =  NULL;
	cJSON * kdfparams =  NULL;
	cJSON *item = NULL;

	root = cJSON_Parse(keystore);

	crypto = cJSON_GetObjectItem(root, "crypto");
	cipherparams = cJSON_GetObjectItem(crypto, "cipherparams");
	kdfparams = cJSON_GetObjectItem(crypto, "kdfparams");

	item = cJSON_GetObjectItem(root, "version");
	if(version != item->valueint)
	{
		cJSON_Delete(root);
		return false;
	}

	item = cJSON_GetObjectItem(crypto, "cipher");
	if(memcmp(item->valuestring, cipher, strlen(cipher)))
	{
		cJSON_Delete(root);
		return false;
	}


	item = cJSON_GetObjectItem(crypto, "kdf");
	if(memcmp(item->valuestring, kdf, strlen(kdf)))
	{
		cJSON_Delete(root);
		return false;
	}

	item = cJSON_GetObjectItem(kdfparams, "prf");
	if(memcmp(item->valuestring, kdfparams_prf, strlen(kdfparams_prf)))
	{
		cJSON_Delete(root);
		return false;
	}

	item = cJSON_GetObjectItem(kdfparams, "c");
	kdfparams_c = item->valueint;

	item = cJSON_GetObjectItem(kdfparams, "dklen");
	kdfparams_dklen = item->valueint;

	item = cJSON_GetObjectItem(cipherparams, "iv");
	cipherparams_iv = hex2str(item->valuestring);

	item = cJSON_GetObjectItem(crypto, "ciphertext");
	ciphertext = hex2str(item->valuestring);

	item = cJSON_GetObjectItem(crypto, "mac");
	mac = hex2str(item->valuestring);
	
	item = cJSON_GetObjectItem(kdfparams, "salt");
	kdfparams_salt = hex2str(item->valuestring);

	item = cJSON_GetObjectItem(root, "address");
	address = hex2str(item->valuestring);
	
	pbkdf2_hmac_sha256((const uint8_t *)pass, strlen(pass), (uint8_t*)kdfparams_salt->str, kdfparams_salt->len, kdfparams_c, (uint8_t*)key, kdfparams_dklen, NULL);	

//mac
	macstr.append(key, 32);
    macstr.append(ciphertext->str, ciphertext->len);

    sha3_256.open(&sha3_256_item);
    sha3_256.update(macstr.c_str(), macstr.size());
    sha3_256.close();
	
	if(mac->len != sizeof(sha3_256_item) || memcmp(mac->str, &sha3_256_item, sizeof(sha3_256_item)))
	{
		cstr_free(cipherparams_iv, true);
		cstr_free(ciphertext, true);
		cstr_free(mac, true);
		cstr_free(kdfparams_salt, true);
		cstr_free(address, true);
		cJSON_Delete(root);
		return false;
	}

	memcpy(encKey, key, 16);

	strEncTxt.append(ciphertext->str, ciphertext->len);
	CTR_Mode<AES>::Decryption Decryptor(encKey, keysize, (CryptoPP::byte*)cipherparams_iv->str); 
	StringSource ss3(strEncTxt , true, new StreamTransformationFilter( Decryptor, new StringSink( strDecTxt)));

	hex_print((unsigned char *)strDecTxt.c_str(), strDecTxt.size());

	cstr_free(cipherparams_iv, true);
	cstr_free(ciphertext, true);
	cstr_free(mac, true);
	cstr_free(kdfparams_salt, true);
	cstr_free(address, true);
	cJSON_Delete(root);

    return KeyFromPrivate(strDecTxt.c_str(), strDecTxt.size());
}

bool accountinfo::GetPrivateKeyHex(const char *Bs58Addr, char *pridata, unsigned int iLen)
{
    account acc;
    bool bSuccess = FindKey(Bs58Addr, acc);

    if (!bSuccess)
    {
        acc = CurrentUser;
    }

	if(acc.sPriStr.size() <= 0)
    {
        return false;
    }
		
	if(pridata == NULL || iLen < acc.sPriStr.size() * 2)
    {
        iLen = acc.sPriStr.size() * 2;
        return false;
    }
		
	cstring *sPri = str2hex(acc.sPriStr.c_str(), acc.sPriStr.size());
	memcpy(pridata, sPri->str, sPri->len);
	cstr_free(sPri, true);
	return true;
}

bool accountinfo::ImportPrivateKeyHex(const char *pridata)
{
	bool iResult = false;
	cstring *prikey = hex2str(pridata);
    if (!prikey)
    {
        return false;
    }

	iResult = KeyFromPrivate(prikey->str, prikey->len);
	cstr_free(prikey, true);
	return iResult;
}
