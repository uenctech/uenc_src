#include "ca_http_api.h"
#include "ca_global.h"
#include "ca_transaction.h"
#include "Crypto_ECDSA.h"
#include "../net/peer_node.h"
#include "../utils/singleton.h"
#include "../utils/json.hpp"
#include "ca_synchronization.h"
#include "../net/httplib.h"
#include "../utils/string_util.h"
#include "../utils/base64.h"
#include "block.pb.h"
#include "MagicSingleton.h"
#include "ca_rocksdb.h"
#include <algorithm>
#include <string>
#include "../include/ScopeGuard.h"
#include "ca_txhelper.h"
#include "../include/net_interface.h"
#include "ca_base64.h"
#include "ca_message.h"
#include <sstream>
#include "ca_MultipleApi.h"
#include "ca_jcAPI.h"
#include "ca_txvincache.h"
#include "ca_txfailurecache.h"
#include "ca_global.h"
#include "../net/global.h"

void ca_register_http_callbacks()
{
    HttpServer::registerCallback("/", api_jsonrpc);
    HttpServer::registerCallback("/block", api_print_block);
    HttpServer::registerCallback("/info", api_info);
    HttpServer::registerCallback("/info_queue", api_info_queue);
    HttpServer::registerCallback("/get_block", api_get_block);
    HttpServer::registerCallback("/get_block_hash", api_get_block_hash);
    HttpServer::registerCallback("/get_block_by_hash", api_get_block_by_hash);
    HttpServer::registerCallback("/get_tx_owner", api_get_tx_owner);
    HttpServer::registerCallback("/test_create_multi_tx", test_create_multi_tx);
    HttpServer::registerCallback("/get_db_key", api_get_db_key);
    HttpServer::registerCallback("/get_gas", api_get_gas);
    HttpServer::registerCallback("/add_block_callback", add_block_callback_test);

    //json rpc=========
    HttpServer::registerJsonRpcCallback("jsonrpc_test", jsonrpc_test);
    HttpServer::registerJsonRpcCallback("get_height", jsonrpc_get_height);
    HttpServer::registerJsonRpcCallback("get_balance", jsonrpc_get_balance);
    HttpServer::registerJsonRpcCallback("get_txids_by_height", jsonrpc_get_txids_by_height);
    HttpServer::registerJsonRpcCallback("get_tx_by_txid", jsonrpc_get_tx_by_txid);
    HttpServer::registerJsonRpcCallback("get_tx_by_txid_and_nodeid", jsonrpc_get_tx_by_txid_and_nodeid);
    HttpServer::registerJsonRpcCallback("create_tx_message", jsonrpc_create_tx_message);
    HttpServer::registerJsonRpcCallback("send_tx", jsonrpc_send_tx);
    HttpServer::registerJsonRpcCallback("get_avg_fee", jsonrpc_get_avg_fee);
    HttpServer::registerJsonRpcCallback("generate_wallet", jsonrpc_generate_wallet);
    HttpServer::registerJsonRpcCallback("generate_sign", jsonrpc_generate_sign);
    HttpServer::registerJsonRpcCallback("send_multi_tx", jsonrpc_send_multi_tx);
    HttpServer::registerJsonRpcCallback("get_pending_transaction", jsonrpc_get_pending_transaction);
    HttpServer::registerJsonRpcCallback("get_failure_transaction", jsonrpc_get_failure_transaction);
    HttpServer::registerJsonRpcCallback("get_block_info_list", jsonrpc_get_block_info_list);
    HttpServer::registerJsonRpcCallback("confirm_transaction", jsonrpc_confirm_transaction);

    //Start http service 
    HttpServer::start();
}

void add_block_callback_test(const Request &req, Response &res)
{
    std::cout << "Receive callback request from Client: " << req.body << std::endl;
    res.set_content(req.body, "text/plain"); // "application/json"
}

nlohmann::json jsonrpc_test(const nlohmann::json &param)
{
    std::string param1 = param["param1"].get<std::string>();
    nlohmann::json ret;
    ret["result"]["echo param"] = param1;
    return ret;
}

void api_jsonrpc(const Request &req, Response &res)
{
    nlohmann::json ret;
    ret["jsonrpc"] = "2.0";
    try
    {
        auto json = nlohmann::json::parse(req.body);

        std::string method = json["method"];

        auto p = HttpServer::rpc_cbs.find(method);
        if (p == HttpServer::rpc_cbs.end())
        {
            ret["error"]["code"] = -32601;
            ret["error"]["message"] = "Method not found";
            ret["id"] = "";
        }
        else
        {
            auto params = json["params"];
            ret = HttpServer::rpc_cbs[method](params);
            try
            {
                ret["id"] = json["id"].get<int>();
            }
            catch (const std::exception &e)
            {
                ret["id"] = json["id"].get<std::string>();
            }
            ret["jsonrpc"] = "2.0";
        }
    }
    catch (const std::exception &e)
    {
        ret["error"]["code"] = -32700;
        ret["error"]["message"] = "Parse error";
        ret["id"] = "";
    }
    // std::cout << ret.dump(4) << std::endl;
    res.set_content(ret.dump(4), "application/json");
}

void api_get_db_key(const Request &req, Response &res)
{
    std::string key;
    if (req.has_param("key"))
    {
        key = req.get_param_value("key");
    }

    std::string value = MagicSingleton<Rocksdb>::GetInstance()->getKey(key);
    res.set_content(value, "text/plain");
}

void api_print_block(const Request &req, Response &res)
{
    int num = 100;
    if (req.has_param("num"))
    {
        num = atol(req.get_param_value("num").c_str());
    }

    std::cout << req.body << std::endl;

    std::string str = printBlocks(num);
    res.set_content(str, "text/plain");
}

void api_info_queue(const Request &req, Response &res)
{
    std::ostringstream oss;
    oss << "queue_read:" << global::queue_read.msg_queue_.size() << std::endl;
    oss << "queue_work:" << global::queue_work.msg_queue_.size() << std::endl;
    oss << "queue_write:" << global::queue_write.msg_queue_.size() << std::endl;
    oss << "\n"
        << std::endl;

    double total = .0f;
    oss << "------------------------------------------" << std::endl;
    for (auto &item : global::reqCntMap)
    {
        total += (double)item.second.second;
        oss.precision(3);
        oss << item.first << ": " << item.second.first << " size: " << (double)item.second.second / 1024 / 1024 << " MB" << std::endl;
    }
    oss << "------------------------------------------" << std::endl;
    oss << "Total: " << total / 1024 / 1024 << " MB" << std::endl;

    res.set_content(oss.str(), "text/plain");
}

void api_info(const Request &req, Response &res)
{

    std::ostringstream oss;

    oss << "queue----------:" << std::endl;
    oss << "queue_read:" << global::queue_read.msg_queue_.size() << std::endl;
    oss << "queue_work:" << global::queue_work.msg_queue_.size() << std::endl;
    oss << "queue_write:" << global::queue_write.msg_queue_.size() << std::endl;
    oss << "\n"
        << std::endl;

    oss << "amount----------:" << std::endl;
    for (auto &i : g_AccountInfo.AccountList)
    {
        uint64_t amount = CheckBalanceFromRocksDb(i.first);
        oss << i.first + ":" + std::to_string(amount) << std::endl;
        // std::string strPub;
        // g_AccountInfo.GetPubKeyStr(i.first.c_str(), strPub);
        // oss << base64Encode(strPub) << std::endl;
    }
    oss << "\n"
        << std::endl;

    std::vector<Node> pubNodeList = Singleton<PeerNode>::get_instance()->get_public_node();
    oss << "Public PeerNode size is: " << pubNodeList.size() << std::endl;
    oss << Singleton<PeerNode>::get_instance()->nodelist_info(pubNodeList);

    oss << std::endl
        << std::endl;

    std::vector<Node> nodeList = Singleton<PeerNode>::get_instance()->get_nodelist();
    oss << "PeerNode size is: " << nodeList.size() << std::endl;
    oss << Singleton<PeerNode>::get_instance()->nodelist_info(nodeList);

    res.set_content(oss.str(), "text/plain");
}

void api_get_block(const Request &req, Response &res)
{
    nlohmann::json blocks;

    int top = 0;
    if (req.has_param("top"))
    {
        top = atol(req.get_param_value("top").c_str());
    }
    int num = 0;
    if (req.has_param("num"))
    {
        num = atol(req.get_param_value("num").c_str());
    }
    std::string hashs;
    if (req.has_param("hashs"))
    {
        hashs = req.get_param_value("hashs");
    }
    // std::cout << "top:" << top << " num:" << num << std::endl;
    // std::cout << "hash:" << hashs << std::endl;
    std::vector<std::string> tmp;
    StringUtil::SplitString(hashs, tmp, "_");
    std::unordered_set<std::string> exist_hash(tmp.begin(), tmp.end());

    num = num > SYNC_NUM_LIMIT ? SYNC_NUM_LIMIT : num;
    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
    Transaction *txn = pRocksDb->TransactionInit();
    if (txn == NULL)
    {
        std::cout << "(api_get_block) TransactionInit failed !" << std::endl;
    }

    ON_SCOPE_EXIT
    {
        pRocksDb->TransactionDelete(txn, false);
    };

    if (top < 0 || num <= 0)
    {
        error("api_get_block top < 0||num <= 0");
        return;
    }

    unsigned int mytop;
    pRocksDb->GetBlockTop(txn, mytop);
    if (top > (int)mytop)
    {
        error("api_get_block begin > mytop");
        return;
    }
    int k = 0;
    int block_num = 0;
    for (auto i = top; i <= top + num; i++)
    {
        if (block_num >= num)
        {
            break;
        }
        std::vector<std::string> vBlockHashs;
        pRocksDb->GetBlockHashsByBlockHeight(txn, i, vBlockHashs);
        block_num += vBlockHashs.size();
        for (auto hash : vBlockHashs)
        {
            string strHeader;
            pRocksDb->GetBlockByBlockHash(txn, hash, strHeader);
            CBlock cblock;
            cblock.ParseFromString(strHeader);
            CTransaction *tx = cblock.mutable_txs(0);
            // std::cout << "block hash：" << hash << std::endl;
            for (int i = 0; i < tx->vin_size(); i++)
            {
                CTxin *vin = tx->mutable_vin(i);
                std::string utxo_hash = vin->mutable_prevout()->hash();
                std::string pub = vin->mutable_scriptsig()->pub();
                std::string addr = GetBase58Addr(pub);

                uint64_t amount = TxHelper::GetUtxoAmount(utxo_hash, addr);
                vin->mutable_prevout()->set_hash(addr + "_" + std::to_string(amount) + "_" + utxo_hash);
                // std::cout <<  i << ":vin:" << addr + "_" + std::to_string(amount) << std::endl;
            }
            strHeader = cblock.SerializeAsString();

            if (i == top)
            {
                auto result = exist_hash.find(cblock.hash());
                if (result == exist_hash.end())
                {
                    blocks[k++] = httplib::detail::base64_encode(strHeader);
                }
            }
            else
            {
                blocks[k++] = httplib::detail::base64_encode(strHeader);
            }
        }
    }

    res.set_content(blocks.dump(4), "application/json");
}

void api_get_block_hash(const Request &req, Response &res)
{
    nlohmann::json blocks;

    int top = 0;
    if (req.has_param("top"))
    {
        top = atol(req.get_param_value("top").c_str());
    }
    int num = 0;
    if (req.has_param("num"))
    {
        num = atol(req.get_param_value("num").c_str());
    }

    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
    Transaction *txn = pRocksDb->TransactionInit();
    if (txn == NULL)
    {
        std::cout << "(get_blkinfo_ser) TransactionInit failed !" << std::endl;
    }

    unsigned int my_top = 0;
    pRocksDb->GetBlockTop(txn, my_top);

    nlohmann::json block_hashs;
    int end = std::min(top, (int)my_top);
    int begin = (end - num > 0) ? (end - num) : 0;

    std::vector<std::string> hashs;
    for (auto i = begin; i <= end; i++)
    {
        std::vector<std::string> vBlockHashs;
        pRocksDb->GetBlockHashsByBlockHeight(txn, i, vBlockHashs); //i height
        // std::for_each(vBlockHashs.begin(), vBlockHashs.end(),
        //         [](std::string &s){ s = s.substr(0,HASH_LEN);}
        // );
        hashs.insert(hashs.end(), vBlockHashs.begin(), vBlockHashs.end());
    }

    for (size_t i = 0; i < hashs.size(); i++)
    {
        block_hashs[i] = hashs[i];
    }

    res.set_content(block_hashs.dump(4), "application/json");
}

void api_get_block_by_hash(const Request &req, Response &res)
{
    nlohmann::json blocks;

    std::string hashs;
    if (req.has_param("hashs"))
    {
        hashs = req.get_param_value("hashs");
    }
    std::vector<std::string> v_blocks;
    SplitString(hashs, v_blocks, "_");

    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
    Transaction *txn = pRocksDb->TransactionInit();
    if (txn == NULL)
    {
        std::cout << "(get_blkinfo_ser) TransactionInit failed !" << std::endl;
    }

    int i = 0;
    for (auto hash : v_blocks)
    {
        string strHeader;
        pRocksDb->GetBlockByBlockHash(txn, hash, strHeader);
        CBlock cblock;
        cblock.ParseFromString(strHeader);
        CTransaction *tx = cblock.mutable_txs(0);
        for (int i = 0; i < tx->vin_size(); i++)
        {
            CTxin *vin = tx->mutable_vin(i);
            std::string utxo_hash = vin->mutable_prevout()->hash();
            std::string pub = vin->mutable_scriptsig()->pub();
            std::string addr = GetBase58Addr(pub);

            uint64_t amount = TxHelper::GetUtxoAmount(utxo_hash, addr);
            vin->mutable_prevout()->set_hash(addr + "_" + std::to_string(amount) + "_" + utxo_hash);
        }
        strHeader = cblock.SerializeAsString();

        blocks[i++] = httplib::detail::base64_encode(strHeader);
    }
    res.set_content(blocks.dump(4), "application/json");
}

void api_get_tx_owner(const Request &req, Response &res)
{
    std::string tx_hash;
    if (req.has_param("tx_hash"))
    {
        tx_hash = req.get_param_value("tx_hash");
    }
    std::string str = TxHelper::GetTxOwnerStr(tx_hash);
    res.set_content(str, "text/plain");
}

void api_get_gas(const Request &req, Response &res)
{
    nlohmann::json data;
    int i = 0;
    std::map<std::string, uint64_t> fees = net_get_node_ids_and_fees();
    for (auto item : fees)
    {
        data[i++] = item.second;
    }

    res.set_content(data.dump(4), "application/json");
}

void test_create_multi_tx(const Request &req, Response &res)
{

    std::string from_addr;
    if (req.has_param("from_addr"))
    {
        from_addr = req.get_param_value("from_addr");
    }

    std::string to_addr;
    if (req.has_param("to_addr"))
    {
        to_addr = req.get_param_value("to_addr");
    }

    std::string amount;
    if (req.has_param("amount"))
    {
        amount = req.get_param_value("amount");
    }

    std::string fee_str = "1";
    if (req.has_param("fee"))
    {
        fee_str = req.get_param_value("fee");
    }

    std::stringstream ssminerfees;
    ssminerfees << fee_str;
    double fee;
    ssminerfees >> fee;

    std::vector<std::string> fromAddr;
    StringUtil::SplitString(from_addr, fromAddr, ",");

    std::vector<std::string> toAddr;
    StringUtil::SplitString(to_addr, toAddr, ",");

    std::vector<std::string> tmp;
    std::vector<uint64_t> v_amount;
    StringUtil::SplitString(amount, tmp, ",");
    for (auto i : tmp)
    {
        std::stringstream ssamount;
        ssamount << i;
        uint64_t amount;
        ssamount >> amount;
        v_amount.push_back(amount);
    }

    if (toAddr.size() > v_amount.size())
    {
        res.set_content("toAddr.size() > v_amount.size()", "text/plain");
        return;
    }

    std::map<std::string, int64_t> toAddrAmount;
    for (size_t i = 0; i < toAddr.size(); i++)
    {
        toAddrAmount[toAddr[i]] = v_amount[i] * DECIMAL_NUM;
    }

    TxHelper::DoCreateTx(fromAddr, toAddrAmount, 3, fee * DECIMAL_NUM);
}

nlohmann::json jsonrpc_get_height(const nlohmann::json &param)
{
    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
    Transaction *txn = pRocksDb->TransactionInit();
    if (txn == NULL)
    {
        std::cout << "(get_blkinfo_ser) TransactionInit failed !" << std::endl;
    }

    unsigned int top = 0;
    pRocksDb->GetBlockTop(txn, top);

    nlohmann::json ret;
    ret["result"]["height"] = std::to_string(top);
    return ret;
}

nlohmann::json jsonrpc_get_balance(const nlohmann::json &param)
{
    nlohmann::json ret;
    std::string address;
    try
    {
        if (param.find("address") != param.end())
        {
            address = param["address"].get<std::string>();
        }
        else
        {
            throw std::exception();
        }
    }
    catch (const std::exception &e)
    {
        ret["error"]["code"] = -32602;
        ret["error"]["message"] = "Invalid params";
        return ret;
    }

    if (!CheckBase58Addr(address))
    {
        ret["error"]["code"] = -1;
        ret["error"]["message"] = "address is invalid ";
        return ret;
    }

    uint64_t balance = CheckBalanceFromRocksDb(address.c_str());
    ret["result"]["balance"] = std::to_string((double)balance / DECIMAL_NUM);
    return ret;
}

nlohmann::json jsonrpc_get_txids_by_height(const nlohmann::json &param)
{
    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
    Transaction *txn = pRocksDb->TransactionInit();
    if (txn == NULL)
    {
        std::cout << "(jsonrpc_get_txids_by_height) TransactionInit failed !" << std::endl;
    }

    ON_SCOPE_EXIT
    {
        pRocksDb->TransactionDelete(txn, false);
    };

    nlohmann::json ret;
    std::regex pattern("^[1-9]\\d*|0$");
    bool bIsvalid = std::regex_match(param["height"].get<std::string>(), pattern);
    if (!bIsvalid)
    {
        ret["error"]["code"] = -1;
        ret["error"]["message"] = "height is invalid";
        return ret;
    }
    uint32_t height = 0;
    try
    {
        if (param.find("height") != param.end())
        {
            height = atoi(param["height"].get<std::string>().c_str());
        }
        else
        {
            throw std::exception();
        }
    }
    catch (const std::exception &e)
    {
        ret["error"]["code"] = -32602;
        ret["error"]["message"] = "Invalid params";
        return ret;
    }

    unsigned int top = 0;
    if (pRocksDb->GetBlockTop(txn, top) != 0)
    {
        ret["error"]["code"] = -3;
        ret["error"]["message"] = "Getting the block top is error!";
        return ret;
    }

    if (height > top)
    {
        ret["error"]["code"] = -4;
        ret["error"]["message"] = "height more than block top ";
        return ret;
    }

    std::vector<std::string> vBlockHashs;
    pRocksDb->GetBlockHashsByBlockHeight(txn, height, vBlockHashs);
    int k = 0;
    ret["result"] = nlohmann::json::array();

    for (auto hash : vBlockHashs)
    {
        string strHeader;
        pRocksDb->GetBlockByBlockHash(txn, hash, strHeader);

        CBlock cblock;
        cblock.ParseFromString(strHeader);

        for (int i = 0; i < cblock.txs_size(); i++)
        {
            CTransaction tx = cblock.txs(i);
            ret["result"][k++] = tx.hash();
        }
    }

    return ret;
}

nlohmann::json jsonrpc_get_tx_by_txid(const nlohmann::json &param)
{
    nlohmann::json ret;
    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
    Transaction *txn = pRocksDb->TransactionInit();
    if (txn == NULL)
    {
        std::cout << "(jsonrpc_get_txids_by_height) TransactionInit failed !" << std::endl;
    }

    ON_SCOPE_EXIT
    {
        pRocksDb->TransactionDelete(txn, false);
    };
    std::string hash;
    try
    {
        if (param.find("hash") != param.end())
        {
            hash = param["hash"].get<std::string>();
        }
        else
        {
            throw std::exception();
        }
    }
    catch (const std::exception &e)
    {
        ret["error"]["code"] = -32602;
        ret["error"]["message"] = "Invalid params";
        return ret;
    }

    if (hash.size() != 64)
    {
        ret["error"]["code"] = -1;
        ret["error"]["message"] = "hash is invalid";
        return ret;
    }

    std::string strTxRaw;
    if (pRocksDb->GetTransactionByHash(txn, hash, strTxRaw) != 0)
    {
        ret["error"]["code"] = -32000;
        ret["error"]["message"] = "not find txraw";
        return ret;
    }

    string blockhash;
    unsigned height;
    int stat;
    stat = pRocksDb->GetBlockHashByTransactionHash(txn, hash, blockhash);
    if (stat != 0)
    {
        ret["error"]["code"] = -32602;
        ret["error"]["message"] = "rocksdb error";
        return ret;
    }
    stat = pRocksDb->GetBlockHeightByBlockHash(txn, blockhash, height);
    if (stat != 0)
    {
        ret["error"]["code"] = -32602;
        ret["error"]["message"] = "rocksdb error";
        return ret;
    }
    ret["result"]["height"] = std::to_string(height);

    CTransaction utxoTx;
    utxoTx.ParseFromString(strTxRaw);

    nlohmann::json extra = nlohmann::json::parse(utxoTx.extra());
    std::string txType = extra["TransactionType"].get<std::string>();

    ret["result"]["hash"] = utxoTx.hash();
    ret["result"]["time"] = utxoTx.time();
    ret["result"]["type"] = txType;

    // std::vector<std::string> vins = TxHelper::GetTxOwner(hash);
    // int k = 0;
    // for(auto & addr:vins)
    // {
    //     ret["result"]["vin"][k++] = addr;
    //     ret["result"]
    // }

    for (int i = 0; i < utxoTx.vin_size(); i++)
    {
        CTxin vin = utxoTx.vin(i);
        std::string utxo_hash = vin.prevout().hash();
        std::string pub = vin.scriptsig().pub();
        std::string addr = GetBase58Addr(pub);

        uint64_t amount = TxHelper::GetUtxoAmount(utxo_hash, addr);
        ret["result"]["vin"][i]["address"] = addr;
        ret["result"]["vin"][i]["prev_hash"] = utxo_hash;
        ret["result"]["vin"][i]["output_index"] = 0;
        ret["result"]["vin"][i]["output_value"] = std::to_string((double)amount / DECIMAL_NUM);
    }

    for (int i = 0; i < utxoTx.vout_size(); i++)
    {
        CTxout txout = utxoTx.vout(i);
        ret["result"]["vout"][i]["address"] = txout.scriptpubkey();
        ret["result"]["vout"][i]["value"] = std::to_string((double)txout.value() / DECIMAL_NUM);
    }
    return ret;
}

nlohmann::json jsonrpc_get_tx_by_txid_and_nodeid(const nlohmann::json &param)
{
    nlohmann::json ret;

    std::string hash, nodeid;
    try
    {
        if (param.find("hash") == param.end() || param.find("nodeid") == param.end())
            throw std::exception();
        hash = param["hash"].get<std::string>();
        nodeid = param["nodeid"].get<std::string>();
    }
    catch (const std::exception &e)
    {
        ret["error"]["code"] = -32602;
        ret["error"]["message"] = "Invalid params";
        return ret;
    }

    if (hash.size() != 64)
    {
        ret["error"]["code"] = -1;
        ret["error"]["message"] = "hash is invalid";
        return ret;
    }
    //Send request to get transaction information 
    Node node;
    if (nodeid == Singleton<PeerNode>::get_instance()->get_self_id())
    {
        /*ret["error"]["code"] = -1;
        ret["error"]["message"] = "nodeid can not be self";
        return ret;*/
        node = Singleton<PeerNode>::get_instance()->get_self_node();
    }
    else if (!Singleton<PeerNode>::get_instance()->find_node(id_type(nodeid), node))
    {
        ret["error"]["code"] = -1;
        ret["error"]["message"] = "nodeid is invalid";
        return ret;
    }

    GetTransInfoReq req;
    req.set_txid(hash);
    req.set_nodeid( Singleton<PeerNode>::get_instance()->get_self_id() );
    net_com::send_message(node, req, net_com::Compress::kCompress_False, net_com::Encrypt::kEncrypt_False, net_com::Priority::kPriority_High_2);
    //Wait for 20 seconds to get the return information
    CTransaction utxo_tx;
    bool is_empty = true;
    int height = 0;
    for (int i = 0; i < 15; i++)
    {
        sleep(2); //Sleep for 2 seconds 
        {
            std::lock_guard<std::mutex> lock(global::g_mutex_transinfo);
            if (!global::g_is_utxo_empty)
            {
                global::g_is_utxo_empty = true;
                utxo_tx = global::g_trans_info.trans();
                is_empty = false;
                height = global::g_trans_info.height();
                break;
            }
        }
    }
    //If no data is returned within 30 seconds, the search fails 
    if (is_empty)
    {
        ret["error"]["code"] = -32000;
        ret["error"]["message"] = "not find txraw";
        return ret;
    }

    nlohmann::json extra = nlohmann::json::parse(utxo_tx.extra());

    ret["result"]["hash"] = utxo_tx.hash();
    ret["result"]["time"] = utxo_tx.time();
    ret["result"]["type"] = extra["TransactionType"].get<std::string>();
    ret["result"]["height"] = std::to_string(height);
    for (int i = 0; i < utxo_tx.vin_size(); i++)
    {
        CTxin vin = utxo_tx.vin(i);
        std::string utxo_hash = vin.prevout().hash();
        std::string pub = vin.scriptsig().pub();
        std::string addr = GetBase58Addr(pub);

        uint64_t amount = TxHelper::GetUtxoAmount(utxo_hash, addr);
        ret["result"]["vin"][i]["address"] = addr;
        ret["result"]["vin"][i]["prev_hash"] = utxo_hash;
        ret["result"]["vin"][i]["output_index"] = 0;
        ret["result"]["vin"][i]["output_value"] = std::to_string((double)amount / DECIMAL_NUM);
    }

    for (int i = 0; i < utxo_tx.vout_size(); i++)
    {
        CTxout txout = utxo_tx.vout(i);
        ret["result"]["vout"][i]["address"] = txout.scriptpubkey();
        ret["result"]["vout"][i]["value"] = std::to_string((double)txout.value() / DECIMAL_NUM);
    }
    return ret;
}

nlohmann::json jsonrpc_create_tx_message(const nlohmann::json &param)
{
    nlohmann::json ret;
    double fee = 0;
    std::vector<std::string> from_addrs;
    std::map<std::string, int64_t> toAddrAmount;
    std::string strFee;
    try
    {
        if (param.find("fee") != param.end())
        {
            strFee = param["fee"].get<std::string>();
        }
        else
        {
            throw std::exception();
        }
        istringstream issInputFee(strFee);
        issInputFee >> fee;
        fee = (fee + FIX_DOUBLE_MIN_PRECISION) * DECIMAL_NUM;

        if (param.find("from_addr") != param.end())
        {
            for (const auto &addr : param["from_addr"])
            {
                from_addrs.push_back(addr.get<std::string>());
            }
        }
        else
        {
            throw std::exception();
        }

        if (param.find("to_addr") != param.end())
        {
            int i = 0;
            for (const auto &addr : param["to_addr"])
            {
                if (param["to_addr"][i].find("addr") != param["to_addr"][i].end() && param["to_addr"][i].find("value") != param["to_addr"][i].end())
                {
                    std::regex pattern("[0-9]+(\\.[0-9]{1,6})?");
                    string search_value = addr["value"].get<std::string>();
                    bool bIsvalid = std::regex_match(search_value, pattern);
                    if (!bIsvalid)
                    {
                        ret["error"]["code"] = -32602;
                        ret["error"]["message"] = "The value is wrong or More than 6 decimal places";
                        return ret;
                    }
                    double value = 0;
                    istringstream issInputValue(addr["value"].get<std::string>());
                    issInputValue >> value;
                    value = (value + FIX_DOUBLE_MIN_PRECISION) * DECIMAL_NUM;
                    toAddrAmount[addr["addr"].get<std::string>()] = value;
                }
                else
                {
                    throw std::exception();
                }
                i++;
            }
        }
        else
        {
            throw std::exception();
        }
    }
    catch (const std::exception &e)
    {
        ret["error"]["code"] = -32602;
        ret["error"]["message"] = "Invalid params";
        return ret;
    }

    CTransaction outTx;
    int error_number = TxHelper::CreateTxMessage(from_addrs, toAddrAmount, g_MinNeedVerifyPreHashCount, fee, outTx, false);
    if (error_number != 0)
    {
        ret["error"]["code"] = -32000;
        ret["error"]["message"] = "create fail,error number:" + std::to_string(error_number);
        return ret;
    }

    std::string ser_original = outTx.SerializeAsString();
    std::string ser_original_base64 = base64Encode(ser_original);

    for (int i = 0; i < outTx.vin_size(); i++)
    {
        CTxin *txin = outTx.mutable_vin(i);
        ;
        txin->clear_scriptsig();
    }
    std::string serTx = outTx.SerializeAsString();
    std::string encodeStr = base64Encode(serTx);
    std::string encodeStrHash = getsha256hash(encodeStr);

    ret["result"]["tx_data"] = ser_original_base64;
    ret["result"]["tx_encode_hash"] = encodeStrHash;
    return ret;
}

nlohmann::json jsonrpc_send_tx(const nlohmann::json &param)
{
    nlohmann::json ret;
    std::string tx_data;
    std::string tx_signature;
    std::string public_key;
    std::string tx_encode_hash;

    try
    {
        if (param.find("tx_data") != param.end() &&
            param.find("tx_signature") != param.end() &&
            param.find("public_key") != param.end() &&
            param.find("tx_encode_hash") != param.end())
        {
            tx_data = param["tx_data"].get<std::string>();
            tx_signature = param["tx_signature"].get<std::string>();
            public_key = param["public_key"].get<std::string>();
            tx_encode_hash = param["tx_encode_hash"].get<std::string>();
        }
        else
        {
            throw std::exception();
        }
    }
    catch (const std::exception &e)
    {
        ret["error"]["code"] = -32602;
        ret["error"]["message"] = "Invalid params";
        return ret;
    }

    unsigned char tx_data_tmp[tx_data.size()] = {0};
    unsigned long tx_data_len = base64_decode((unsigned char *)tx_data.data(), tx_data.size(), tx_data_tmp);
    std::string tx_data_str((char *)tx_data_tmp, tx_data_len);

    unsigned char public_key_tmp[public_key.size()] = {0};
    unsigned long public_key_len = base64_decode((unsigned char *)public_key.data(), public_key.size(), public_key_tmp);
    std::string public_key_str((char *)public_key_tmp, public_key_len);

    unsigned char tx_signature_tmp[tx_signature.size()] = {0};
    unsigned long tx_signature_len = base64_decode((unsigned char *)tx_signature.data(), tx_signature.size(), tx_signature_tmp);
    std::string tx_signature_str((char *)tx_signature_tmp, tx_signature_len);

    if (tx_data_len == 0 || public_key_len == 0 || tx_signature_len == 0)
    {
        ret["error"]["code"] = -32602;
        ret["error"]["message"] = "Invalid base64 string";
        return ret;
    }

    CTransaction tx;
    tx.ParseFromString(tx_data_str);

    //signature 
    for (int i = 0; i < tx.vin_size(); i++)
    {
        auto vin = tx.mutable_vin(i);
        vin->mutable_scriptsig()->set_sign(tx_signature_str);
        vin->mutable_scriptsig()->set_pub(public_key_str);
    }

    std::string serTx = tx.SerializeAsString();

    TxMsg txMsg;
    txMsg.set_version(getVersion());

    std::string ID = Singleton<Config>::get_instance()->GetKID();
    txMsg.set_id(ID);
    txMsg.set_txencodehash(tx_encode_hash);
    txMsg.set_tx(serTx);

    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
    Transaction *txn = pRocksDb->TransactionInit();
    unsigned int top = 0;
    pRocksDb->GetBlockTop(txn, top);
    txMsg.set_top(top);

    std::string blockHash;
    pRocksDb->GetBestChainHash(txn, blockHash);
    txMsg.set_prevblkhash(blockHash);

    auto extra = nlohmann::json::parse(tx.extra());
    int needVerifyPreHashCount = extra["NeedVerifyPreHashCount"].get<int>();
    txMsg.set_trycountdown(CalcTxTryCountDown(needVerifyPreHashCount));

    // msgdata is for the convenience of calling the interface and has no practical meaning 
    auto msg = make_shared<TxMsg>(txMsg);

    std::string tx_hash;
    int error_num = DoHandleTx(msg, tx_hash);
    if (error_num != 0)
    {
        ret["error"]["code"] = -32000;
        ret["error"]["message"] = "create fail,error number:" + std::to_string(error_num);
        return ret;
    }
    ret["result"]["tx_hash"] = tx_hash;

    return ret;
}

nlohmann::json jsonrpc_send_multi_tx(const nlohmann::json &param)
{
    nlohmann::json ret;
    std::string tx_data;
    std::string tx_signature;
    std::string public_key;
    std::string tx_encode_hash;

    std::map<std::string, std::tuple<std::string, std::string>> sign;
    try
    {
        if (param.find("tx_data") != param.end() &&
            param.find("tx_encode_hash") != param.end())
        {
            tx_data = param["tx_data"].get<std::string>();
            tx_encode_hash = param["tx_encode_hash"].get<std::string>();
        }
        else
        {
            throw std::exception();
        }

        if (param.find("sign") != param.end())
        {
            int i = 0;
            for (const auto &item : param["sign"])
            {
                if (param["sign"][i].find("public_key") != param["sign"][i].end() && param["sign"][i].find("tx_signature") != param["sign"][i].end())
                {
                    std::string public_key = item["public_key"].get<std::string>();
                    std::string tx_signature = item["tx_signature"].get<std::string>();
                    std::string decode_pub = base64Decode(public_key);

                    std::cout << "public_key:" << public_key << std::endl;
                    std::cout << "tx_signature:" << tx_signature << std::endl;
                    std::cout << "addr:" << GetBase58Addr(decode_pub) << std::endl;

                    sign[GetBase58Addr(decode_pub)] = std::make_tuple(decode_pub, base64Decode(tx_signature));
                }
                else
                {
                    throw std::exception();
                }
                i++;
            }
        }
        else
        {
            throw std::exception();
        }
    }
    catch (const std::exception &e)
    {
        ret["error"]["code"] = -32602;
        ret["error"]["message"] = "Invalid params";
        return ret;
    }

    std::string tx_data_str = base64Decode(tx_data);

    CTransaction tx;
    tx.ParseFromString(tx_data_str);

    //signature 
    for (int i = 0; i < tx.vin_size(); i++)
    {
        auto vin = tx.mutable_vin(i);

        std::string addr = vin->mutable_scriptsig()->pub();
        auto result = sign.find(addr);
        if (result != sign.end())
        {
            vin->mutable_scriptsig()->set_sign(std::get<1>(sign[addr]));
            vin->mutable_scriptsig()->set_pub(std::get<0>(sign[addr]));
            std::cout << "sign:========" << std::endl;
            std::cout << "addr:" << addr << endl;
            std::cout << "pub:" << base64Encode(std::get<0>(sign[addr])) << endl;
            std::cout << "sign:" << base64Encode(std::get<1>(sign[addr])) << endl;
        }
    }

    std::string serTx = tx.SerializeAsString();

    TxMsg txMsg;
    txMsg.set_version(getVersion());

    std::string ID = Singleton<Config>::get_instance()->GetKID();
    txMsg.set_id(ID);
    txMsg.set_txencodehash(tx_encode_hash);
    txMsg.set_tx(serTx);

    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
    Transaction *txn = pRocksDb->TransactionInit();
    unsigned int top = 0;
    pRocksDb->GetBlockTop(txn, top);
    txMsg.set_top(top);

    std::string blockHash;
    pRocksDb->GetBestChainHash(txn, blockHash);
    txMsg.set_prevblkhash(blockHash);

    auto extra = nlohmann::json::parse(tx.extra());
    int needVerifyPreHashCount = extra["NeedVerifyPreHashCount"].get<int>();
    txMsg.set_trycountdown(CalcTxTryCountDown(needVerifyPreHashCount));

    auto msg = make_shared<TxMsg>(txMsg);

    std::string tx_hash;
    int error_num = DoHandleTx(msg, tx_hash);
    if (error_num != 0)
    {
        ret["error"]["code"] = -32000;
        ret["error"]["message"] = "create fail,error number:" + std::to_string(error_num);
        return ret;
    }
    ret["result"]["tx_hash"] = tx_hash;

    return ret;
}

nlohmann::json jsonrpc_get_avg_fee(const nlohmann::json &param)
{
    int64_t avg_fee_tmp = m_api::getAvgFee();
    double avg_fee = 0;
    if (avg_fee_tmp < 0)
    {
        avg_fee = 0;
    }
    else
    {
        avg_fee = (double)avg_fee_tmp / DECIMAL_NUM;
    }

    nlohmann::json ret;
    ret["result"]["avg_fee"] = std::to_string(avg_fee);
    return ret;
}

nlohmann::json jsonrpc_generate_wallet(const nlohmann::json &param)
{
    nlohmann::json ret;

    const int BUFF_SIZE = 128;
    char *out_private_key = new char[BUFF_SIZE]{0};
    int *out_private_key_len = new int{BUFF_SIZE};
    char *out_public_key = new char[BUFF_SIZE]{0};
    int *out_public_key_len = new int{BUFF_SIZE};
    char *out_bs58addr = new char[BUFF_SIZE]{0};
    int *out_bs58addr_len = new int{BUFF_SIZE};
    GenWallet_(out_private_key, out_private_key_len, out_public_key, out_public_key_len, out_bs58addr, out_bs58addr_len);

    char *base64_pri_key = new char[BUFF_SIZE]{0};
    char *base64_pub_key = new char[BUFF_SIZE]{0};
    base64_encode((unsigned char *)out_private_key, *out_private_key_len, (unsigned char *)base64_pri_key);
    base64_encode((unsigned char *)out_public_key, *out_public_key_len, (unsigned char *)base64_pub_key);
    std::string private_key(base64_pri_key);
    std::string public_key(base64_pub_key);
    std::string address(out_bs58addr);

    ret["result"]["private_key"] = private_key;
    ret["result"]["public_key"] = public_key;
    ret["result"]["address"] = address;

    delete[] out_private_key;
    delete[] out_private_key_len;
    delete[] out_public_key;
    delete[] out_public_key_len;
    delete[] out_bs58addr;
    delete[] out_bs58addr_len;
    delete[] base64_pub_key;
    delete[] base64_pri_key;

    return ret;
}

nlohmann::json jsonrpc_generate_sign(const nlohmann::json &param)
{
    nlohmann::json ret;
    std::string data;
    std::string private_key_base64;
    const int BUFF_SIZE = 128;
    try
    {
        if (param.find("data") != param.end() &&
            param.find("private_key") != param.end())
        {
            data = param["data"].get<std::string>();
            private_key_base64 = param["private_key"].get<std::string>();
            char *private_key = new char[BUFF_SIZE]{0};
            int size = base64_decode((unsigned char *)private_key_base64.c_str(), private_key_base64.size(), (unsigned char *)private_key);

            char *signature_msg = new char[BUFF_SIZE]{0};
            int *out_len = new int{BUFF_SIZE};
            GenSign_(private_key, size, data.c_str(), data.size(), signature_msg, out_len);

            std::string signature(signature_msg, *out_len);
            ret["result"]["message"] = signature;

            delete[] out_len;
            delete[] signature_msg;
            delete[] private_key;
        }
        else
        {
            throw std::exception();
        }
    }
    catch (const std::exception &e)
    {
        ret["error"]["code"] = -32602;
        ret["error"]["message"] = "Invalid params";
        return ret;
    }

    return ret;
}

nlohmann::json jsonrpc_get_pending_transaction(const nlohmann::json &param)
{
    nlohmann::json ret;

    std::string address;

    if (param.find("address") != param.end())
    {
        address = param["address"].get<std::string>();
    }

    if (!address.empty())
    {
        if (!CheckBase58Addr(address))
        {
            ret["error"]["code"] = -1;
            ret["error"]["message"] = "address is invalid";
            return ret;
        }
    }

    std::vector<TxVinCache::Tx> vectTxs;
    if (address.empty())
    {
        MagicSingleton<TxVinCache>::GetInstance()->GetAllTx(vectTxs);
    }
    else
    {
        MagicSingleton<TxVinCache>::GetInstance()->Find(address, vectTxs);
    }

    ret["result"]["total"] = vectTxs.size();

    int row = 0;
    for (auto iter = vectTxs.begin(); iter != vectTxs.end(); ++iter)
    {
        nlohmann::json tx;
        tx["hash"] = iter->txHash;

        int pos = 0;
        for (const auto &in : iter->vins)
        {
            tx["vin"][pos++] = in;
        }

        pos = 0;
        for (const auto &from : iter->from)
        {
            tx["from"][pos++] = from;
        }

        pos = 0;
        for (const auto &to : iter->to)
        {
            tx["to"][pos++] = to;
        }

        tx["amount"] = iter->amount;
        tx["timestamp"] = iter->timestamp;
        tx["broadstamp"] = iter->txBroadcastTime;
        tx["gap"] = iter->gas;

        pos = 0;
        for (const auto &amount : iter->toAmount)
        {
            tx["toAmount"][pos++] = amount;
        }

        tx["type"] = iter->type;

        ret["result"]["transaction"][row++] = tx;
    }

    return ret;
}

nlohmann::json jsonrpc_get_failure_transaction(const nlohmann::json &param)
{
    nlohmann::json ret;

    std::string address;
    if (param.find("address") != param.end())
    {
        address = param["address"].get<std::string>();
    }

    if (!address.empty())
    {
        if (!CheckBase58Addr(address))
        {
            ret["error"]["code"] = -1;
            ret["error"]["message"] = "address isinvalid";
            return ret;
        }
    }

    std::vector<TxVinCache::Tx> vectTxs;
    if (address.empty())
    {
        MagicSingleton<TxFailureCache>::GetInstance()->GetAll(vectTxs);
    }
    else
    {
        MagicSingleton<TxFailureCache>::GetInstance()->Find(address, vectTxs);
    }

    ret["result"]["total"] = vectTxs.size();

    int row = 0;
    for (auto iter = vectTxs.begin(); iter != vectTxs.end(); ++iter)
    {
        nlohmann::json tx;
        tx["hash"] = iter->txHash;

        int pos = 0;
        for (const auto &in : iter->vins)
        {
            tx["vin"][pos++] = in;
        }

        pos = 0;
        for (const auto &from : iter->from)
        {
            tx["from"][pos++] = from;
        }

        pos = 0;
        for (const auto &to : iter->to)
        {
            tx["to"][pos++] = to;
        }

        tx["amount"] = iter->amount;
        tx["timestamp"] = iter->timestamp;
        tx["broadstamp"] = iter->txBroadcastTime;
        tx["gap"] = iter->gas;

        pos = 0;
        for (const auto &amount : iter->toAmount)
        {
            tx["toAmount"][pos++] = amount;
        }

        tx["type"] = iter->type;

        ret["result"]["transaction"][row++] = tx;
    }

    return ret;
}

nlohmann::json jsonrpc_get_block_info_list(const nlohmann::json &param)
{
    nlohmann::json ret;
    uint32_t index = 0;
    uint32_t count = 0;
    uint32_t type = 0;

    std::regex pattern("^[1-9]\\d*|0$");
    if (param.find("index") != param.end())
    {
        bool bIsvalid = std::regex_match(param["index"].get<std::string>(), pattern);
        if (!bIsvalid)
        {
            ret["error"]["code"] = -32602;
            ret["error"]["message"] = "index Invalid params";
            return ret;
        }
        index = atoi(param["index"].get<std::string>().c_str());
    }
    if (param.find("count") != param.end())
    {

        bool bIsvalid = std::regex_match(param["count"].get<std::string>(), pattern);
        if (!bIsvalid)
        {
            ret["error"]["code"] = -32602;
            ret["error"]["message"] = "count Invalid params";
            return ret;
        }
        count = atoi(param["count"].get<std::string>().c_str());
    }
    if (param.find("type") != param.end())
    {
        bool bIsvalid = std::regex_match(param["type"].get<std::string>(), pattern);
        if (!bIsvalid)
        {
            ret["error"]["code"] = -32602;
            ret["error"]["message"] = "type Invalid params";
            return ret;
        }
        type = atoi(param["type"].get<std::string>().c_str());
    }

    if (type != 0)
    {
    }

    if (count == 0)
    {
        ret["error"]["code"] = -1;
        ret["error"]["message"] = "count is not equal zero！";
        return ret;
    }

    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
    Transaction *txn = pRocksDb->TransactionInit();
    if (txn == nullptr)
    {
        ret["error"]["code"] = -2;
        ret["error"]["message"] = "Rocksdb is error!";
        return ret;
    }

    ON_SCOPE_EXIT
    {
        pRocksDb->TransactionDelete(txn, true);
    };

    unsigned int top = 0;
    if (pRocksDb->GetBlockTop(txn, top) != 0)
    {
        ret["error"]["code"] = -3;
        ret["error"]["message"] = "Getting the block top is error!";
        return ret;
    }

    if (count > top)
    {
        ret["error"]["code"] = -4;
        ret["error"]["message"] = "count more than block top ";
        return ret;
    }

    uint64_t maxHeight = 0;
    uint64_t minHeight = 0;

    if (index == 0)
    {
        maxHeight = top;
        minHeight = maxHeight > count ? maxHeight - count : 0;
    }
    else
    {
        maxHeight = index > top ? top : index;
        minHeight = index > count ? index - count : 0;
    }

    for (uint32_t height = maxHeight; height > minHeight; --height)
    {
        std::vector<std::string> blockHashs;
        if (pRocksDb->GetBlockHashsByBlockHeight(txn, height, blockHashs) != 0)
        {
            continue;
        }

        std::vector<CBlock> blocks;
        for (auto &blkHash : blockHashs)
        {
            std::string blockStr;
            if (pRocksDb->GetBlockByBlockHash(txn, blkHash, blockStr) != 0)
            {
                continue;
            }

            CBlock cblock;
            cblock.ParseFromString(blockStr);
            if (cblock.hash().empty())
            {
                continue;
            }
            blocks.push_back(cblock);
        }
        // All blocks of a single height are in reverse chronological order 
        std::sort(blocks.begin(), blocks.end(), [](CBlock &a, CBlock &b) { return a.time() > b.time(); });

        nlohmann::json jsonHeight;
        for (auto &cblock : blocks)
        {
            nlohmann::json jsonBlock;
            jsonBlock["block_hash"] = cblock.hash();
            jsonBlock["block_height"] = cblock.height();
            jsonBlock["block_time"] = cblock.time();

            CTransaction tx;
            for (auto &t : cblock.txs())
            {
                CTxin txin0 = t.vin(0);
                if (txin0.scriptsig().sign() != std::string(FEE_SIGN_STR) &&
                    txin0.scriptsig().sign() != std::string(EXTRA_AWARD_SIGN_STR))
                {
                    tx = t;
                    break;
                }
            }

            jsonBlock["tx"]["hash"] = tx.hash();

            std::vector<std::string> owners = TxHelper::GetTxOwner(tx);
            for (auto &txOwner : owners)
            {
                jsonBlock["tx"]["from"].push_back(txOwner);
            }

            uint64_t amount = 0;
            nlohmann::json extra = nlohmann::json::parse(tx.extra());
            std::string txType = extra["TransactionType"].get<std::string>();
            if (txType == TXTYPE_REDEEM)
            {
                nlohmann::json txInfo = extra["TransactionInfo"].get<nlohmann::json>();
                std::string redeemUtxo = txInfo["RedeemptionUTXO"];

                std::string txRaw;
                if (pRocksDb->GetTransactionByHash(txn, redeemUtxo, txRaw) == 0)
                {
                    CTransaction utxoTx;
                    utxoTx.ParseFromString(txRaw);
                    std::string fromAddrTmp;

                    for (int i = 0; i < utxoTx.vout_size(); i++)
                    {
                        CTxout txout = utxoTx.vout(i);
                        if (txout.scriptpubkey() != VIRTUAL_ACCOUNT_PLEDGE)
                        {
                            fromAddrTmp = txout.scriptpubkey();
                            continue;
                        }
                        amount = txout.value();
                    }
                    if (!fromAddrTmp.empty())
                        jsonBlock["tx"]["to"].push_back(fromAddrTmp);
                }
            }

            for (auto &txOut : tx.vout())
            {
                if (owners.end() != find(owners.begin(), owners.end(), txOut.scriptpubkey()))
                {
                    continue;
                }
                else
                {
                    jsonBlock["tx"]["to"].push_back(txOut.scriptpubkey());
                    amount += txOut.value();
                }
            }

            jsonBlock["tx"]["amount"] = to_string((double_t)amount / DECIMAL_NUM);

            jsonHeight.push_back(jsonBlock);
        }
        ret["result"]["height"].push_back(jsonHeight);
    }

    return ret;
}

// Create: Confirm the transaction with transaction hash, 20210311,   Liu
nlohmann::json jsonrpc_confirm_transaction(const nlohmann::json &param)
{
    nlohmann::json ret;

    std::string txHash;

    if (param.find("tx_hash") != param.end())
    {
        txHash = param["tx_hash"].get<std::string>();
    }

    if (txHash.empty())
    {
        ret["error"]["code"] = -1;
        ret["error"]["message"] = "Please input tx hash.";
        return ret;
    }

    g_RpcTransactionConfirm.add(txHash);
    SendTransactionConfirm(txHash, ConfirmRpcFlag, 100);

    int i = 0;
    int count = 0;
    const int SUCCESS_COUNT = 60;
    const int WAIT_TIMES = 5;
    do
    {
        sleep(3);
        count = g_RpcTransactionConfirm.get_count(txHash);
    } while (count < SUCCESS_COUNT && i++ < WAIT_TIMES);

    count = g_RpcTransactionConfirm.get_count(txHash);
    std::vector<std::string> ids;
    g_RpcTransactionConfirm.get_ids(txHash,ids);
    ret["result"]["total"] = ids.size();
    nlohmann::json txid;
    int pos = 0;
    for(const auto &id:ids)
    {
        txid[pos++] = id;
    }

    if (count >= SUCCESS_COUNT)
    {
        ret["result"]["success"] = 1;
    }
    else
    {
        ret["result"]["success"] = 0;
    }
   // ret["result"]["count"] = count;
    ret["result"]["nodeid"] = txid;
    g_RpcTransactionConfirm.remove(txHash);

    return ret;
}
