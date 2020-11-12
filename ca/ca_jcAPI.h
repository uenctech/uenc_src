extern "C" {
    /**
        说明:
            生成公私钥和钱包地址
        参数:
            ver: 版本号
                传0生成1开头的base58地址
                传5生成3开头的base58地址
        返回值:
            私钥,公钥,base58地址 拼接而成的字符串 以空格衔接
            私钥,公钥为十六进制字符串
    */
    char* GenWallet(int ver);

    /*
        说明:
            生成公私钥和钱包地址
        参数:
            out_private_key: 传出参数，传出私钥(字节流形式)，调用方负责开辟内存, 确保大于 33 字节
            out_private_key_len: 传入传出参数，传入时代表开辟的内存大小，传出时返回私钥的实际长度 
            out_public_key：传出参数，传出公钥(字节流形式)，调用方负责开辟内存，确保大于 67 字节
            out_public_key_len：传入传出参数，传入时代表开辟的内存大小，传出时返回公钥的实际长度
            out_bs58addr：传出参数，传出地址，调用方负责开辟内存，确保大于 35 字节
            out_bs58addr_len：传入传出参数，传入时代表开辟的内存大小，传出时返回地址的实际长度
        返回值:
            0 代表成功
            -1 代表开辟的内存空间不足
    */
    int GenWallet_(char *out_private_key, int *out_private_key_len,
                  char *out_public_key, int *out_public_key_len, 
                  char *out_bs58addr, int *out_bs58addr_len);


    /**
        说明:
            生成base64编码之后的签名
        参数:
            pri: 十六进制的私钥
            msg: 待签名信息
            len: 待签名信息的长度
        返回值:
            经过base64编码之后的签名信息
    */
    char* GenSign(char* pri, char* msg, int len);


    /*
    说明:
        生成base64编码之后的签名信息
    参数:
        pri: 私钥(字节流形式)
        pri_len: 私钥的长度 
        msg：待签名信息
        msg_len：待签名信息的长度
        signature_msg：传出参数，传出base64编码之后的签名信息，调用方负责开辟内存，确保大于 90 字节
        out_len：传入传出参数，传入时代表开辟的内存大小，传出时返回签名信息的实际长度
    返回值:
        0 代表成功
        -1 代表开辟的内存空间不足
    */
    int GenSign_(const char* pri, int pri_len,
                const char* msg, int msg_len,
                char *signature_msg, int *out_len); 
}
