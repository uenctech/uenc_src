#include "ca_phoneAPI.h"
#include "ca_base64.h"

//g++ -fPIC -c jcAPI.cpp
//g++ -shared *.o -o libjcAPI.so libphone.a
extern "C" {
    // static unsigned char alphabet_map[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    // static unsigned char reverse_map[] =
    // {
    //     255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    //     255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    //     255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 62, 255, 255, 255, 63,
    //     52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 255, 255, 255, 255, 255, 255,
    //     255,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    //     15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 255, 255, 255, 255, 255,
    //     255, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    //     41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 255, 255, 255, 255, 255
    // };

    // unsigned long base64_encode(const unsigned char *text, unsigned long text_len, unsigned char *encode)
    // {
    //     unsigned long i, j;
    //     for (i = 0, j = 0; i+3 <= text_len; i+=3)
    //     {
    //         encode[j++] = alphabet_map[text[i]>>2];                             //Take the first 6 bits of the first character and find the corresponding result character 
    //         encode[j++] = alphabet_map[((text[i]<<4)&0x30)|(text[i+1]>>4)];     //Combine the last 2 digits of the first character with the first 4 digits of the second character and find the corresponding result character 
    //         encode[j++] = alphabet_map[((text[i+1]<<2)&0x3c)|(text[i+2]>>6)];   //Combine the last 4 digits of the second character with the first 2 digits of the third character and find the corresponding result character 
    //         encode[j++] = alphabet_map[text[i+2]&0x3f];                         //Take out the last 6 digits of the third character and find the resulting character 
    //     }

    //     if (i < text_len)
    //     {
    //         unsigned long tail = text_len - i;
    //         if (tail == 1)
    //         {
    //             encode[j++] = alphabet_map[text[i]>>2];
    //             encode[j++] = alphabet_map[(text[i]<<4)&0x30];
    //             encode[j++] = '=';
    //             encode[j++] = '=';
    //         }
    //         else //tail==2
    //         {
    //             encode[j++] = alphabet_map[text[i]>>2];
    //             encode[j++] = alphabet_map[((text[i]<<4)&0x30)|(text[i+1]>>4)];
    //             encode[j++] = alphabet_map[(text[i+1]<<2)&0x3c];
    //             encode[j++] = '=';
    //         }
    //     }
    //     return j;
    // }

    // unsigned long base64_decode(const unsigned char *code, unsigned long code_len, unsigned char *plain)
    // {
    //     assert((code_len&0x03) == 0);  //If its condition returns an error, the program execution is terminated. A multiple of 4. 

    //     unsigned long i, j = 0;
    //     unsigned char quad[4];
    //     for (i = 0; i < code_len; i+=4)
    //     {
    //         for (unsigned long k = 0; k < 4; k++)
    //         {
    //             quad[k] = reverse_map[code[i+k]];//Group, each group of four are converted to decimal numbers in base64 table in turn
    //         }

    //         assert(quad[0]<64 && quad[1]<64);

    //         plain[j++] = (quad[0]<<2)|(quad[1]>>4); //Take out the first 6 digits of the decimal number corresponding to the base64 table of the first character and combine the first 2 digits of the decimal number of the base64 table corresponding to the second character 


    //         if (quad[2] >= 64)
    //             break;
    //         else if (quad[3] >= 64)
    //         {
    //             plain[j++] = (quad[1]<<4)|(quad[2]>>2); //Take the second character corresponding to the last 4 digits of the decimal number of the base64 table and combine the first 4 digits of the third character corresponding to the decimal number of the base64 table 
    //             break;
    //         }
    //         else
    //         {
    //             plain[j++] = (quad[1]<<4)|(quad[2]>>2);
    //             plain[j++] = (quad[2]<<6)|quad[3];//Take the third character corresponding to the last 2 digits of the decimal number of the base64 table and combine it with the 4th character 
    //         }
    //     }
    //     return j;
    // }


    static const char hexdigit[] = "0123456789abcdef";

    void encode_hex_local(char *hexstr, const void *p_, size_t len)
    {
        const unsigned char *p = (const unsigned char *)p_;
        unsigned int i;

        for (i = 0; i < len; i++) {
            unsigned char v, n1, n2;

            v = p[i];
            n1 = v >> 4;
            n2 = v & 0xf;

            *hexstr++ = hexdigit[n1];
            *hexstr++ = hexdigit[n2];
        }

        *hexstr = 0;
    }

    //Generate wallet 
    //Public and private keys are in hexadecimal 
    //Private key + public key + bs58 address spliced with spaces 
    char* GenWallet(int ver) {
        string out_pri_key;
        string out_pub_key;
        const uint BUFF_SIZE = 128;
        ca_phoneAPI::CPPgenPairKey(out_pri_key, out_pub_key);

        //Private key 
        char* pri_hex = new char[BUFF_SIZE]{0};
        encode_hex_local(pri_hex, out_pri_key.c_str(), out_pri_key.size());

        //Public key 
        char* pub_hex = new char[BUFF_SIZE*2]{0};
        encode_hex_local(pub_hex, out_pub_key.c_str(), out_pub_key.size());

        //bs58
        const char *pub_key = out_pub_key.c_str();
        const int pub_len = out_pub_key.size();
        char *bs58_addr = new char[BUFF_SIZE]{0};
        int *bs58_len = new int;
        ca_phoneAPI::genBs58Addr(pub_key, pub_len, bs58_addr, bs58_len, ver);

        //Splicing 
        string wallet(pri_hex);
        wallet += " ";
        wallet += pub_hex;
        wallet += " ";
        wallet += bs58_addr;
        char* wallet_c = new char[BUFF_SIZE*4]{0};
        strcpy(wallet_c, wallet.c_str());

        return wallet_c;
    }

    int GenWallet_(char *out_private_key, int *out_private_key_len,
                  char *out_public_key, int *out_public_key_len, 
                  char *out_bs58addr, int *out_bs58addr_len)
    {   
        if (*out_private_key_len < 32 || *out_public_key_len < 66 || *out_bs58addr_len < 34)
        {
            return -1;
        }
        string out_pri_key;
        string out_pub_key;
        ca_phoneAPI::CPPgenPairKey(out_pri_key, out_pub_key);

        memcpy(out_private_key, out_pri_key.c_str(), out_pri_key.size());
        *out_private_key_len = out_pri_key.size();

        memcpy(out_public_key, out_pub_key.c_str(), out_pub_key.size());
        *out_public_key_len = out_pub_key.size();

        ca_phoneAPI::genBs58Addr(out_pub_key.c_str(), out_pub_key.size(), out_bs58addr, out_bs58addr_len, 0);

        return 0;
    }

    //Generate signature 
    //Return the signature after bs64 
    char* GenSign(char* pri, char* msg, int len) {
        string pri_str(pri);
        ECDSA<ECP, SHA1>::PrivateKey pri_key;
        ca_phoneAPI::SetPrivateKey(pri_key, pri_str);

        string message(msg, len);
        string signature;
        ca_phoneAPI::SignMessage(pri_key, message, signature);

        uint encode_len = signature.size() * 2;
        unsigned char* encode = new unsigned char[encode_len];
        base64_encode((unsigned char *)signature.data(), signature.size(), encode);
        std::cout << "encode " << encode << std::endl;

        return (char*)encode;
    }

    //Generate signature 
    //Return the signature after bs64 
    int GenSign_(const char* pri, int pri_len,
                 const char* msg, int msg_len,
                 char *signature_msg, int *out_len) 
    {   
        if (*out_len < 90)
        {
            return -1;
        }
        char pri_hex[128] = {0};
        encode_hex_local(pri_hex, pri, pri_len);

        string pri_str(pri_hex);
        ECDSA<ECP, SHA1>::PrivateKey pri_key;
        ca_phoneAPI::SetPrivateKey(pri_key, pri_str);

        string message(msg, msg_len);
        string signature;
        ca_phoneAPI::SignMessage(pri_key, message, signature);
        
        base64_encode((unsigned char *)signature.data(), signature.size(), (unsigned char *)signature_msg);
        *out_len = strlen(signature_msg);

        return 0;
    }

}
