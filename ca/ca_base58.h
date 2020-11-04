/*
 * @Author: your name
 * @Date: 2020-08-19 13:14:48
 * @LastEditTime: 2020-10-14 09:54:14
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \ebpc\ca\ca_base58.h
 */
#ifndef __CA_BASE58__H
#define __CA_BASE58__H

#include <stdbool.h>
#include <stddef.h>
#include <string>
#include "ca_util.h"
#include <vector>
#ifdef __cplusplus
extern "C" {
#endif

#define  B58VER 0x00

extern bool (*b58_sha256_impl)(void *, const void *, size_t);

extern bool b58tobin(void *bin, size_t *binsz, const char *b58, size_t b58sz);

extern bool b58enc(char *b58, size_t *b58sz, const void *bin, size_t binsz);
int base58_encode(const char *in, size_t in_len, char *out, size_t *out_len);
int base58_decode(const char *in, size_t in_len, char *out, size_t *out_len);

extern bool b58check_enc(char *b58c, size_t *b58c_sz, uint8_t ver, const void *data, size_t datasz);

bool GetBase58Addr(char *b58c, size_t *b58c_sz, uint8_t ver, const void *data, size_t datasz);


#ifdef __cplusplus
}
#endif

std::string GetBase58Addr(std::string key);

/**
 * @description: 校验base58地址是否合法
 * @param: base58地址
 * @return: 
 * true 合法
 * false 非法
 */
bool CheckBase58Addr(const std::string & base58Addr);
#endif
