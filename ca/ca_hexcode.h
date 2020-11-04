#ifndef __CA_HEXCODE_H__
#define __CA_HEXCODE_H__
#include <stdbool.h>
#include <stdio.h>
#include "ca_cstr.h" 
#include <string>

#ifdef __cplusplus
extern "C" {
#endif

void encode_hex(char *hexstr, const void *p_, size_t len);
bool decode_hex(void *p, size_t max_len, const char *hexstr, size_t *out_len_);
cstring *str2hex(const void *in_buf, size_t in_len);
cstring *hex2str(const char *hexstr);
void hex_print(const unsigned char *hexstr, const int len);

std::string Str2Hex(const std::string & rawStr);
std::string Hex2Str(const std::string & hexStr);


#ifdef __cplusplus
}
#endif

#endif
