#ifndef __CA_RIPEMD160_H__
#define __CA_RIPEMD160_H__

#include <stdint.h>

#define RIPEMD160_DIGEST_LENGTH 20

typedef struct _RIPEMD160_CTX {
    uint32_t total[2];    
    uint32_t state[5];    
    uint8_t buffer[64];   
} RIPEMD160_CTX;

void ripemd160_Init(RIPEMD160_CTX *ctx);
void ripemd160_Update(RIPEMD160_CTX *ctx, const void *input, uint32_t ilen);
void ripemd160_Final(uint8_t output[RIPEMD160_DIGEST_LENGTH],
		     RIPEMD160_CTX *ctx);
void ripemd160(const void *msg,
	       uint32_t msg_len,
	       uint8_t hash[RIPEMD160_DIGEST_LENGTH]);

#endif
