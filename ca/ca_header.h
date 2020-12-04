#ifndef _CA_HEADER_H_
#define _CA_HEADER_H_

#include "MagicSingleton.h"
#include "../common/config.h"
#include "../include/cJSON.h"
#include "../include/logging.h"
#include "../include/ScopeGuard.h"


#ifndef _CA_FILTER_FUN_
	#include "../include/net_interface.h"
    #include "ca_rocksdb.h"
    #include "proto/block.pb.h"
    #include "proto/transaction.pb.h"
#endif

#endif // !_CA_HEADER_H_