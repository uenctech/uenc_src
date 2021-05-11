#include "ca_block_http_callback.h"
#include "../net/httplib.h"
#include "../utils/singleton.h"
#include "../common/config.h"
#include "../include/ScopeGuard.h"
#include "ca_txhelper.h"
#include "ca_global.h"
#include "ca_rocksdb.h"
#include "MagicSingleton.h"
#include <functional>
#include <iostream>
#include <sstream>
#include <random>

CBlockHttpCallback::CBlockHttpCallback() : running_(false), url_("localhost"), port_(11190)
{
    
}

bool CBlockHttpCallback::AddBlock(const string& block)
{
    if (block.empty())
        return false;
        
    unique_lock<std::mutex> lck(mutex_);
    blocks_.push_back(block);
    cv_.notify_all();

    return true;
}

bool CBlockHttpCallback::AddBlock(const CBlock& block)
{
    string json = ToJson(block);
    return AddBlock(json);
}

bool CBlockHttpCallback::Start(const string& url, int port)
{
    url_ = url;
    port_ = port;
    running_ = true;
    work_thread_ = std::thread(std::bind(&CBlockHttpCallback::Work, this));
    work_thread_.detach();
 
    return true;
}

void CBlockHttpCallback::Stop()
{
    running_ = false;
}

bool CBlockHttpCallback::IsRunning()
{
    return running_;
}

int CBlockHttpCallback::Work()
{
    while (running_)
    {
        string currentBlock;
        {
            std::unique_lock<std::mutex> lck(mutex_);
            while (blocks_.empty())
            {
                cout << "Enter waiting for condition variable." << endl;
                cv_.wait(lck);
            }
            cout << "Handle the first block..." << endl;
            currentBlock = blocks_.front();
            blocks_.erase(blocks_.begin());
        }

        SendBlockHttp(currentBlock);
    }

    return true;
}

bool CBlockHttpCallback::SendBlockHttp(const string& block)
{
    //int port = 8080;
    //port = Singleton<Config>::get_instance()->GetHttpPort();
    httplib::Client client(url_, port_);
    auto res = client.Post("/add_block_callback", block, "application/json");
    if (res)
    {
        std::cout << res->status << std::endl;
        std::cout << res->get_header_value("Content-Type") << std::endl;
        std::cout << res->body << std::endl;
    }
    else
    {
        std::cout << "Client post failed" << std::endl;
        //std::cout << res.error() << std::endl;
    }

    return (bool)res;
}

string CBlockHttpCallback::ToJson(const CBlock& block)
{
    nlohmann::json jsonBlock;
    jsonBlock["block_hash"] = block.hash();
    jsonBlock["block_height"] = block.height();
    jsonBlock["block_time"] = block.time();

    CTransaction tx;
    for (auto & t : block.txs())
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
    for (auto& txOwner : owners)
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

        auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
        Transaction* txn = pRocksDb->TransactionInit();
        if (txn != nullptr)
        {
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
            pRocksDb->TransactionDelete(txn, true);
        }
    }

    for (auto & txOut : tx.vout())
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

    jsonBlock["tx"]["amount"] = to_string((double_t) amount / DECIMAL_NUM);

    string json = jsonBlock.dump(4);
    return json;
}

void CBlockHttpCallback::Test()
{
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<int> dist(1, 32767);

    stringstream stream;
    stream << "Test http callback, ID: " << dist(mt);
    string test_str = stream.str();
    AddBlock(test_str);
}

void CBlockHttpCallback::Test2()
{
    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if (txn == nullptr)
	{
        return ;
	}
	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, true);
	};

    unsigned int top = 0;
    if (pRocksDb->GetBlockTop(txn, top) != 0)
    {
        return ;
    }
    if (top >  20)
        top = 20;

    for (uint32_t height = top; height > 0; --height)
    {
        std::vector<std::string> blockHashs;
        if (pRocksDb->GetBlockHashsByBlockHeight(txn, height, blockHashs) != 0)
        {
            continue ;
        }

        for (auto& blkHash : blockHashs)
        {
            std::string blockStr;
            if (pRocksDb->GetBlockByBlockHash(txn, blkHash, blockStr) != 0)
            {
                continue ;
            }

            CBlock cblock;
            cblock.ParseFromString(blockStr);

            AddBlock(cblock);
        }
    }
}

int CBlockHttpCallback::Process(CBlockHttpCallback* self)
{
    //std::unique_lock<std::mutex> lck(mtx);
    int result = self->Work();

    return result;
}
