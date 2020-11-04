




#include <stdio.h>
#include "ca_sha3_256.h"


sha3_256_t::sha3_256_t()
{
	this->psha3_256_item = NULL;
}


void sha3_256_t::open(sha3_256_item_t *psha3_256_item)
{
	sha3_init(&this->ctx, SHA3_256_DIGEST_SIZE);

	this->psha3_256_item = psha3_256_item;
}


void sha3_256_t::update(const void * const p, uint64_t size)
{
	if (this->psha3_256_item == NULL) return;
	sha3_update(&this->ctx, (uint8_t *)p, size);
}


void sha3_256_t::close()
{
	sha3_final(&this->ctx, (uint8_t *)this->psha3_256_item);
	this->psha3_256_item = NULL;
}

