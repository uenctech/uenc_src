#ifndef __CA_COREDEFS_H__
#define __CA_COREDEFS_H__

#ifdef __cplusplus
extern "C" {
#endif

enum {

    MAX_BLOCK_SIZE      = 1000000,
};

enum {
    COIN            = 100000000LL,
};

enum bp_address_type {
    PUBKEY_ADDRESS = 0,
    SCRIPT_ADDRESS = 5,
    PRIVKEY_ADDRESS = 128,
    PUBKEY_ADDRESS_TEST = 111,
    SCRIPT_ADDRESS_TEST = 196,
    PRIVKEY_ADDRESS_TEST = 239,
};

enum chains {
    CHAIN_BITCOIN,
    CHAIN_TESTNET3,
    
    CHAIN_LAST = CHAIN_TESTNET3
};

struct chain_info {
    enum chains     chain_id;
    const char      *name;      /* "bitcoin", "testnet3" */

    unsigned char       addr_pubkey;
    unsigned char       addr_script;
    
    unsigned char       netmagic[5];
    const char      *genesis_hash;  /* hex string */
};

#ifdef __cplusplus
}
#endif

#endif
