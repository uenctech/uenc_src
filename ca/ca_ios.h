//
//  ca_ios.h
//  UENCCA
//
//  Created by 盛嘉炜 on 2019/8/19.
//  Copyright © 2019 盛嘉炜. All rights reserved.
//

#ifndef ca_ios_h
#define ca_ios_h

#include <endian.h>

#if defined(__APPLE__) // 如果是MAC或者iOS
    #define E32toh EndianU32_NtoL
    #define E16toh EndianU16_NtoL
    #define E64toh EndianU64_NtoL

    #define H32toh EndianS32_NtoL
    #define H16toh EndianS16_NtoL
    #define H64toh EndianS64_NtoL

    #include <libkern/OSByteOrder.h>
    #define Bswap_32 OSSwapInt32
    #define Bswap_16 OSSwapInt16
    #define Bswap_64 OSSwapInt64

#else // 其他情况(可以用宏区分windows和Centos)
    #define E32toh le32toh
    #define E16toh le16toh
    #define E64toh le64toh

    #define H32toh htole32
    #define H16toh htole16
    #define H64toh htole64

    #include <byteswap.h>
    #define Bswap_32 bswap_32
    #define Bswap_16 bswap_16
    #define Bswap_64 bswap_64

#endif



#endif /* ca_ios_h */
