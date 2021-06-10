extern "C" {
    /**
        Description :
            Generate public and private keys and wallet addresses 
        parameter :
            ver: version number 
                Pass 0 to generate base58 address starting with 1 
                Pass 5 to generate base58 address starting with 3 
        return value :
            Private key, public key, base58 address concatenated string connected by spaces 
            Private key, public key is a hexadecimal string 
    */
    char* GenWallet(int ver);

    /*
        Description :
            Generate public and private keys and wallet addresses 
        parameter :
            out_private_key: Outgoing parameters, outgoing private key (in byte stream format), the caller is responsible for opening up the memory, ensuring that it is greater than 33 bytes 

            out_private_key_len: Incoming and outgoing parameters, which represent the size of the memory opened when they are passed in, and return the actual length of the private key when they are passed out  
            out_public_key：For parameters and public keys (in byte stream format), the caller is responsible for opening up memory to ensure that it is greater than 67 bytes 
            out_public_key_len：Incoming and outgoing parameters, when they are passed in, they represent the size of the memory opened up, and when they are out, they return the actual length of the public key 
            out_bs58addr：For outgoing parameters and outgoing addresses, the caller is responsible for opening up the memory to ensure that it is greater than 35 bytes 
            out_bs58addr_len：Incoming and outgoing parameters, when they are passed in, they represent the size of the memory opened up, and when they are out, the actual length of the return address is 
        return value :
            0 means success 
            -1 means insufficient memory space 
    */
    int GenWallet_(char *out_private_key, int *out_private_key_len,
                  char *out_public_key, int *out_public_key_len, 
                  char *out_bs58addr, int *out_bs58addr_len);


    /**
        Description :
            Generate signature after base64 encoding 
        parameter :
            pri: Hexadecimal private key 
            msg: Information to be signed 
            len: The length of the message to be signed 
        return value :
            Signature information after base64 encoding 
    */
    char* GenSign(char* pri, char* msg, int len);


    /*
    Description :
        Generate signature information after base64 encoding 
    parameter :
        pri: Private key (byte stream format) 
        pri_len: The length of the private key  
        msg：Information to be signed 
        msg_len：The length of the message to be signed 
        signature_msg：Outgoing parameters, outgoing signature information after base64 encoding, the caller is responsible for opening up the memory to ensure that it is greater than 90 bytes 
        out_len：Incoming and outgoing parameters, when they are passed in, they represent the size of the opened memory, and when they are out, they return the actual length of the signature 
    return value :
         0 means success
        -1 means insufficient memory space 
    */
    int GenSign_(const char* pri, int pri_len,
                const char* msg, int msg_len,
                char *signature_msg, int *out_len); 
}
