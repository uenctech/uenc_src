#ifndef __CA_MESSAGE_H__
#define __CA_MESSAGE_H__

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "ca_cstr.h"

#include "proto/block.pb.h"
#include "proto/transaction.pb.h"


#ifdef __cplusplus
extern "C" {
#endif

#define P2P_HEADER_SIZE  (4 + 12 + 4 + 4)

struct p2p_message_hdr {
    unsigned char   netmagic[4];
    char        command[12];
    uint32_t    data_len;
    unsigned char   hash[4];
};

struct p2p_message {
    struct p2p_message_hdr  hdr;
    void            *data;
};

enum {
	ERROR = 0,
    MSG_TX = 1,
    MSG_BLOCK = 2,
};

cstring *message_str(const unsigned char netmagic[4], const char *command_, const void *data, uint32_t data_len);
cstring *txstr_append_signid(const void *data, uint32_t data_len,int verify_num,const std::vector<std::string>& ids=std::vector<std::string>() );
bool txstr_parse(void *data, uint32_t data_len, CTransaction &tx, std::vector<std::string>& ids, int * verify_num );
#ifdef __cplusplus
}
#endif

#endif
