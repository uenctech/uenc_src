#include "../net/http_server.h"
#include "ca_test.h"


void ca_register_http_callbacks();



void api_jsonrpc(const Request & req, Response & res);

void api_print_block(const Request & req, Response & res);
void api_info(const Request & req, Response & res);
void api_get_block(const Request & req, Response & res);
void api_get_block_hash(const Request & req, Response & res);
void api_get_block_by_hash(const Request & req, Response & res);

void api_get_tx_owner(const Request & req, Response & res);
void api_get_gas(const Request & req, Response & res);



void test_create_multi_tx(const Request & req, Response & res);
void api_get_db_key(const Request & req, Response & res);


nlohmann::json jsonrpc_test(const nlohmann::json & param);

nlohmann::json jsonrpc_get_height(const nlohmann::json & param);
nlohmann::json jsonrpc_get_balance(const nlohmann::json & param);
nlohmann::json jsonrpc_get_txids_by_height(const nlohmann::json & param);
nlohmann::json jsonrpc_get_tx_by_txid(const nlohmann::json & param);
nlohmann::json jsonrpc_create_tx_message(const nlohmann::json & param);
nlohmann::json jsonrpc_send_tx(const nlohmann::json & param);
nlohmann::json jsonrpc_send_multi_tx(const nlohmann::json & param);
nlohmann::json jsonrpc_get_avg_fee(const nlohmann::json & param);
nlohmann::json jsonrpc_generate_wallet(const nlohmann::json & param);
nlohmann::json jsonrpc_generate_sign(const nlohmann::json & param);











