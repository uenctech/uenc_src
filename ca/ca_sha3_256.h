




#ifndef SHA3_256_HPP_INCLUDE
#define SHA3_256_HPP_INCLUDE

#include <stdint.h>
#include "ca_sha3.h"

class sha3_256_t
{
public:
	typedef uint8_t sha3_256_item_t[SHA3_256_DIGEST_SIZE];

	sha3_256_t();
	void open(sha3_256_item_t *psha3_256_item);
	void update(const void * const p, uint64_t size);
	void close();

private:
	struct sha3_state ctx;
	sha3_256_item_t *psha3_256_item;
};

#endif
