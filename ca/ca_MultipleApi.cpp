#include "ca_MultipleApi.h"
#include "ca_txhelper.h"
#include "ca_phonetx.h"
#include "ca.h"
using std::cout;
using std::endl;
using std::string;

//namespace start
namespace m_api {

int is_version = -1;

void GetNodeServiceFee(const std::shared_ptr<GetNodeServiceFeeReq> &node_fee_req, GetNodeServiceFeeAck &node_fee_ack)
{
    uint64_t max_fee = 0, min_fee = 0, service_fee = 0;
    auto rdb_ptr = MagicSingleton<Rocksdb>::GetInstance();
    rdb_ptr->GetDeviceSignatureFee(service_fee);
    auto node_fee = net_get_node_ids_and_fees();
    if (node_fee.size() > 1) 
    {
        std::vector<uint64_t> node_fee_list;
        if (service_fee != 0) 
        {
            node_fee_list.push_back(service_fee);
        }
        for (auto v : node_fee)
        {
            if (v.second != 0 &&  v.second >= 1000  &&  v.second <= 100000) 
            {
                node_fee_list.push_back(v.second);
            }
        }

        sort(node_fee_list.begin(), node_fee_list.end());
        uint32_t fee_list_count = static_cast<uint32_t>(node_fee_list.size());

        std::cout << "fee_list_count 0-->" << node_fee_list[0] << std::endl;
        std::cout << "fee_list_count e-->" << node_fee_list[fee_list_count-1] << std::endl;
        std::cout << "fee_list_count--->" << fee_list_count << std::endl;
        //0.31 - 1
        uint32_t list_begin = fee_list_count * 0.51;
        uint32_t list_end = fee_list_count;

        std::cout << "list_begin =" << list_begin << std::endl;
        std::cout << "list_end =" << list_end << std::endl;

        list_begin = list_begin == 0 ? 1 : list_begin;
        list_end = list_end == 0 ? 1 : list_end;
        if (node_fee_list.size() == 1) 
        {
            min_fee = 0;
        } 
        else 
        {
            min_fee = node_fee_list[list_begin - 1];
        }
        max_fee = node_fee_list[list_end - 1];
        service_fee = (max_fee + min_fee)/2;
        cout<<"min_fee = " << min_fee <<endl;
        cout<<"max_fee = " << max_fee <<endl;
        cout<<"service_fee = "<< service_fee <<endl;
        if(min_fee == 0)
        {
            min_fee = 1000;
        }
        if( min_fee < 1|| max_fee > 100000)
        {
            node_fee_ack.set_code(-1);
            node_fee_ack.set_description("单节点签名费错误");
            cout<<"min_fee and  max_fee show "<<endl;
            return ;
        }
        std::cout << "min_fee=" << min_fee << std::endl;
        std::cout << "max_fee=" << max_fee << std::endl;
        std::cout << "service_fee=" << service_fee << std::endl;
    }

    std::cout << "node_fee.size() =" << node_fee.size() << std::endl;

    auto servicep_fee = node_fee_ack.add_service_fee_info();
    servicep_fee->set_max_fee(to_string(((double)max_fee)/DECIMAL_NUM));
    servicep_fee->set_min_fee(to_string(((double)min_fee)/DECIMAL_NUM));

    servicep_fee->set_service_fee(to_string(((double)service_fee)/DECIMAL_NUM));
    node_fee_ack.set_version(getVersion());
    node_fee_ack.set_code(0);
    node_fee_ack.set_description("获取成功");

    return;
}

void SetServiceFee(const std::shared_ptr<SetServiceFeeReq> &fee_req, SetServiceFeeAck &fee_ack) 
{
    using namespace std;
    fee_ack.set_version(getVersion());
    std::string dev_pass = fee_req->password();
    std::string hashOriPass = generateDeviceHashPassword(dev_pass);
    std::string targetPassword = Singleton<Config>::get_instance()->GetDevPassword();
    if (hashOriPass != targetPassword) 
    {
        fee_ack.set_code(-6);
        fee_ack.set_description("密码错误");
        return;
    }
    cout << "fee_req->service_fee() =" << fee_req->service_fee() << endl;
    double nodesignfee = stod(fee_req->service_fee());
    if(nodesignfee < 0.001 || nodesignfee > 0.1)
    {
        fee_ack.set_code(-7);
        fee_ack.set_description("滑动条数值显示错误");
        cout<<"return num show nodesignfee = "<<nodesignfee<<endl;
        return;
    }
    uint64_t service_fee = (uint64_t)(stod(fee_req->service_fee()) * DECIMAL_NUM);
    cout << "fee " << service_fee << endl;
    //设置手续费 SET
    auto rdb_ptr = MagicSingleton<Rocksdb>::GetInstance();

    auto status = rdb_ptr->SetDeviceSignatureFee(service_fee);
    if (!status) 
    {
        cout << ">>>>>>>>>>>>>>>>>>>>>>>> SetServiceFee4 =" << service_fee << endl;
        net_update_fee_and_broadcast(service_fee);
    }

    fee_ack.set_code(status);
    fee_ack.set_description("设置成功");
    
    return;
}


void GetServiceInfo(const std::shared_ptr<GetServiceInfoReq>& msg,GetServiceInfoAck &response_ack)
{
    int db_status = 0; //数据库返回状态 0为成功
    auto rdb_ptr = MagicSingleton<Rocksdb>::GetInstance();
    Transaction* txn = rdb_ptr->TransactionInit();
    if (txn == NULL) 
    {
        std::cout << "(GetBlockInfoAck) TransactionInit failed !" <<  __LINE__ << std::endl;
    }

    ON_SCOPE_EXIT 
    {
        rdb_ptr->TransactionDelete(txn, false);
    };

    unsigned int top = 0;
    rdb_ptr->GetBlockTop(txn, top);
    unsigned int height = top;

    //查询前100块数据
    std::vector<std::string> hash;
    db_status = rdb_ptr->GetBlockHashsByBlockHeight(txn, top, hash);
    if (db_status) 
    {
        std::cout << __LINE__ << std::endl;
    }
    std::string block_hash = hash[0];

    //查找100范围内所有块

    uint64_t max_fee, min_fee, def_fee, avg_fee, count;
    uint64_t temp_fee {0};

    min_fee = 1000;
    max_fee = 100000;

    {
        CBlockHeader block;
        std::string serialize_block;

        top = top > 100 ? 100 : top;
        for (int32_t i = top; i > 0; --i) 
        {
            db_status = rdb_ptr->GetBlockHeaderByBlockHash(txn, block_hash, serialize_block);
            if (db_status) 
            {
                std::cout << __LINE__ << std::endl;
            }
            block.ParseFromString(serialize_block);

            //解析块头
            string serialize_header;
            rdb_ptr->GetBlockByBlockHash(txn, block.hash(), serialize_header);
            CBlock cblock;
            cblock.ParseFromString(serialize_header);

            for (int32_t i_i = 0; i_i < cblock.txs_size(); i_i++) 
            {
                if (i_i == 1)
                 {
                    CTransaction tx = cblock.txs(i_i);
                    for (int32_t j = 0; j < tx.vout_size(); j++)
                    {
                        CTxout txout = tx.vout(j);
                        if (txout.value() > 0) 
                        {
                            temp_fee = txout.value();
                            cout << "temp_fee++ " << temp_fee << endl;
                            break;
                        }
                    }
                }
            }
            cout<<"temp_fee--"<<temp_fee<<endl;
            if (i == (int32_t)top) 
            {
                max_fee = temp_fee;
                min_fee = temp_fee;
                count = temp_fee;
            } 
            else 
            {
                max_fee = max_fee > temp_fee ? max_fee : temp_fee;
                //cout<<__LINE__<<endl;
               // cout<< "max_fee ="<<max_fee <<endl;
                min_fee = min_fee < temp_fee ? min_fee : temp_fee;
                //cout<< "min_fee ="<<min_fee <<endl;
                count += temp_fee;
            }
           // cout<<"i--"<<i<<endl;
            block_hash = block.prevhash();
        }
    }

    uint64_t show_service_fee = 0;
    db_status =rdb_ptr->GetDeviceSignatureFee(show_service_fee);
    if (db_status) 
    {
        std::cout << __LINE__ << std::endl;
    }

    if (!show_service_fee) 
    {
        //def_fee = (max_fee + min_fee)/2;
        def_fee = 0;
    }
    else
    {
        def_fee = show_service_fee;
    }
    cout << "max_fee =" << max_fee << endl;
    cout << "min_fee =" << min_fee << endl;
    cout <<"show_service_fee =" << show_service_fee <<endl;
    if (top == 100) 
    {
        cout << "top == 100 " << count << endl;
        avg_fee = count/100;
    } 
    else 
    {
        cout << "top != 100 " << top << endl;
        avg_fee = (max_fee + min_fee)/2;
    }

    min_fee = 1000;
    max_fee = 100000;

  
    response_ack.set_version(getVersion());
    response_ack.set_code(0);
    response_ack.set_description("获取设备成功");

    response_ack.set_mac_hash("test_hash"); //TODO
    response_ack.set_device_version(getEbpcVersion()); //TODO global
    response_ack.set_height(height);
    response_ack.set_sequence(msg->sequence_number());

    auto service_fee = response_ack.add_service_fee_info();
    service_fee->set_max_fee(to_string(((double)max_fee)/DECIMAL_NUM));
    service_fee->set_min_fee(to_string(((double)min_fee)/DECIMAL_NUM));
    service_fee->set_service_fee(to_string(((double)def_fee)/DECIMAL_NUM));
    service_fee->set_avg_fee(to_string(((double)avg_fee)/DECIMAL_NUM));

    std::cout << "max_fee = " << to_string(((double)max_fee)/DECIMAL_NUM) << std::endl;
    std::cout << "min_fee =" << to_string(((double)min_fee)/DECIMAL_NUM) << std::endl;
    std::cout << "def_fee =" << to_string(((double)def_fee)/DECIMAL_NUM) << std::endl;
    std::cout << "avg_fee =" << to_string(((double)avg_fee)/DECIMAL_NUM) << std::endl;

    return;
}

uint64_t getAvgFee()
{
    int db_status = 0; //数据库返回状态 0为成功
    auto rdb_ptr = MagicSingleton<Rocksdb>::GetInstance();
    Transaction* txn = rdb_ptr->TransactionInit();
    if (txn == NULL) 
    {
        std::cout << "(GetBlockInfoAck) TransactionInit failed !" <<  __LINE__ << std::endl;
    }

    ON_SCOPE_EXIT 
    {
        rdb_ptr->TransactionDelete(txn, false);
    };

    unsigned top = 0;
    rdb_ptr->GetBlockTop(txn, top);

    //查询前100块数据
    std::vector<std::string> hash;
    db_status = rdb_ptr->GetBlockHashsByBlockHeight(txn, top, hash);
    if (db_status) 
    {
        std::cout << __LINE__ << std::endl;
    }
    std::string block_hash = hash[0];

    //查找100范围内所有块

    uint64_t max_fee, min_fee, def_fee, avg_fee, count;
    uint64_t temp_fee {0};

    min_fee = 1000;
    max_fee = 100000;

    CBlockHeader block;
    std::string serialize_block;

    top = top > 100 ? 100 : top;
    for (int32_t i = top; i > 0; --i) 
    {
        db_status = rdb_ptr->GetBlockHeaderByBlockHash(txn, block_hash, serialize_block);
        if (db_status)
        {
            std::cout << __LINE__ << std::endl;
        }
        block.ParseFromString(serialize_block);

        //解析块头
        string serialize_header;
        rdb_ptr->GetBlockByBlockHash(txn, block.hash(), serialize_header);
        CBlock cblock;
        cblock.ParseFromString(serialize_header);

        for (int32_t i_i = 0; i_i < cblock.txs_size(); i_i++) 
        {
            //cout<<"cblock.txs_size()=" << cblock.txs_size()<<endl;
            if (i_i == 1) 
            {
                CTransaction tx = cblock.txs(i_i);
                for (int32_t j = 0; j < tx.vout_size(); j++) 
                {
                    CTxout txout = tx.vout(j);
                    if (txout.value() > 0) 
                    {
                        temp_fee = txout.value();
                        //cout<<"temp_fee = txout.value()=" <<temp_fee <<endl;
                        break;
                    }
                }
            }
        }

        if (i == (int32_t)top) 
        {
            cout<<"i="<<i <<endl;
            max_fee = temp_fee;
            min_fee = temp_fee;
            count = temp_fee;
        } 
        else 
        {
            max_fee = max_fee > temp_fee ? max_fee : temp_fee;
            min_fee = min_fee < temp_fee ? min_fee : temp_fee;
            count += temp_fee;
        }

        block_hash = block.prevhash();
    }

    uint64_t show_service_fee = 0;

    rdb_ptr->GetDeviceSignatureFee(show_service_fee);

    if (!show_service_fee)
    {
        //def_fee = (max_fee + min_fee)/2;
        def_fee = 0;
    } 
    else 
    {
        def_fee = show_service_fee;
        cout<<"show_service_fee="<<show_service_fee <<endl;
    }
    cout<<"getAvgFee()"<<endl;
    cout << "max_fee =" << max_fee << endl;
    cout << "min_fee =" << min_fee << endl;
    if (top == 100)
    {
        cout << "top == 100 " << count << endl;
        avg_fee = count/100;
    } 
    else
    {
        cout << "top != 100 " << count << endl;
        avg_fee = (max_fee + min_fee)/2;
    }
    (void)def_fee;
    return avg_fee;
}


void HandleGetServiceInfoReq(const std::shared_ptr<GetServiceInfoReq>& msg,const MsgData& msgdata) 
{
    GetServiceInfoAck getInfoAck;
    if (is_version) 
    {
        getInfoAck.set_version(getVersion());
        getInfoAck.set_code(is_version);
        getInfoAck.set_description("版本错误");

        net_send_message<GetServiceInfoAck>(msgdata, getInfoAck);
        return;
    }
   
    GetServiceInfo(msg,getInfoAck);
    net_send_message<GetServiceInfoAck>(msgdata, getInfoAck);
    return;
}

void GetPacketFee(const std::shared_ptr<GetPacketFeeReq> &packet_req, GetPacketFeeAck &packet_ack) 
{
    using namespace std;

    cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> GetPacketFee " << packet_req->public_net_ip() << endl;
    auto rdb_ptr = MagicSingleton<Rocksdb>::GetInstance();
    Transaction* txn = rdb_ptr->TransactionInit();
    if (txn == NULL) 
    {
        std::cout << "(GetBlockInfoAck) TransactionInit failed !" <<  __LINE__ << std::endl;
    }

    ON_SCOPE_EXIT {
        rdb_ptr->TransactionDelete(txn, false);
    };

    uint64_t packet_fee {0};
    rdb_ptr->GetDevicePackageFee(packet_fee);
  
    packet_ack.set_version(getVersion());
    packet_ack.set_code(0);
    packet_ack.set_description("获取成功");

    double d_fee = ((double)packet_fee) / DECIMAL_NUM;
    std::string s_fee= to_string(d_fee);
    packet_ack.set_packet_fee(s_fee);

    return;
}

template <typename T>
void AddBlockInfo(T &block_info_ack, CBlockHeader &block) 
{
    using std::cout;
    using std::endl;
    using std::string;

    auto rdb_ptr = MagicSingleton<Rocksdb>::GetInstance();
    Transaction* txn = rdb_ptr->TransactionInit();
    if (txn == NULL) 
    {
        std::cout << "(AddBlockInfo) TransactionInit failed !" << std::endl;
    }

    bool bRollback = false;
    ON_SCOPE_EXIT{
        rdb_ptr->TransactionDelete(txn, bRollback);
    };

    //解析块头
    string serialize_header;
    rdb_ptr->GetBlockByBlockHash(txn, block.hash(), serialize_header);
    CBlock cblock;
    cblock.ParseFromString(serialize_header);

    //开始拼接block_info_list
    auto blocks = block_info_ack.add_block_info_list();

    blocks->set_height(block.height());
    blocks->set_hash_merkle_root(cblock.merkleroot());
    blocks->set_hash_prev_block(cblock.prevhash());
    blocks->set_block_hash(cblock.hash());
    blocks->set_ntime(cblock.time());

    for (int32_t i = 0; i < cblock.txs_size(); i++) 
    {
        CTransaction tx = cblock.txs(i);

        //添加tx_info_list
        auto tx_info = blocks->add_tx_info_list();
        tx_info->set_tx_hash(tx.hash());

        for (int32_t j = 0; j < tx.signprehash_size(); j++) 
        {
            CSignPreHash signPreHash = tx.signprehash(j);
            char buf[2048] = {0};
            size_t buf_len = sizeof(buf);
            GetBase58Addr(buf, &buf_len, 0x00, signPreHash.pub().c_str(), signPreHash.pub().size());
            tx_info->add_transaction_signer(buf);
        }

        //添加vin_info
        for (int32_t j = 0; j < tx.vin_size(); j++) 
        {
            auto vin_list = tx_info->add_vin_list();

            CTxin txin = tx.vin(j);
            CScriptSig scriptSig = txin.scriptsig();
            int scriptSigLen = scriptSig.sign().size();
            char * hexStr = new char[scriptSigLen * 2 + 2]{0};

            std::string sign_str(scriptSig.sign().data(), scriptSig.sign().size());
            //cout << "sign_str " << sign_str << endl;
            if (sign_str == FEE_SIGN_STR || sign_str == EXTRA_AWARD_SIGN_STR) 
            {
                vin_list->set_script_sig(sign_str);
                //cout << "FEE_SIGN_STR " << FEE_SIGN_STR << endl;
            } 
            else 
            {
                char * hexStr = new char[scriptSigLen * 2 + 2]{0};
                encode_hex(hexStr, scriptSig.sign().data(), scriptSigLen);
                vin_list->set_script_sig(hexStr);
            }
            vin_list->set_pre_vout_hash(txin.prevout().hash());
            vin_list->set_pre_vout_index((uint64_t)txin.prevout().n());

            delete [] hexStr;
        }

        //添加vout_info
        for (int32_t j = 0; j < tx.vout_size(); j++) 
        {
            auto vout_list = tx_info->add_vout_list();

            CTxout txout = tx.vout(j);

            vout_list->set_script_pubkey(txout.scriptpubkey());
            double ld_value = (double)txout.value() / DECIMAL_NUM;
            std::string s_value(to_string(ld_value));

            vout_list->set_amount(s_value);
        }

        tx_info->set_nlock_time(tx.time());
        tx_info->set_stx_owner(TxHelper::GetTxOwnerStr(tx));
        tx_info->set_stx_owner_index(tx.n());
        tx_info->set_version(tx.version());
    } 
}

uint64_t GetBalanceByAddress(const std::string & address) 
{
    if (!address.size()) 
    {
        return 0;
    }

    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
    Transaction* txn = pRocksDb->TransactionInit();
    if (!txn) 
    {
        std::cout << "(CheckBalanceFromRocksDb) TransactionInit failed ! " << __LINE__ << std::endl;
    }

    ON_SCOPE_EXIT
    {
        pRocksDb->TransactionDelete(txn, false);
    };

    int64_t balance = 0;
    int r = pRocksDb->GetBalanceByAddress(txn, address, balance);
    if (r) 
    {
        return 0;
    }

    return balance;
}

void GetAmount(const std::shared_ptr<GetAmountReq> &amount_req, GetAmountAck &amount_ack) 
{
    bool enable = false;
    Singleton<Config>::get_instance()->GetEnable(kConfigEnabelTypeGetAmount, &enable);
    if (!enable) {
        amount_ack.set_version(getVersion());
        amount_ack.set_code(-1);
        amount_ack.set_description("Disable Get Amount");
        error("Disable Get Amount");
        return;
    }

    std::string addr = amount_req->address();
    uint64_t balance = GetBalanceByAddress(addr);
    std::string s_balance = to_string(((double)balance) / DECIMAL_NUM);

    amount_ack.set_version(getVersion());
    amount_ack.set_code(0);
    amount_ack.set_description("Get Amount Success");

    amount_ack.set_address(addr);
    amount_ack.set_balance(s_balance);
    //mData["fee"] = fee;

    return;
}

void BlockInfoReq(const std::shared_ptr<GetBlockInfoReq> &block_info_req, GetBlockInfoAck &block_info_ack) 
{
    using std::cout;
    using std::endl;

    int32_t height = block_info_req->height();
    int32_t count = block_info_req->count();

    cout << "height  ---> " << height << endl;
    cout << "count  ---> " << count << endl;
    int db_status;
    auto rdb_ptr = MagicSingleton<Rocksdb>::GetInstance();
    Transaction* txn = rdb_ptr->TransactionInit();
    if (txn == NULL) 
    {
        std::cout << "(GetBlockInfoAck) TransactionInit failed !" <<  __LINE__ << std::endl;
    }

    ON_SCOPE_EXIT {
        rdb_ptr->TransactionDelete(txn, false);
    };

    //FIXME [START]
    std::string bestChainHash;
    db_status = rdb_ptr->GetBestChainHash(txn, bestChainHash);
    if (db_status != 0) 
    {
        std::cout << __LINE__ << std::endl;
    }
    if (bestChainHash.size() == 0) 
    { 
        //使用默认值
        return;
    }
    //FIXME [END]

    uint32_t top;
    db_status = rdb_ptr->GetBlockTop(txn, top);
    if (db_status) 
    {
        std::cout << __LINE__ << std::endl;
    }

    //如为负 则为最高高度和所有块数
    height = height > static_cast<int32_t>(top) ? top : height;
    height = height < 0 ? top : height;
    count = count > height ? height : count;
    count = count < 0 ? top : count; //FIXME

    //通过高度查块hash
    std::vector<std::string> hash;

    //查找count范围内所有块
    CBlockHeader block;
    std::string serialize_block;
    block_info_ack.set_version(getVersion());
    block_info_ack.set_code(0);
    block_info_ack.set_description("success");
    block_info_ack.set_top(top);

    for (int64_t i = count, j = 0; i >= 0; --i, ++j) 
    {
        db_status = rdb_ptr->GetBlockHashsByBlockHeight(txn, height - j, hash);
        if (db_status) 
        {
            std::cout << __LINE__ << std::endl;
        }
        cout << "count-- " << hash.size() << endl;
        for (auto hs : hash)
        {
            db_status = rdb_ptr->GetBlockHeaderByBlockHash(txn, hs, serialize_block);
            if (db_status) 
            {
                std::cout << __LINE__ << std::endl;
            }
            block.ParseFromString(serialize_block);
            //add block_infos
            AddBlockInfo(block_info_ack, block);
        }
        hash.clear();
        //hash = block.prevhash();
        if (i == 0) 
        {
            break;
        }
    }

    cout << block_info_ack.version() << endl;
    cout << block_info_ack.code() << endl;
    cout << block_info_ack.description() << endl;
    cout << block_info_ack.top() << endl;
    cout << endl;
    // for (auto i = 0; i < block_info_ack.block_info_list().size(); ++i) {
    //     auto block_info = block_info_ack.block_info_list(i);
    //     cout << "height: " << block_info.height() << endl;
    //     cout << "hash_merkle_root: " << block_info.hash_merkle_root() << endl;
    //     cout << "hash_prev_block: " << block_info.hash_prev_block() << endl;
    //     cout << "block_hash: " << block_info.block_hash() << endl;
    //     cout << "ntime: " << block_info.ntime() << endl;
    //     cout << "tx_info-------" << endl;

    //     for (auto i_i = 0; i_i < block_info.tx_info_list().size(); ++i_i) {
    //         auto tx_info = block_info.tx_info_list(i_i);
    //         cout << "tx_hash: " << tx_info.tx_hash() << endl;
    //         cout << "signer-------" << endl;
    //         for (auto i_signer = 0; i_signer < tx_info.transaction_signer().size(); ++i_signer) {
    //             cout << "transaction_signer: " << tx_info.transaction_signer(i_signer) << endl;
    //         }

    //         cout << "vin_list-------" << endl;
    //         for (auto i_vin = 0; i_vin < tx_info.vin_list().size(); ++i_vin) {
    //             cout << "script_sig: " << tx_info.vin_list(i_vin).script_sig() << endl;
    //             cout << "pre_vout_hash: " << tx_info.vin_list(i_vin).pre_vout_hash() << endl;
    //             cout << "pre_vout_index: " << tx_info.vin_list(i_vin).pre_vout_index() << endl;
    //         }

    //         cout << "vout_list-------" << endl;
    //         for (auto i_vout = 0; i_vout < tx_info.vout_list().size(); ++i_vout) {
    //             cout << "script_pubkey: " << tx_info.vout_list(i_vout).script_pubkey() << endl;
    //             cout << "amount: " << tx_info.vout_list(i_vout).amount() << endl;
    //         }
    //         cout << "nlock_time: " << tx_info.nlock_time() << endl;
    //         cout << "stx_owner: " << tx_info.stx_owner() << endl;
    //         cout << "stx_owner_index:" << tx_info.stx_owner_index() << endl;
    //         cout << "version: " << tx_info.version() << endl;
    //     }
    // }

    return;
}

void GainDevPasswordReq(const std::shared_ptr<GetDevPasswordReq> &pass_req, GetDevPasswordAck &pass_ack)
{
	std::string password = pass_req->password();

    std::string version = "1";
    int code {0};
    std::string description = "成功";

    Singleton<StringUtil>::get_instance()->Trim(password, true, true);
    std::string originPass = generateDeviceHashPassword(password);
    std::string targetPass = Singleton<Config>::get_instance()->GetDevPassword();

    if (originPass != targetPass) 
    {
        pass_ack.set_version(getVersion());
        pass_ack.set_code(-2);
        pass_ack.set_description("密码错误");
        return;
    }

    pass_ack.set_version(getVersion());
    pass_ack.set_code(code);
    pass_ack.set_description(description);
    cout<< "g_AccountInfo.DefaultKeyBs58Addr= "<<g_AccountInfo.DefaultKeyBs58Addr <<endl; 
    pass_ack.set_address(g_AccountInfo.DefaultKeyBs58Addr);

    return;
}

void DevPasswordReq(const std::shared_ptr<SetDevPasswordReq> &pass_req, SetDevPasswordAck &pass_ack) 
{
    using std::cout;
    using std::endl;

    std::string old_pass = pass_req->old_pass();
    std::string new_pass = pass_req->new_pass();

    cout << "old-->" << old_pass << endl;
    cout << "new-->" << new_pass << endl;

    pass_ack.set_version(getVersion());
    if (old_pass.empty() || new_pass.empty()) 
    {
        pass_ack.set_code(-2);
        pass_ack.set_description("The password cannot be empty");
        return;
    }

    if (!isDevicePasswordValid(old_pass)) 
    {
        pass_ack.set_code(-3);
        pass_ack.set_description("oriPass invalid");
        return;
    }

    if (!isDevicePasswordValid(new_pass)) 
    {
        pass_ack.set_code(-4);
        pass_ack.set_description("newPass invalid");
        return;
    }

    if (old_pass == new_pass) 
    {
        pass_ack.set_code(-5);
        pass_ack.set_description("The new password cannot be the same as the old one");
        return;
    }

    std::string hashOriPass = generateDeviceHashPassword(old_pass);
    std::string targetPassword = Singleton<Config>::get_instance()->GetDevPassword();
    if (hashOriPass != targetPassword) 
    {
        pass_ack.set_code(-6);
        pass_ack.set_description("Incorrect old password");
        return;
    }

    std::string hashNewPass = generateDeviceHashPassword(new_pass);
    if (!Singleton<Config>::get_instance()->SetDevPassword(hashNewPass)) 
    {
        pass_ack.set_code(-7);
        pass_ack.set_description("Unknown error");
    } 
    else 
    {
        pass_ack.set_code(0);
        pass_ack.set_description("success");
    }

    return;
}

void GetTransactionInfo(const std::shared_ptr<GetAddrInfoReq> &addr_req, GetAddrInfoAck &addr_ack) 
{
    std::string addr = addr_req->address();
    uint32_t index = addr_req->index();
    uint32_t count = addr_req->count();

    int db_status = 0; //数据库返回状态 0为成功
    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
    Transaction* txn = pRocksDb->TransactionInit();
    if(!txn) 
    {
        std::cout << "(GetTransactionInfo) TransactionInit failed !" << std::endl;
    }

    std::string retString;
    ON_SCOPE_EXIT{
        pRocksDb->TransactionDelete(txn, false);
    };

    std::string bestChainHash;
    db_status = pRocksDb->GetBestChainHash(txn, bestChainHash);
    if (db_status) 
    {
        std::cout << __LINE__ << std::endl;
    }

    addr_ack.set_version(getVersion());
    if (!bestChainHash.size()) 
    {
        addr_ack.set_code(-1);
        addr_ack.set_description("Get tx info failed! No Data.");
        addr_ack.set_total(0);
        return;
    }

    std::vector<std::string> vTxHashs;
    db_status = pRocksDb->GetAllTransactionByAddreess(txn, addr, vTxHashs);
    if (!db_status) 
    {
        std::cout << __LINE__ << std::endl;
    }

    std::reverse(vTxHashs.begin(), vTxHashs.end());

    std::vector<std::string> vBlockHashs;
    for (auto strTxHash : vTxHashs) 
    {
        std::string blockHash;
        db_status = pRocksDb->GetBlockHashByTransactionHash(txn, strTxHash, blockHash);
        if (!db_status) 
        {
            std::cout << __LINE__ << std::endl;
        }
        vBlockHashs.push_back(blockHash);
    }

    vBlockHashs.erase(unique(vBlockHashs.begin(), vBlockHashs.end()), vBlockHashs.end());

    if (index >= vBlockHashs.size()) 
    {
        addr_ack.set_code(-2);
        addr_ack.set_description("Get tx info failed! Index out of range.");
        addr_ack.set_total(vBlockHashs.size());
        return;
    }

    size_t end = index + count;
    if (end > vBlockHashs.size()) 
    {
        end = vBlockHashs.size();
    }

    for (size_t i = index; i < end; i++) 
    {
        std::string hash = vBlockHashs[i];
        std::string serBlock;
        db_status = pRocksDb->GetBlockHeaderByBlockHash(txn, hash, serBlock);
        if (db_status) 
        {
            std::cout << __LINE__ << std::endl;
        }
        CBlockHeader block;
        block.ParseFromString(serBlock);
        //add block_infos
        AddBlockInfo(addr_ack, block);
    }

    addr_ack.set_code(0);
    addr_ack.set_description("Get tx info success");
    addr_ack.set_total(vBlockHashs.size());
    return;
}

void GetNodInfo(GetNodeInfoAck& node_ack) 
{
    using std::cout;
    using std::endl;

    node_ack.set_version(getVersion());
    node_ack.set_code(0);
    node_ack.set_description("获取成功");
    
    std::string node_string = Singleton<Config>::get_instance()->GetNodeInfo();
    if (node_string.empty()) 
    {
        node_ack.set_code(-1);
        node_ack.set_description("配置获取失败");
        return;
    }

    auto node_json = nlohmann::json::parse(node_string);

    for (auto it = node_json.begin(); it != node_json.end(); ++it) 
    {
        auto node_list = node_ack.add_node_list();
        auto info_json = it.value();
        for (auto info_it = info_json.begin(); info_it != info_json.end(); ++info_it) 
        {

            if (info_it.key() == "local") 
            {
                node_list->set_local(info_it.value());
            } 
            else 
            {
                auto param_json = info_it.value();
                for (auto param_it = param_json.begin(); param_it != param_json.end(); ++param_it) 
                {
                    auto node_info = node_list->add_node_info();
                    auto data_json = param_it.value();

                    for (auto data_it = data_json.begin(); data_it != data_json.end(); ++data_it) 
                    {

                        if (data_it.key() == "enable") 
                        {
                            node_info->set_enable(data_it.value());
                        } 
                        else if (data_it.key() == "ip") 
                        {
                            node_info->set_ip(data_it.value());
                        } 
                        else if (data_it.key() == "name") 
                        {
                            node_info->set_name(data_it.value());
                        } 
                        else if (data_it.key() == "port") 
                        {
                            node_info->set_port(data_it.value());
                        }

                        node_info->set_price(""); 
                    }
                }
            }
        }
    }

    std::vector<Node> nodes = net_get_public_node();
    for (int i = 0; i != node_ack.node_list_size(); ++i)
    {
        NodeList * nodeList = node_ack.mutable_node_list(i);
        for (int j = 0; j != nodeList->node_info_size(); ++j)
        {
            NodeInfos * nodeInfos = nodeList->mutable_node_info(j);
            for (auto & node : nodes)
            {
                if (node.is_public_node && IpPort::ipsz(node.public_ip) == nodeInfos->ip())
                {
                    nodeInfos->set_price(to_string((double)(node.package_fee) / DECIMAL_NUM));
                }
            }
        }
    }

    // 自身节点
    if (Singleton<Config>::get_instance()->GetIsPublicNode())
    {
        for (int i = 0; i != node_ack.node_list_size(); ++i)
        {
            NodeList * nodeList = node_ack.mutable_node_list(i);
            for (int j = 0; j != nodeList->node_info_size(); ++j)
            {
                NodeInfos * nodeInfos = nodeList->mutable_node_info(j);
                if (Singleton<Config>::get_instance()->GetLocalIP() == nodeInfos->ip())
                {
                    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
                    Transaction* txn = pRocksDb->TransactionInit();

                    ON_SCOPE_EXIT{
                        pRocksDb->TransactionDelete(txn, true);
                    };

                    uint64_t packageFee = 0;
                    pRocksDb->GetDevicePackageFee(packageFee);
                    nodeInfos->set_price(to_string((double)(packageFee) / DECIMAL_NUM));
                }
            }
        }
    }
    return;
}

void GetClientInfo(const std::shared_ptr<GetClientInfoReq> &clnt_req, GetClientInfoAck &clnt_ack) 
{
    using namespace std;

    auto phone_type = (ClientType)(clnt_req->phone_type());
    auto phone_lang = (ClientLanguage)clnt_req->phone_lang();
    std::string phone_version = clnt_req->phone_version();

    std::string sMinVersion = Singleton<Config>::get_instance()->GetClientVersion(phone_type);
    std::string sClientVersion = phone_version;

    std::vector<std::string> vMin;
    std::vector<std::string> vCleint;

    StringSplit(vMin, sMinVersion, ".");
    StringSplit(vCleint, sClientVersion, ".");

    bool isUpdate = true;
    int code {0};

    if (vMin.size() != vCleint.size()) 
    {
        code = -1;
        isUpdate = false;
    } 
    else 
    {
        for (size_t i = 0; i < vMin.size(); i++) 
        {
            std::string sMin = vMin[i];
            std::string sClient = vCleint[i];

            int nMin = atoi(sMin.c_str());
            int nClient = atoi(sClient.c_str());

            if (nMin < nClient) 
            {
                isUpdate = false;
                break;
            }
        }
    }

    clnt_ack.set_min_version(sMinVersion);

    std::string clientInfo = ca_clientInfo_read();

    clnt_ack.set_version(getVersion());
    clnt_ack.set_code(code);
    clnt_ack.set_description("Get Client Version Success");
    clnt_ack.set_is_update("0");

    if (isUpdate) 
    {
        clnt_ack.set_is_update("1");

        std::string sVersion;
        std::string sDesc;
        std::string sDownload;

        cout << "clientInfo " << clientInfo << endl;
        cout << "phone_type " << phone_type << endl;
        cout << "phone_lang " << phone_lang << endl;
        cout << "sVersion " << sVersion << endl;
        cout << "sDesc " << sDesc << endl;
        cout << "sDownload " << sDownload << endl;

        int r = ca_getUpdateInfo(clientInfo, phone_type, phone_lang, sVersion, sDesc, sDownload);
        if (!r) 
        {
            clnt_ack.set_code(1);
            clnt_ack.set_description("Get Client Version Success");

            clnt_ack.set_ver(sVersion);
            clnt_ack.set_desc(sDesc);
            clnt_ack.set_dl(sDownload);
        } 
        else 
        {
            clnt_ack.set_code(r);
            clnt_ack.set_description("Get Client Version failed");
        }

    }

    return;
}

void HandleGetDevPrivateKeyReq(const std::shared_ptr<GetDevPrivateKeyReq>& msg,  GetDevPrivateKeyAck& devprikey_ack )
{
    string passwd = msg->password();
    if(passwd.empty())
    {
        devprikey_ack.set_version(getVersion());
        devprikey_ack.set_code(-1);
        devprikey_ack.set_description("The password input error");
        return ;
    }
    string Bs58Addr = msg->bs58addr();
    if(Bs58Addr.empty())
    {
        devprikey_ack.set_version(getVersion());
        devprikey_ack.set_code(-2);
        devprikey_ack.set_description("The Bs58Addr input error");
        return;
    }

    std::string hashOriPass = generateDeviceHashPassword(passwd);
    std::string targetPassword = Singleton<Config>::get_instance()->GetDevPassword();
    if (hashOriPass != targetPassword) 
    {
        devprikey_ack.set_version(getVersion());
        devprikey_ack.set_code(-3);
        devprikey_ack.set_description("Incorrect old password");
        return;
    }
  
    std::map<std::string, account>::iterator iter;
    bool flag = false;
    iter = g_AccountInfo.AccountList.begin();
    while(iter != g_AccountInfo.AccountList.end())
    {
        if(strlen(Bs58Addr.c_str()) == iter->first.length() &&!memcmp(iter->first.c_str(), Bs58Addr.c_str(), iter->first.length()))
        {
            flag = true;
            break;
        }
        iter++;
    }
   
    if(flag)
    {
        devprikey_ack.set_version(getVersion());
        devprikey_ack.set_code(0);
        devprikey_ack.set_description("success!");

        DevPrivateKeyInfo  *pDevPrivateKeyInfo = devprikey_ack.add_devprivatekeyinfo();
        pDevPrivateKeyInfo->set_base58addr(Bs58Addr);   
        char keystore_data[2400] = {0};
        int keystoredata_len = sizeof(keystore_data);
        g_AccountInfo.GetKeyStore(Bs58Addr.c_str(), passwd.c_str(),keystore_data, keystoredata_len);    
        pDevPrivateKeyInfo->set_keystore(keystore_data);
        char out_data[1024] = {0};
        int data_len = sizeof(out_data);
        g_AccountInfo.GetMnemonic(Bs58Addr.c_str(), out_data, data_len);
        pDevPrivateKeyInfo->set_mnemonic(out_data);
    }
    else
    {
        devprikey_ack.set_version(getVersion());
        devprikey_ack.set_code(-4);
        devprikey_ack.set_description("addr is not exist");
    }    
}


void HandleCreatePledgeTxMsgReq(const std::shared_ptr<CreatePledgeTxMsgReq>& msg, const MsgData &msgdata)
{
    // 判断版本是否兼容
    CreatePledgeTxMsgAck createPledgeTxMsgAck; 
	if( 0 != IsVersionCompatible( getVersion() ) )
	{
        createPledgeTxMsgAck.set_version(getVersion());
		createPledgeTxMsgAck.set_code(-1);
		createPledgeTxMsgAck.set_description("version error!");
        net_send_message<CreatePledgeTxMsgAck>(msgdata, createPledgeTxMsgAck);
		error("HandleCreatePledgeTxMsgReq IsVersionCompatible");
		return ;
	}
   
    uint64_t gasFee = std::stod(msg->gasfees().c_str()) * DECIMAL_NUM;
    uint64_t amount = std::stod(msg->amt().c_str()) * DECIMAL_NUM;
    uint32_t needverifyprehashcount  = std::stoi(msg->needverifyprehashcount()) ;
  
    if(msg->addr().size()<= 0 || amount <= 0 || needverifyprehashcount < (uint32_t)g_MinNeedVerifyPreHashCount || gasFee <= 0)
    {
        createPledgeTxMsgAck.set_version(getVersion());
        createPledgeTxMsgAck.set_code(-2);
		createPledgeTxMsgAck.set_description("parameter  error!");
        net_send_message<CreatePledgeTxMsgAck>(msgdata, createPledgeTxMsgAck);
        error("CreatePledgeFromAddr parameter error!");
        return ;
    }

    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if ( txn == NULL )
	{
        createPledgeTxMsgAck.set_version(getVersion());
        createPledgeTxMsgAck.set_code(-3);
		createPledgeTxMsgAck.set_description("TransactionInit failed!");
        net_send_message<CreatePledgeTxMsgAck>(msgdata, createPledgeTxMsgAck);
        error("(HandleCreatePledgeTxMsgReq) TransactionInit failed !");
		return ;
	}

	ON_SCOPE_EXIT
    {
		pRocksDb->TransactionDelete(txn, false);
	};

    std::vector<std::string> fromAddr;
    fromAddr.push_back(msg->addr());

    std::map<std::string, int64_t> toAddr;
    toAddr[VIRTUAL_ACCOUNT_PLEDGE] = amount;

    CTransaction outTx;
    int ret = TxHelper::CreateTxMessage(fromAddr, toAddr,needverifyprehashcount , gasFee, outTx);

    for(int i = 0;i <outTx.vin_size();i++)
    {
        CTxin *txin = outTx.mutable_vin(i);
        txin->clear_scriptsig();
    }

	if(ret != 0)
	{
        createPledgeTxMsgAck.set_version(getVersion());
        createPledgeTxMsgAck.set_code(-5);
		createPledgeTxMsgAck.set_description("CreateTransaction  error!");
        net_send_message<CreatePledgeTxMsgAck>(msgdata, createPledgeTxMsgAck);
		error("CreateTransaction Error ...\n");
		return ;
	}

    nlohmann::json txInfo;
    txInfo["PledgeType"] = PLEDGE_NET_LICENCE;
    txInfo["PledgeAmount"] = amount;

    auto extra = nlohmann::json::parse(outTx.extra());
    extra["TransactionType"] = TXTYPE_PLEDGE;
	extra["TransactionInfo"] = txInfo;
	outTx.set_extra(extra.dump());
    std::string txData = outTx.SerializePartialAsString();

    size_t encodeLen = txData.size() * 2 + 1;
	unsigned char encode[encodeLen] = {0};
	memset(encode, 0, encodeLen);
	long codeLen = base64_encode((unsigned char *)txData.data(), txData.size(), encode);

    std::string encodeStr((char *)encode, codeLen);
	std::string txEncodeHash = getsha256hash(encodeStr);

    createPledgeTxMsgAck.set_version(getVersion());
    createPledgeTxMsgAck.set_code(0);
    createPledgeTxMsgAck.set_description("success");
    createPledgeTxMsgAck.set_txdata(encodeStr);
    createPledgeTxMsgAck.set_txencodehash(txEncodeHash);
    net_send_message<CreatePledgeTxMsgAck>(msgdata, createPledgeTxMsgAck);
    return ;
}

void HandlePledgeTxMsgReq(const std::shared_ptr<PledgeTxMsgReq>& msg, const MsgData &msgdata)
{
    TxMsgAck PledgeTxMsgAck;
    PledgeTxMsgAck.set_version(getVersion());
   
	//判断版本是否兼容
	if( 0 != IsVersionCompatible(msg->version() ) )
	{
		PledgeTxMsgAck.set_code(-101);
		PledgeTxMsgAck.set_message("Version incompatible!");
        net_send_message<TxMsgAck>(msgdata, PledgeTxMsgAck);
		return ;
	}

    if (msg->sertx().data() == nullptr || msg->sertx().size() == 0 || 
        msg->strsignature().data() == nullptr || msg->strsignature().size() == 0 || 
        msg->strpub().data() == nullptr || msg->strpub().size() == 0)
    {
   		PledgeTxMsgAck.set_code(-102);
		PledgeTxMsgAck.set_message("param error");
        net_send_message<TxMsgAck>(msgdata, PledgeTxMsgAck);
		return ;
    }

	// 将交易信息体，公钥，签名信息反base64
	unsigned char serTxCstr[msg->sertx().size()] = {0};
	unsigned long serTxCstrLen = base64_decode((unsigned char *)msg->sertx().data(), msg->sertx().size(), serTxCstr);
	std::string serTxStr((char *)serTxCstr, serTxCstrLen);

	CTransaction tx;
	tx.ParseFromString(serTxStr);

	unsigned char strsignatureCstr[msg->strsignature().size()] = {0};
	unsigned long strsignatureCstrLen = base64_decode((unsigned char *)msg->strsignature().data(), msg->strsignature().size(), strsignatureCstr);

	unsigned char strpubCstr[msg->strsignature().size()] = {0};
	unsigned long strpubCstrLen = base64_decode((unsigned char *)msg->strpub().data(), msg->strpub().size(), strpubCstr);

	for (int i = 0; i < tx.vin_size(); i++)
	{
		CTxin * txin = tx.mutable_vin(i);
		txin->mutable_scriptsig()->set_sign( std::string( (char *)strsignatureCstr, strsignatureCstrLen ) );
		txin->mutable_scriptsig()->set_pub( std::string( (char *)strpubCstr, strpubCstrLen ) );  
	}

    std::string serTx = tx.SerializeAsString();
	cJSON * root = cJSON_Parse( tx.extra().data() );
	cJSON * needVerifyPreHashCountTmp = cJSON_GetObjectItem(root, "NeedVerifyPreHashCount");
	int needVerifyPreHashCount = needVerifyPreHashCountTmp->valueint;
	cJSON_Delete(root);

	cstring *txstr = txstr_append_signid(serTx.c_str(), serTx.size(), needVerifyPreHashCount );
	std::string txstrtmp(txstr->str, txstr->len);
	
	TxMsg phoneToTxMsg;
    phoneToTxMsg.set_version(getVersion());
	phoneToTxMsg.set_tx(txstrtmp);
	phoneToTxMsg.set_txencodehash(msg->txencodehash());

	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if( txn == NULL )
	{
        PledgeTxMsgAck.set_code(-103);
		PledgeTxMsgAck.set_message(" TransactionInit failed !");
        net_send_message<TxMsgAck>(msgdata, PledgeTxMsgAck);
        return;
	}

	bool bRollback = true;
	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, bRollback);
	};

	unsigned int top = 0;
	pRocksDb->GetBlockTop(txn, top);	
	phoneToTxMsg.set_top(top);

	auto txmsg = make_shared<TxMsg>(phoneToTxMsg);
	HandleTx( txmsg, msgdata );
    cstr_free(txstr, true);
    return ;
}

void HandleCreateRedeemTxMsgReq(const std::shared_ptr<CreateRedeemTxMsgReq>& msg,const MsgData &msgdata)
{
    CreateRedeemTxMsgAck createRedeemTxMsgAck;
    // 判断版本是否兼容
	if( 0 != IsVersionCompatible( getVersion() ) )
	{
        createRedeemTxMsgAck.set_version(getVersion());
		createRedeemTxMsgAck.set_code(-1);
		createRedeemTxMsgAck.set_description("version error!");
        net_send_message<CreateRedeemTxMsgAck>(msgdata, createRedeemTxMsgAck);
        cout<<"HandleCreateRedeemTxMsgReq IsVersionCompatible"<<endl;
		error("HandleCreateRedeemTxMsgReq IsVersionCompatible");
		return ;
	}
    
    string fromAddr = msg->addr();
    uint64_t gasFee = std::stod(msg->gasfees().c_str()) * DECIMAL_NUM;
    uint64_t amount = std::stoi(msg->amt().c_str()) * DECIMAL_NUM;
    uint32_t needverifyprehashcount  = std::stoi(msg->needverifyprehashcount()) ;
    string txhash = msg->txhash();
   
    if(fromAddr.size()<= 0 || amount <= 0 || needverifyprehashcount < (uint32_t)g_MinNeedVerifyPreHashCount || gasFee <= 0||txhash.empty())
    {
        createRedeemTxMsgAck.set_version(getVersion());
        createRedeemTxMsgAck.set_code(-2);
		createRedeemTxMsgAck.set_description("parameter  error!");
        net_send_message<CreateRedeemTxMsgAck>(msgdata, createRedeemTxMsgAck);
        cout<<"HandleCreateRedeemTxMsgReq parameter error"<<endl;
        error("HandleCreateRedeemTxMsgReq parameter error!");
        return ;
    }

    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if ( txn == NULL )
	{
        createRedeemTxMsgAck.set_version(getVersion());
        createRedeemTxMsgAck.set_code(-3);
		createRedeemTxMsgAck.set_description("TransactionInit failed!");
        net_send_message<CreateRedeemTxMsgAck>(msgdata, createRedeemTxMsgAck);
        cout<<"(HandleCreateRedeemTxMsgReq) TransactionInit failed !"<<endl;
        error("(HandleCreateRedeemTxMsgReq) TransactionInit failed !");
		return ;
	}

	ON_SCOPE_EXIT
    {
		pRocksDb->TransactionDelete(txn, false);
	};
    // 查询账号是否已经质押资产
    std::vector<string> addresses;
    int db_status = pRocksDb->GetPledgeAddress(txn, addresses);
    if(db_status != 0)
    {
        createRedeemTxMsgAck.set_version(getVersion());
        createRedeemTxMsgAck.set_code(-4);
		createRedeemTxMsgAck.set_description("GetPledgeAddress error!");
        net_send_message<CreateRedeemTxMsgAck>(msgdata, createRedeemTxMsgAck);
        cout<<"HandleCreateRedeemTxMsgReq error!"<<endl;
        error("HandleCreateRedeemTxMsgReq error!");
        return ;
    }

    auto iter = find(addresses.begin(), addresses.end(), fromAddr);
    if( iter == addresses.end() )
    {
        createRedeemTxMsgAck.set_version(getVersion());
        createRedeemTxMsgAck.set_code(-5);
		createRedeemTxMsgAck.set_description("account:no Pledge !");
        net_send_message<CreateRedeemTxMsgAck>(msgdata, createRedeemTxMsgAck);
        cout<<"account: %s no Pledge !"<<endl;
        error( "account: %s no Pledge !", fromAddr.c_str() );
        return ;
    }

    CBlock cblock;
    string blockHeaderStr ;
    std::vector<string> utxoes;
    pRocksDb->GetPledgeAddressUtxo(txn,fromAddr, utxoes);
    if (utxoes.size() > 0)
    {
        std::string blockHash;
        pRocksDb->GetBlockHashByTransactionHash(txn, utxoes[0], blockHash); 
        int db_status1 = pRocksDb->GetBlockByBlockHash(txn,blockHash,blockHeaderStr);
    
        if(db_status1 != 0)
        {
            createRedeemTxMsgAck.set_version(getVersion());
            createRedeemTxMsgAck.set_code(-6);
		    createRedeemTxMsgAck.set_description("GetBlockByBlockHash error!");
            net_send_message<CreateRedeemTxMsgAck>(msgdata, createRedeemTxMsgAck);
            cout<<"HandleCreateRedeemTxMsgReq error!"<<endl;
            error("HandleCreateRedeemTxMsgReq error!");
            return ;
        }
     }
    cblock.ParseFromString(blockHeaderStr);

    std::string utxoStr = msg->txhash();
    if (utxoStr.empty())
    {
        for (int i = 0; i < cblock.txs_size(); i++)
        {
            CTransaction tx = cblock.txs(i);
            if (CheckTransactionType(tx) == kTransactionType_Tx)
            {
                for (int j = 0; j < tx.vout_size(); j++)
                {   
                    CTxout vout = tx.vout(j);
                    if (vout.scriptpubkey() == VIRTUAL_ACCOUNT_PLEDGE)
                    {
                        utxoStr = tx.hash();
                    }
                }
            }
        }
    }

    if (utxoStr.empty())
    {
        createRedeemTxMsgAck.set_version(getVersion());
        createRedeemTxMsgAck.set_code(-7);
		createRedeemTxMsgAck.set_description("blockHeaderStr error !");
        net_send_message<CreateRedeemTxMsgAck>(msgdata, createRedeemTxMsgAck);
        cout<<"(HandleCreateRedeemTxMsgReq) blockHeaderStr error !"<<endl;
        error("(HandleCreateRedeemTxMsgReq) blockHeaderStr error !");
		return ;   
    }

    std::vector<string> utxos;
    db_status = pRocksDb->GetPledgeAddressUtxo(txn, fromAddr, utxos);
    if (db_status != 0)
    {
        createRedeemTxMsgAck.set_version(getVersion());
        createRedeemTxMsgAck.set_code(-8);
		createRedeemTxMsgAck.set_description("GetPledgeAddressUtxo error!");
        net_send_message<CreateRedeemTxMsgAck>(msgdata, createRedeemTxMsgAck);
        cout<<"HandleCreateRedeemTxMsgReq error!"<<endl;
        error("HandleCreateRedeemTxMsgReq error!");
        return ;
    }

    auto utxoIter = find(utxos.begin(), utxos.end(), utxoStr);
    if (utxoIter == utxos.end())
    {
        createRedeemTxMsgAck.set_version(getVersion());
        createRedeemTxMsgAck.set_code(-9);
		createRedeemTxMsgAck.set_description("not found pledge UTXO!");
        net_send_message<CreateRedeemTxMsgAck>(msgdata, createRedeemTxMsgAck);
        cout<<"not found pledge UTXO!"<<endl;
        error("not found pledge UTXO!");
        return ;
    }
    CTransaction outTx;
    bool isTrue = FindUtxosFromRocksDb(fromAddr, fromAddr, 0, needverifyprehashcount, gasFee, outTx, utxoStr);
    for(int i = 0;i <outTx.vin_size();i++)
    {
        CTxin *txin = outTx.mutable_vin(i);
        txin->clear_scriptsig();
    }
    
	for (int i = 0; i != outTx.vin_size(); ++i)
	{
			CTxin * txin = outTx.mutable_vin(i);
			txin->clear_scriptsig();
	}

	outTx.clear_signprehash();
	outTx.clear_hash();

	if(!isTrue)
	{
        createRedeemTxMsgAck.set_version(getVersion());
        createRedeemTxMsgAck.set_code(-10);
		createRedeemTxMsgAck.set_description("CreateReleaseTransaction Error ...");
        net_send_message<CreateRedeemTxMsgAck>(msgdata, createRedeemTxMsgAck);
        cout<<"CreateReleaseTransaction Error ...\n"<<endl;
		error("CreateReleaseTransaction Error ...\n");
		return ;
	}

    uint64_t packageFee = 0;
    if ( 0 != pRocksDb->GetDevicePackageFee(packageFee) )
    {
        createRedeemTxMsgAck.set_version(getVersion());
        createRedeemTxMsgAck.set_code(-11);
		createRedeemTxMsgAck.set_description("GetDevicePackageFee Error ...");
        net_send_message<CreateRedeemTxMsgAck>(msgdata, createRedeemTxMsgAck);
        cout<<"(HandleCreateRedeemTxMsgReq) GetDevicePackageFee Error ...\n"<<endl;
		error("(HandleCreateRedeemTxMsgReq) GetDevicePackageFee Error ...\n");
		return ;
    }

    nlohmann::json txInfo;
    txInfo["RedeemptionUTXO"] = txhash;
    txInfo["ReleasePledgeAmount"] = amount;

	nlohmann::json extra;
    extra["fromaddr"] = fromAddr;
	extra["NeedVerifyPreHashCount"] = needverifyprehashcount;
	extra["SignFee"] = gasFee;
    extra["PackageFee"] = packageFee;   // 本节点代发交易需要打包费
	extra["TransactionType"] = TXTYPE_REDEEM;
    extra["TransactionInfo"] = txInfo;

	outTx.set_extra(extra.dump());
    std::string txData = outTx.SerializePartialAsString();

    size_t encodeLen = txData.size() * 2 + 1;
	unsigned char encode[encodeLen] = {0};
	memset(encode, 0, encodeLen);
	long codeLen = base64_encode((unsigned char *)txData.data(), txData.size(), encode);

    std::string encodeStr((char *)encode, codeLen);
	std::string txEncodeHash = getsha256hash(encodeStr);
    createRedeemTxMsgAck.set_version(getVersion());
    createRedeemTxMsgAck.set_code(0);
    createRedeemTxMsgAck.set_description("success");
    createRedeemTxMsgAck.set_txdata(encodeStr);
    createRedeemTxMsgAck.set_txencodehash(txEncodeHash);
    net_send_message<CreateRedeemTxMsgAck>(msgdata, createRedeemTxMsgAck);
}

void HandleRedeemTxMsgReq(const std::shared_ptr<RedeemTxMsgReq>& msg, const MsgData &msgdata )
{
    TxMsgAck txMsgAck; 
    txMsgAck.set_version(getVersion());

    // 判断版本是否兼容
	if( 0 != IsVersionCompatible(getVersion() ) )
	{
		txMsgAck.set_code(-101);
		txMsgAck.set_message("Version incompatible!");
        net_send_message<TxMsgAck>(msgdata, txMsgAck);
		return ;
	}
	// 将交易信息体，公钥，签名信息反base64
	unsigned char serTxCstr[msg->sertx().size()] = {0};
	unsigned long serTxCstrLen = base64_decode((unsigned char *)msg->sertx().data(), msg->sertx().size(), serTxCstr);
	std::string serTxStr((char *)serTxCstr, serTxCstrLen);

	CTransaction tx;
	tx.ParseFromString(serTxStr);

	unsigned char strsignatureCstr[msg->strsignature().size()] = {0};
	unsigned long strsignatureCstrLen = base64_decode((unsigned char *)msg->strsignature().data(), msg->strsignature().size(), strsignatureCstr);

	unsigned char strpubCstr[msg->strsignature().size()] = {0};
	unsigned long strpubCstrLen = base64_decode((unsigned char *)msg->strpub().data(), msg->strpub().size(), strpubCstr);

	for (int i = 0; i < tx.vin_size(); i++)
	{
		CTxin * txin = tx.mutable_vin(i);
		txin->mutable_scriptsig()->set_sign( std::string( (char *)strsignatureCstr, strsignatureCstrLen ) );
		txin->mutable_scriptsig()->set_pub( std::string( (char *)strpubCstr, strpubCstrLen ) );
	}

    std::string serTx = tx.SerializeAsString();
	cJSON * root = cJSON_Parse( tx.extra().data() );
	cJSON * needVerifyPreHashCountTmp = cJSON_GetObjectItem(root, "NeedVerifyPreHashCount");
	int needVerifyPreHashCount = needVerifyPreHashCountTmp->valueint;
	cJSON_Delete(root);

	cstring *txstr = txstr_append_signid(serTx.c_str(), serTx.size(), needVerifyPreHashCount );
	std::string txstrtmp(txstr->str, txstr->len);

	TxMsg phoneToTxMsg;
    phoneToTxMsg.set_version(getVersion());
	phoneToTxMsg.set_tx(txstrtmp);
	phoneToTxMsg.set_txencodehash(msg->txencodehash());

	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if( txn == NULL )
	{
		std::cout << "(HandleRedeemTxMsgReq) TransactionInit failed !" << std::endl;
	}

	bool bRollback = true;
	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, bRollback);
	};

	unsigned int top = 0;
	pRocksDb->GetBlockTop(txn, top);	
	phoneToTxMsg.set_top(top);

	auto txmsg = make_shared<TxMsg>(phoneToTxMsg);
	HandleTx( txmsg, msgdata );
    cstr_free(txstr, true);
    return; 
}
void HandleGetPledgeListReq(const std::shared_ptr<GetPledgeListReq>& req,  GetPledgeListAck& ack)
{
    ack.set_version(getVersion());
        
    std::string addr = req->addr();
    if (addr.length() == 0)
    {
        ack.set_code(-1);
        ack.set_description("The Bs58Addr input error");
        return;
    }

    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
    Transaction* txn = pRocksDb->TransactionInit();
    ON_SCOPE_EXIT 
    {
        pRocksDb->TransactionDelete(txn, true);
    };

    std::vector<string> utxoes;
    auto db_status = pRocksDb->GetPledgeAddressUtxo(txn, addr, utxoes);
    if (db_status != 0)
    {
        ack.set_code(-2);
        ack.set_description("Get pledge utxo error");
        return;
    }

    size_t size = utxoes.size();
    ack.set_total(size);
    if (size == 0)
    {
        ack.set_code(-3);
        ack.set_description("No pledge");
        return; 
    }

    reverse(utxoes.begin(), utxoes.end());

    uint32 index = req->index();
    uint32 count = req->count();

    if (index > (size - 1))
    {
        ack.set_code(-4);
        ack.set_description("index out of range");
        return;
    }

    size_t range = index + count;
    if (range >= size)
    {
        range = size;
    }

    for (size_t i = index; i < range; i++)
    {
        std::string strUtxo = utxoes[i];
        
        std::string serTxRaw;
        db_status = pRocksDb->GetTransactionByHash(txn, strUtxo, serTxRaw);
        if (db_status != 0)
        {
            error("Get pledge tx error");
            continue;
        }

        CTransaction tx;
        tx.ParseFromString(serTxRaw);

        if(tx.vout_size() != 2)
        {
            error("invalid tx");
            continue;
        }

        if (tx.hash().length() == 0)
        {
            error("Get pledge tx error");
            continue;
        }

        std::string strBlockHash;
        db_status = pRocksDb->GetBlockHashByTransactionHash(txn, tx.hash(), strBlockHash);
        if (db_status != 0)
        {
            error("Get pledge block hash error");
            continue;
        }

        std::string serBlock;
        db_status = pRocksDb->GetBlockHeaderByBlockHash(txn, strBlockHash, serBlock);
        if (db_status != 0)
        {
            error("Get pledge block error");
            continue;
        }

        CBlockHeader block;
        block.ParseFromString(serBlock);

        if (block.hash() == 0)
        {
            error("Block error");
            continue;
        }
        
        std::vector<std::string> txOwnerVec;
		SplitString(tx.txowner(), txOwnerVec, "_");

        if (txOwnerVec.size() == 0)
        {
            continue;
        }
        
        PledgeItem * pItem = ack.add_list();
        
        pItem->set_blockhash(block.hash());
        pItem->set_blockheight(block.height());
        pItem->set_utxo(strUtxo);
        pItem->set_time(tx.time());

        pItem->set_fromaddr(txOwnerVec[0]);

        for (int i = 0; i < tx.vout_size(); i++)
        {
            CTxout txout = tx.vout(i);
            if (txout.scriptpubkey() == VIRTUAL_ACCOUNT_PLEDGE)
            {
                pItem->set_toaddr(txout.scriptpubkey());
                pItem->set_amount(to_string((double_t)txout.value() / DECIMAL_NUM));
                break;
            }
        }
        pItem->set_detail("");        
    }

    ack.set_code(0);
    ack.set_description("success");
}


void HandleGetTxInfoListReq(const std::shared_ptr<GetTxInfoListReq>& req, GetTxInfoListAck & ack)
{
    ack.set_version(getVersion());
        
    std::string addr = req->addr();
    if (addr.length() == 0)
    {
        ack.set_code(-1);
        ack.set_description("The Bs58Addr input error");
        return;
    }

    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
    Transaction* txn = pRocksDb->TransactionInit();
    ON_SCOPE_EXIT 
    {
        pRocksDb->TransactionDelete(txn, true);
    };
    
    std::vector<std::string> txHashs;
    auto db_status = pRocksDb->GetAllTransactionByAddreess(txn, addr, txHashs);
    if (db_status != 0)
    {
        ack.set_code(-2);
        ack.set_description("Get strTxHash error");
        return;
    }

    size_t size = txHashs.size();
    if (size == 0)
    {
        ack.set_code(-3);
        ack.set_description("No txhash");
        return; 
    }

    // 去重
    std::set<std::string> txHashsSet(txHashs.begin(), txHashs.end());
    txHashs.assign(txHashsSet.begin(), txHashsSet.end());

    // 重新賦值
    txHashs.clear();
    for (auto & txHash : txHashsSet)
    {
        txHashs.push_back(txHash);
    }

    std::sort(txHashs.begin(), txHashs.end(), [&pRocksDb, &db_status, &txn](const std::string & aTxHash, const std::string & bTxHash){

        std::string serTxRaw;
        db_status = pRocksDb->GetTransactionByHash(txn, aTxHash, serTxRaw);
        if (db_status != 0)
        {
            return true;
        }

        CTransaction aTx;
        aTx.ParseFromString(serTxRaw);

        db_status = pRocksDb->GetTransactionByHash(txn, bTxHash, serTxRaw);
        if (db_status != 0)
        {
            return true;
        }

        CTransaction bTx;
        bTx.ParseFromString(serTxRaw);

        return aTx.time() > bTx.time();
    });

    size = txHashs.size();
    uint32 index = req->index();
    uint32 count = 0;

    if (index > (size - 1))
    {
        ack.set_code(-4);
        ack.set_description("index out of range");
        return;
    }

    size_t total = size;
    size_t i = index;
    for ( ; i < size; i++)
    {
        std::string strTxHash = txHashs[i];

        std::string serTxRaw;
        db_status = pRocksDb->GetTransactionByHash(txn, strTxHash, serTxRaw);
        if (db_status != 0)
        {
            ack.set_code(-5);
            ack.set_description("Get tx raw error");
            return;
        }

        CTransaction tx;
        tx.ParseFromString(serTxRaw);

        if (tx.hash().length() == 0)
        {
            continue;
        }

        std::string strBlockHash;
        db_status = pRocksDb->GetBlockHashByTransactionHash(txn, strTxHash, strBlockHash);
        if (db_status != 0)
        {
            continue;
        }

        std::string serBlockRaw;
        db_status = pRocksDb->GetBlockByBlockHash(txn, strBlockHash, serBlockRaw);
        if (db_status != 0)
        {
            continue;
        }

        CBlock cblock;
        cblock.ParseFromString(serBlockRaw);

        uint64_t amount = 0;
        CTxin txIn0 = tx.vin(0);
        if (txIn0.scriptsig().sign() == std::string(FEE_SIGN_STR) || 
            txIn0.scriptsig().sign() == std::string(EXTRA_AWARD_SIGN_STR))
        {
            // 奖励账号

            // 判断是否是奖励交易中的交易发起方账号
            bool isOriginator = false;
            uint64_t txOutAmount = 0;

            CTransaction tmpTx;
            for (auto & t : cblock.txs())
            {
                CTxin tmpTxIn0 = t.vin(0);
                if (tmpTxIn0.scriptsig().sign() != std::string(FEE_SIGN_STR) &&
                    tmpTxIn0.scriptsig().sign() != std::string(EXTRA_AWARD_SIGN_STR))
                {
                    tmpTx = t;
                }
            }

            for (auto & out : tx.vout())
            {
                if (out.scriptpubkey() == addr && out.value() == 0)
                {
                    isOriginator = true;
                    break;
                }
            }
            
            if (isOriginator)
            {
                // 如果是发起方的话不进入统计
                continue;
            }

            TxInfoItem * pItem = ack.add_list();
            pItem->set_txhash(tx.hash());
            pItem->set_time(tx.time());
            
            if (txIn0.scriptsig().sign() == std::string(FEE_SIGN_STR))
            {
                pItem->set_type(TxInfoType_Gas);
            }
            else if (txIn0.scriptsig().sign() == std::string(EXTRA_AWARD_SIGN_STR))
            {
                pItem->set_type(TxInfoType_Award);
            }

            for (auto & out : tx.vout())
            {
                if (out.scriptpubkey() == addr)
                {
                    txOutAmount = out.value();
                    break;
                }    
            }

            amount += txOutAmount;
            pItem->set_amount(to_string((double_t)amount / DECIMAL_NUM));
            count++;
            if (count >= req->count())
            {
                break;
            }
            
        }
        else
        {
            // 主交易
            TxInfoItem * pItem = ack.add_list();
            pItem->set_txhash(tx.hash());
            pItem->set_time(tx.time());

            std::vector<std::string> owners = TxHelper::GetTxOwner(tx);
            if (owners.size() == 1 && tx.vout_size() == 2)
            {
                // 质押和解除质押
                uint64_t gas = 0;
                for (auto & tmpTx : cblock.txs())
                {
                    CTxin txIn0 = tmpTx.vin(0);
                    if (txIn0.scriptsig().sign() == std::string(FEE_SIGN_STR))
                    {
                        for (auto & tmpTxOut : tmpTx.vout())
                        {
                            if (tmpTxOut.scriptpubkey() != addr)
                            {
                                gas += tmpTxOut.value();
                            }
                        }
                    }
                }

                std::vector<std::string> txOwnerVec;
                SplitString(tx.txowner(), txOwnerVec, "_");
                if (txOwnerVec.size() == 0)
                {
                    continue;
                }

                if (owners[0] != txOwnerVec[0] && txOwnerVec[0].length() != 0)
                {
                    owners[0] = txOwnerVec[0];
                }

                if (owners[0] == addr && 
                    owners[0] == tx.vout(0).scriptpubkey() && 
                    tx.vout(0).scriptpubkey() == tx.vout(1).scriptpubkey())
                {
                    // 解除质押
                    pItem->set_type(TxInfoType_Redeem);
                    pItem->set_amount(to_string(((double_t)tx.vout(0).value() - gas) / DECIMAL_NUM));

                    count++;
                    if (count >= req->count())
                    {
                        break;
                    }
                    
                    continue;
                }
                else if (owners[0] == addr && 
                        (tx.vout(0).scriptpubkey() == VIRTUAL_ACCOUNT_PLEDGE || tx.vout(0).scriptpubkey() == VIRTUAL_ACCOUNT_PLEDGE))
                {
                    // 质押资产
                    pItem->set_type(TxInfoType_Pledge);
                    pItem->set_amount(to_string(((double_t)tx.vout(0).value() + gas) / DECIMAL_NUM));

                    count++;
                    if (count >= req->count())
                    {
                        break;
                    }
                    
                    continue;
                }
            }
            
            // 是否是发起方
            bool isOriginator = false;
            for (auto & tmpTxIn : tx.vin())
            {
                char buf[2048] = {0};
                size_t buf_len = sizeof(buf);
                GetBase58Addr(buf, &buf_len, 0x00, tmpTxIn.scriptsig().pub().c_str(), tmpTxIn.scriptsig().pub().size());
                if (std::string(buf) == addr)
                {
                    isOriginator = true;
                    break;
                }
            }
            pItem->set_type(isOriginator ? TxInfoType_Originator : TxInfoType_Receiver);

            for (auto & tmpTxOut : tx.vout())
            {
                if (isOriginator)
                {
                    //  发起方的话找非发起方的交易额
                    if (tmpTxOut.scriptpubkey() != addr)
                    {
                        amount += tmpTxOut.value();
                    }  
                }
                else
                {
                    // 非发起方的话直接用该交易额
                    if (tmpTxOut.scriptpubkey() == addr)
                    {
                        amount = tmpTxOut.value();
                    }    
                }
            }

            pItem->set_amount(to_string((double_t)amount / DECIMAL_NUM));

            count++;
            if (count >= req->count())
            {
                break;
            }

            if (isOriginator)
            {
                // 如果是主账号需要加上已付的手续费
                for (auto & tmpTx : cblock.txs())
                {
                    CTxin txIn0 = tmpTx.vin(0);
                    if (txIn0.scriptsig().sign() == std::string(FEE_SIGN_STR))
                    {
                        for (auto & tmpTxOut : tmpTx.vout())
                        {
                            if (tmpTxOut.scriptpubkey() != addr)
                            {
                                amount += tmpTxOut.value();
                                pItem->set_amount(to_string(((double_t)amount) / DECIMAL_NUM));
                            }
                        }
                    }
                }
            }
        }
    }

    if (i == size)
    {
        i--; // 若一次性获得所有数据，索引需要减一
    }

    // 将已解除质押的交易设置为“质押但已解除”类型
    std::vector<std::string> redeemHash;
    for (auto & item : ack.list())
    {
        if (item.type() != TxInfoType_Redeem)
        {
            continue;
        }

        redeemHash.push_back(item.txhash());

    }

    for (auto & hash : redeemHash)
    {
        std::string serTxRaw;
        db_status = pRocksDb->GetTransactionByHash(txn, hash, serTxRaw);
        if (db_status != 0)
        {
            continue;
        }

        CTransaction tx;
        tx.ParseFromString(serTxRaw);
        if (tx.hash().size() == 0)
        {
            continue;
        }

        auto extra = nlohmann::json::parse(tx.extra());
        auto txinfo = extra["TransactionInfo"];
        std::string utxo = txinfo["RedeemptionUTXO"];


        for (size_t i = 0; i != (size_t)ack.list_size(); ++i)
        {
            TxInfoItem * item = ack.mutable_list(i);
            if (item->type() == TxInfoType_Pledge && 
                item->txhash() == utxo)
            {
                item->set_type(TxInfoType_PledgedAndRedeemed);
            }
        }
    }

    ack.set_total(total);
    ack.set_index(i);
    
    ack.set_code(0);
    ack.set_description("success");

}

// 处理手机端块列表请求
void HandleGetBlockInfoListReq(const std::shared_ptr<GetBlockInfoListReq>& msg, const MsgData& msgdata)
{
    // 回执消息体
    GetBlockInfoListAck getBlockInfoListAck;
    getBlockInfoListAck.set_version( getVersion() );

    // 版本判断
    if( 0 != IsVersionCompatible( msg->version() ) )
	{
        getBlockInfoListAck.set_code(-1);
        getBlockInfoListAck.set_description("version error! ");
        net_send_message<GetBlockInfoListAck>(msgdata, getBlockInfoListAck);
		error("(HandleGetBlockInfoListReq) IsVersionCompatible error!");
		return ;
	}

    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if( txn == NULL )
	{
		getBlockInfoListAck.set_code(-2);
		getBlockInfoListAck.set_description("TransactionInit failed !");
		net_send_message<GetBlockInfoListAck>(msgdata, getBlockInfoListAck);
		error("(HandleGetBlockInfoListReq) TransactionInit failed !");
		return ;
	}

	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, true);
	};

    unsigned int top = 0;
    if (0 != pRocksDb->GetBlockTop(txn, top) )
    {
        getBlockInfoListAck.set_code(-3);
		getBlockInfoListAck.set_description("GetBlockTop failed !");
		net_send_message<GetBlockInfoListAck>(msgdata, getBlockInfoListAck);
		error("(HandleGetBlockInfoListReq) GetBlockTop failed !");
		return ;
    }

    getBlockInfoListAck.set_top(top);

    uint64_t txcount = 0;
    if (0 != pRocksDb->GetTxCount(txn, txcount) )
    {
        getBlockInfoListAck.set_code(-4);
		getBlockInfoListAck.set_description("GetTxCount failed!");
		net_send_message<GetBlockInfoListAck>(msgdata, getBlockInfoListAck);
		error("(HandleGetBlockInfoListReq) GetTxCount failed!");
        return ;
    }

    getBlockInfoListAck.set_txcount(txcount);

    if (msg->count() < 0) 
    {
        getBlockInfoListAck.set_code(-5);
		getBlockInfoListAck.set_description("count cannot be less than 0!");
		net_send_message<GetBlockInfoListAck>(msgdata, getBlockInfoListAck);
		error("(HandleGetBlockInfoListReq) count cannot be less than 0!");
        return ;
    }

    uint64_t maxHeight = 0;
    uint64_t minHeight = 0;

    if (msg->index() == 0)
    {
        maxHeight = top;
        minHeight = maxHeight > msg->count() ? maxHeight - msg->count() : 0;
    }
    else
    {
        maxHeight = msg->index() > top ? top : msg->index();
        minHeight = msg->index() > msg->count() ? msg->index() - msg->count() : 0;
    }

    for (uint32_t count = maxHeight; count > minHeight; --count)
    {
        std::vector<std::string> blockHashs;
        if (0 != pRocksDb->GetBlockHashsByBlockHeight(txn, count, blockHashs) )
        {
            getBlockInfoListAck.set_code(-7);
            getBlockInfoListAck.set_description("GetBlockHashsByBlockHeight failed !");
            net_send_message<GetBlockInfoListAck>(msgdata, getBlockInfoListAck);
            error("(HandleGetBlockInfoListReq) GetBlockHashsByBlockHeight failed !");
            return ;
        }

        std::vector<CBlock> blocks;
        for (auto blkHash : blockHashs)
        {
            std::string blockStr;
            if (0 != pRocksDb->GetBlockByBlockHash(txn, blkHash, blockStr) )
            {
                getBlockInfoListAck.set_code(-8);
                getBlockInfoListAck.set_description("GetBlockHeaderByBlockHash failed !");
                net_send_message<GetBlockInfoListAck>(msgdata, getBlockInfoListAck);
                error("(HandleGetBlockInfoListReq) GetBlockHeaderByBlockHash failed !");
                return ;
            }

            CBlock cblock;
            cblock.ParseFromString(blockStr);
            if (cblock.hash().length() == 0)
            {
                continue;
            }
            blocks.push_back(cblock);
        }

        // 单层高度所有块按时间倒序
        std::sort(blocks.begin(), blocks.end(), [](CBlock & a, CBlock & b){
            return a.time() > b.time();
        });

        for (auto & cblock : blocks)
        {
            BlockInfoItem * pBlockInfoItem = getBlockInfoListAck.add_list();
            pBlockInfoItem->set_blockhash(cblock.hash());
            pBlockInfoItem->set_blockheight(cblock.height());
            pBlockInfoItem->set_time(cblock.time());

            CTransaction tx;
            for (auto & t : cblock.txs())
            {
                CTxin txin0 = t.vin(0);
                if (txin0.scriptsig().sign() != std::string(FEE_SIGN_STR) && 
                    txin0.scriptsig().sign() != std::string(EXTRA_AWARD_SIGN_STR))
                {
                    tx = t;
                    break;
                }
            }

            pBlockInfoItem->set_txhash(tx.hash());

            std::vector<std::string> owners = TxHelper::GetTxOwner(tx);
            for (auto txOwner : owners)
            {
                pBlockInfoItem->add_fromaddr(txOwner);
            }

            uint64_t amount = 0;
            nlohmann::json extra = nlohmann::json::parse(tx.extra());
            std::string txType = extra["TransactionType"].get<std::string>();
            if (txType == TXTYPE_REDEEM)
            {
                nlohmann::json txInfo = extra["TransactionInfo"].get<nlohmann::json>();
                std::string redeemUtxo = txInfo["RedeemptionUTXO"];

                std::string txRaw;
                if (0 != pRocksDb->GetTransactionByHash(txn, redeemUtxo, txRaw) )
                {
                    getBlockInfoListAck.set_code(-9);
                    getBlockInfoListAck.set_description("GetTransactionByHash failed !");
                    net_send_message<GetBlockInfoListAck>(msgdata, getBlockInfoListAck);
                    error("(HandleGetBlockInfoListReq) GetTransactionByHash failed !");
                }

                CTransaction utxoTx;
                std::string fromAddrTmp;
                utxoTx.ParseFromString(txRaw);

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

                pBlockInfoItem->add_toaddr(fromAddrTmp);
            }

            for (auto & txOut : tx.vout())
            {
                if (owners.end() != find(owners.begin(), owners.end(), txOut.scriptpubkey()))
                {
                    continue;
                }
                else
                {
                    pBlockInfoItem->add_toaddr(txOut.scriptpubkey());
                    amount += txOut.value();
                }
            }

            pBlockInfoItem->set_amount(to_string((double_t) amount / DECIMAL_NUM));
        }
    }

    getBlockInfoListAck.set_code(0);
    getBlockInfoListAck.set_description("successful");
    net_send_message<GetBlockInfoListAck>(msgdata, getBlockInfoListAck);
}

// 处理手机端块详情请求
void HandleGetBlockInfoDetailReq(const std::shared_ptr<GetBlockInfoDetailReq>& msg, const MsgData& msgdata)
{
    std::cout << "接受数据: " << std::endl;
    std::cout << "version: " << msg->version() <<std::endl;
    std::cout << "blockhash: " << msg->blockhash() << std::endl;
    
    GetBlockInfoDetailAck getBlockInfoDetailAck;
    getBlockInfoDetailAck.set_version( getVersion() );
    getBlockInfoDetailAck.set_code(0);

    // 版本判断
    if( 0 != IsVersionCompatible( msg->version() ) )
	{
        getBlockInfoDetailAck.set_code(-1);
        getBlockInfoDetailAck.set_description("version error! ");
        net_send_message<GetBlockInfoDetailAck>(msgdata, getBlockInfoDetailAck);
		error("(HandleGetBlockInfoDetailReq) IsVersionCompatible error!");
		return ;
	}

    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if( txn == NULL )
	{
		getBlockInfoDetailAck.set_code(-2);
		getBlockInfoDetailAck.set_description("TransactionInit failed !");
		net_send_message<GetBlockInfoDetailAck>(msgdata, getBlockInfoDetailAck);
		error("(HandleGetBlockInfoDetailReq) TransactionInit failed !");
		return ;
	}

	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, true);
	};

    std::string blockHeaderStr;
    if (0 != pRocksDb->GetBlockByBlockHash(txn, msg->blockhash(), blockHeaderStr) )
    {
        getBlockInfoDetailAck.set_code(-3);
		getBlockInfoDetailAck.set_description("GetBlockByBlockHash failed !");
		net_send_message<GetBlockInfoDetailAck>(msgdata, getBlockInfoDetailAck);
		error("(HandleGetBlockInfoDetailReq) GetBlockByBlockHash failed !");
		return ;
    }

    CBlock cblock;
    cblock.ParseFromString(blockHeaderStr);

    getBlockInfoDetailAck.set_blockhash(cblock.hash());
    getBlockInfoDetailAck.set_blockheight(cblock.height());
    getBlockInfoDetailAck.set_merkleroot(cblock.merkleroot());
    getBlockInfoDetailAck.set_prevhash(cblock.prevhash());
    getBlockInfoDetailAck.set_time(cblock.time());

    CTransaction tx;
    CTransaction gasTx;
    CTransaction awardTx;
    for (auto & t : cblock.txs())
    {
        if (t.vin(0).scriptsig().sign() == std::string(FEE_SIGN_STR))
        {
            gasTx = t;
        }
        else if (t.vin(0).scriptsig().sign() == std::string(EXTRA_AWARD_SIGN_STR))
        {
            awardTx = t;
        }
        else
        {
            tx = t;
        }
    }

    int64_t totalAmount = 0;
    
    // 获取交易的发起方账号
    std::vector<std::string> txOwners;
    txOwners = TxHelper::GetTxOwner(tx);
    
    // 判断是否是解除质押情况
    bool isRedeem = false;
    if (tx.vout_size() == 2)
    {
        std::vector<std::string> txOwnerVec;
        SplitString(tx.txowner(), txOwnerVec, "_");
        if (txOwnerVec.size() == 0)
        {
            getBlockInfoDetailAck.set_code(-4);
            getBlockInfoDetailAck.set_description("txowner empty !");
            net_send_message<GetBlockInfoDetailAck>(msgdata, getBlockInfoDetailAck);
            
            return ;
        }

        if (tx.vout(0).scriptpubkey() == tx.vout(1).scriptpubkey() && 
            txOwnerVec[0] == tx.vout(0).scriptpubkey())
        {
            isRedeem = true;
        }
    }

    if (isRedeem)
    {
        int64_t amount = tx.vout(0).value();

        nlohmann::json extra = nlohmann::json::parse(tx.extra());
        nlohmann::json txInfo = extra["TransactionInfo"].get<nlohmann::json>();
        std::string redeemUtxo = txInfo["RedeemptionUTXO"];

        std::string serTxRaw;
        pRocksDb->GetTransactionByHash(txn, redeemUtxo, serTxRaw);

        CTransaction pledgeTx;
        pledgeTx.ParseFromString(serTxRaw);
        if (pledgeTx.hash().length() == 0)
        {
            getBlockInfoDetailAck.set_code(-5);
            getBlockInfoDetailAck.set_description("get pledge transaction error");
            return;
        }

        for (auto & pledgeVout : pledgeTx.vout())
        {
            if (pledgeVout.scriptpubkey() == VIRTUAL_ACCOUNT_PLEDGE)
            {
                amount = pledgeVout.value();     
                break;
            }
        }
    
        int64_t gas = 0;
        for (auto & gasVout : gasTx.vout())
        {
            gas += gasVout.value();
        }
        
        if (gas == amount)
        {
            amount = 0;
        }
        else
        {
            amount -= gas;
        }

        BlockInfoOutAddr * pBlockInfoOutAddr = getBlockInfoDetailAck.add_blockinfooutaddr();
        pBlockInfoOutAddr->set_addr(tx.vout(0).scriptpubkey());
        pBlockInfoOutAddr->set_amount( std::to_string((double_t)amount / DECIMAL_NUM) );

        totalAmount  = amount;
    }
    else
    {
        // 计算交易总金额,不包括手续费
        for (int j = 0; j < tx.vout_size(); ++j)
        {
            CTxout txout = tx.vout(j);
            if (txOwners.end() == find (txOwners.begin(), txOwners.end(), txout.scriptpubkey() ) )
            {
                // 累计交易总值
                totalAmount += txout.value();

                // 分别记录每个账号的接收金额
                BlockInfoOutAddr * pBlockInfoOutAddr = getBlockInfoDetailAck.add_blockinfooutaddr();
                pBlockInfoOutAddr->set_addr(txout.scriptpubkey());

                int64_t value = txout.value();
                std::string amountStr = std::to_string( (double)value / DECIMAL_NUM );
                pBlockInfoOutAddr->set_amount( amountStr );
            }
        }
    }

    if (getBlockInfoDetailAck.blockinfooutaddr_size() > 5)
    {
        auto outAddr = getBlockInfoDetailAck.mutable_blockinfooutaddr();
        outAddr->erase(outAddr->begin() + 5, outAddr->end());
        getBlockInfoDetailAck.set_code(1);
    }

    for (int j = 0; j < gasTx.vout_size(); ++j)
    {
        CTxout txout = gasTx.vout(j);
        if (txOwners.end() == find (txOwners.begin(), txOwners.end(), txout.scriptpubkey() ) )
        {
            getBlockInfoDetailAck.add_signer(txout.scriptpubkey());
        }
    }

    std::string totalAmountStr = std::to_string( (double)totalAmount / DECIMAL_NUM );
    getBlockInfoDetailAck.set_tatalamount(totalAmountStr);

    net_send_message<GetBlockInfoDetailAck>(msgdata, getBlockInfoDetailAck);
}


void HandleGetTxInfoDetailReq(const std::shared_ptr<GetTxInfoDetailReq>& req, GetTxInfoDetailAck & ack)
{
    ack.set_version(getVersion());
    ack.set_code(0);
        
    std::string strTxHash = req->txhash();
    if (strTxHash.length() == 0)
    {
        ack.set_code(-1);
        ack.set_description("The TxHash is empty");
        return;
    }
    ack.set_txhash(strTxHash);

    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
    Transaction* txn = pRocksDb->TransactionInit();
    ON_SCOPE_EXIT 
    {
        pRocksDb->TransactionDelete(txn, true);
    };

    std::string strBlockHash;
    auto db_status = pRocksDb->GetBlockHashByTransactionHash(txn, strTxHash, strBlockHash);
    if (db_status != 0)
    {
        ack.set_code(-2);
        ack.set_description("The block hash is error");
        return;
    }

    std::string serBlockRaw;
    db_status = pRocksDb->GetBlockByBlockHash(txn, strBlockHash, serBlockRaw);
    if (db_status != 0)
    {
        ack.set_code(-3);
        ack.set_description("The block error");
        return;
    }

    CBlock cblock;
    cblock.ParseFromString(serBlockRaw);

    ack.set_blockhash(cblock.hash());
    ack.set_blockheight(cblock.height());

    CTransaction tx;
    CTransaction gasTx;
    CTransaction awardTx;
    for (auto & t : cblock.txs())
    {
        if (t.vin(0).scriptsig().sign() == std::string(FEE_SIGN_STR))
        {
            gasTx = t;
        }
        else if (t.vin(0).scriptsig().sign() == std::string(EXTRA_AWARD_SIGN_STR))
        {
            awardTx = t;
        }
        else
        {
            tx = t;
        }
    }

    if (tx.hash().length() == 0 || 
        gasTx.hash().length() == 0 ||
        awardTx.hash().length() == 0)
    {
        ack.set_code(-4);
        ack.set_description("get transaction error");
        return;
    }
    

    ack.set_time(tx.time());
    std::vector<string> owners = TxHelper::GetTxOwner(tx);
    for (auto & owner : owners)
    {
        ack.add_fromaddr(owner);
    }

    // 超过更多数据需要另行显示
    if (ack.fromaddr_size() > 5)
    {
        auto fromAddr = ack.mutable_fromaddr();
        fromAddr->erase(fromAddr->begin() + 5, fromAddr->end());
        ack.set_code(2);
    }

    int64_t amount = 0;

    // 判断是否是解除质押情况
    bool isRedeem = false;
    if (tx.vout_size() == 2)
    {
        std::vector<std::string> txOwnerVec;
        SplitString(tx.txowner(), txOwnerVec, "_");

        if (txOwnerVec.size() == 0)
        {
            ack.set_code(-4);
            ack.set_description("txowner is empty");
            return;
        }

        if (tx.vout(0).scriptpubkey() == tx.vout(1).scriptpubkey() && 
            txOwnerVec[0] == tx.vout(0).scriptpubkey())
        {
            isRedeem = true;
        }
    }

    if (isRedeem)
    {
        amount = tx.vout(0).value();
        
        nlohmann::json extra = nlohmann::json::parse(tx.extra());
        nlohmann::json txInfo = extra["TransactionInfo"].get<nlohmann::json>();

        std::string redeemUtxo = txInfo["RedeemptionUTXO"];

        std::string serTxRaw;
        pRocksDb->GetTransactionByHash(txn, redeemUtxo, serTxRaw);

        CTransaction pledgeTx;
        pledgeTx.ParseFromString(serTxRaw);
        if (pledgeTx.hash().length() == 0)
        {
            ack.set_code(-5);
            ack.set_description("get pledge transaction error");
            return;
        }

        for (auto & pledgeVout : pledgeTx.vout())
        {
            if (pledgeVout.scriptpubkey() == VIRTUAL_ACCOUNT_PLEDGE)
            {
                amount = pledgeVout.value();     
                break;
            }
        }
    
        ToAddr * toaddr = ack.add_toaddr();
        toaddr->set_toaddr(tx.vout(0).scriptpubkey());

        int64_t gas = 0;
        for (auto & gasVout : gasTx.vout())
        {
            gas += gasVout.value();
        }
        
        if (gas == amount)
        {
            amount = 0;
        }

        toaddr->set_amt(to_string( (double_t)amount / DECIMAL_NUM) );
    }
    else
    {
        std::map<std::string, int64_t> toAddrMap;
        for (auto owner : owners)
        {
            for (auto & txout : tx.vout())
            {
                std::string targetAddr = txout.scriptpubkey();
                if (owner != txout.scriptpubkey())
                {
                    toAddrMap.insert(std::make_pair(targetAddr, txout.value()));
                }
            }
        }

        // 排除vin中已有的
        for (auto iter = toAddrMap.begin(); iter != toAddrMap.end(); ++iter)
        {
            for (auto owner : owners)
            {
                if (owner ==  iter->first)
                {
                    iter = toAddrMap.erase(iter);
                }
            }
        }

        for (auto & item : toAddrMap)
        {
            amount += item.second;
            ToAddr * toaddr = ack.add_toaddr();
            toaddr->set_toaddr(item.first);
            toaddr->set_amt(to_string( (double_t)item.second / DECIMAL_NUM));
        }
        
        // 超过更多数据需要另行显示
        if (ack.toaddr_size() > 5)
        {
            auto toaddr = ack.mutable_toaddr();
            toaddr->erase(toaddr->begin() + 5, toaddr->end());
            ack.set_code(1);
        }
    }
 
    ack.set_amount(to_string((double_t)amount / DECIMAL_NUM));

    int64_t totalAward = 0; // 总奖励
    int64_t awardAmount = 0; // 个人奖励
    for (auto & txout : awardTx.vout())
    {
        std::string targetAddr = txout.scriptpubkey();
        if (targetAddr == req->addr())
        {
            awardAmount = txout.value();
        }
        totalAward += txout.value();
    }
    ack.set_awardamount(to_string( (double_t)awardAmount / DECIMAL_NUM) );
    ack.set_award(to_string( (double_t) totalAward / DECIMAL_NUM) );

    int64_t gas = 0;
    int64_t awardGas = 0;
    for (auto & txout : gasTx.vout())
    {
        gas += txout.value();
        std::string targetAddr = txout.scriptpubkey();
        if (targetAddr == req->addr())
        {
            awardGas = txout.value();
        }
    }
    
    ack.set_gas(to_string((double_t)gas / DECIMAL_NUM));
    ack.set_awardgas(to_string((double_t)awardGas / DECIMAL_NUM));

    ack.set_description("success");
}

// string version = 1;
// sint32 code = 2;
// string description = 3;
// 手机连接矿机发起质押交易
void HandleCreateDevicePledgeTxMsgReq(const std::shared_ptr<CreateDevicePledgeTxMsgReq>& msg, const MsgData &msgdata )
{
    CreatePledgeTransaction(msg->addr(), msg->amt(), std::stoi(msg->needverifyprehashcount()), msg->gasfees(), msg->password(), msgdata );
    return ;
}

// 手机连接矿机发起解质押交易
void HandleCreateDeviceRedeemTxReq(const std::shared_ptr<CreateDeviceRedeemTxReq> &msg, const MsgData &msgdata )
{
    CreateRedeemTransaction(msg->addr(), std::stoi(msg->needverifyprehashcount()), msg->gasfees(), msg->utxo(), msg->password(), msgdata );
    return ;
}


void assign(const std::shared_ptr<Message> &msg, const MsgData &msgdata, const std::string msg_name) 
{
    cout << "msg_name " << msg_name << endl;

    if (msg_name == "GetNodeServiceFeeReq") 
    {
        const std::shared_ptr<GetNodeServiceFeeReq>ack_msg = dynamic_pointer_cast<GetNodeServiceFeeReq>(msg);

        GetNodeServiceFeeAck fee_ack;
        if (!is_version) 
        {
            GetNodeServiceFee(ack_msg, fee_ack);
        } 
        else 
        {
            fee_ack.set_version(getVersion());
            fee_ack.set_code(is_version);
            fee_ack.set_description("版本错误");
        }

        net_send_message<GetNodeServiceFeeAck>(msgdata, fee_ack);
    } 
    else if (msg_name == "SetServiceFeeReq") 
    {
        const std::shared_ptr<SetServiceFeeReq>ack_msg = dynamic_pointer_cast<SetServiceFeeReq>(msg);

        SetServiceFeeAck amount_ack;
        if (!is_version) 
        {
            SetServiceFee(ack_msg, amount_ack);
        } 
        else 
        {
            amount_ack.set_version(getVersion());
            amount_ack.set_code(is_version);
            amount_ack.set_description("版本错误");
        }

        net_send_message<SetServiceFeeAck>(msgdata, amount_ack);
    } 
    else if (msg_name == "GetServiceInfoReq") 
    {
        const std::shared_ptr<GetServiceInfoReq>ack_msg = dynamic_pointer_cast<GetServiceInfoReq>(msg);

       HandleGetServiceInfoReq(ack_msg, msgdata);
    } 
    else if (msg_name == "GetBlockTopReq") 
    {
        const std::shared_ptr<GetBlockTopReq>ack_msg = dynamic_pointer_cast<GetBlockTopReq>(msg);

       // HandleGetBlockTopReq(ack_msg, msgdata);
    } 
    else if (msg_name == "TApiGetBlockTopAck") 
    {
        const std::shared_ptr<TApiGetBlockTopAck>ack_msg = dynamic_pointer_cast<TApiGetBlockTopAck>(msg);

        //TApiGetBlockTopAckFunc(ack_msg, msgdata);
    } 
    else if (msg_name == "GetPacketFeeReq") 
    {
        const std::shared_ptr<GetPacketFeeReq>ack_msg = dynamic_pointer_cast<GetPacketFeeReq>(msg);

        GetPacketFeeAck amount_ack;
        if (!is_version) 
        {
            GetPacketFee(ack_msg, amount_ack);
        } 
        else 
        {
            amount_ack.set_version(getVersion());
            amount_ack.set_code(is_version);
            amount_ack.set_description("版本错误");
        }

        net_send_message<GetPacketFeeAck>(msgdata, amount_ack);
    } 
    else if (msg_name == "GetAmountReq") 
    {
        const std::shared_ptr<GetAmountReq>ack_msg = dynamic_pointer_cast<GetAmountReq>(msg);

        GetAmountAck amount_ack;
        if (!is_version) 
        {
            GetAmount(ack_msg, amount_ack);
        } 
        else 
        {
            amount_ack.set_version(getVersion());
            amount_ack.set_code(is_version);
            amount_ack.set_description("版本错误");
        }

        net_send_message<GetAmountAck>(msgdata, amount_ack);
    } 
    else if (msg_name == "GetBlockInfoReq") 
    {
        const std::shared_ptr<GetBlockInfoReq>ack_msg = dynamic_pointer_cast<GetBlockInfoReq>(msg);

        cout << "GetBlockInfoReq: height " << ack_msg->height() << endl;
        cout << "GetBlockInfoReq: count " << ack_msg->count() << endl;

        GetBlockInfoAck block_info_ack;
        if (!is_version) 
        {
            BlockInfoReq(ack_msg, block_info_ack);
        } 
        else 
        {
            block_info_ack.set_version(getVersion());
            block_info_ack.set_code(is_version);
            block_info_ack.set_description("版本错误");
        }

        net_send_message<GetBlockInfoAck>(msgdata, block_info_ack);
    } 
    else if (msg_name == "GetDevPasswordReq") 
    {
        const std::shared_ptr<GetDevPasswordReq>ack_msg = dynamic_pointer_cast<GetDevPasswordReq>(msg);
        cout << "GetDevPasswordReq: pass " << ack_msg->password() << endl;

        GetDevPasswordAck pass_ack;
        if (!is_version) 
        {
            GainDevPasswordReq(ack_msg, pass_ack);
        } 
        else 
        {
            pass_ack.set_version(getVersion());
            pass_ack.set_code(is_version);
            pass_ack.set_description("版本错误");
        }

        net_send_message<GetDevPasswordAck>(msgdata, pass_ack);
    } 
    else if (msg_name == "SetDevPasswordReq") 
    {
        const std::shared_ptr<SetDevPasswordReq>ack_msg = dynamic_pointer_cast<SetDevPasswordReq>(msg);
        cout << "SetDevPasswordReq: pass " << ack_msg->old_pass() << endl;
        cout << "SetDevPasswordReq: pass " << ack_msg->new_pass() << endl;

        SetDevPasswordAck pass_ack;
        if (!is_version) 
        {
            DevPasswordReq(ack_msg, pass_ack);
        } 
        else 
        {
            pass_ack.set_version(getVersion());
            pass_ack.set_code(is_version);
            pass_ack.set_description("版本错误");
        }

        net_send_message<SetDevPasswordAck>(msgdata, pass_ack);
    } 
    else if (msg_name == "GetAddrInfoReq") 
    {
        const std::shared_ptr<GetAddrInfoReq>ack_msg = dynamic_pointer_cast<GetAddrInfoReq>(msg);

        GetAddrInfoAck addr_ack;
        if (!is_version) 
        {
            GetTransactionInfo(ack_msg, addr_ack);
        } 
        else 
        {
            addr_ack.set_version(getVersion());
            addr_ack.set_code(is_version);
            addr_ack.set_description("版本错误");
        }

        net_send_message<GetAddrInfoAck>(msgdata, addr_ack);
    } 
    else if (msg_name == "GetNodeInfoReq") 
    {
        const std::shared_ptr<GetNodeInfoReq>ack_msg = dynamic_pointer_cast<GetNodeInfoReq>(msg);

        GetNodeInfoAck node_ack;
        if (!is_version) 
        {
            GetNodInfo(node_ack);
        } 
        else
        {
            node_ack.set_version(getVersion());
            node_ack.set_code(is_version);
            node_ack.set_description("版本错误");
        }

        net_send_message<GetNodeInfoAck>(msgdata, node_ack);
    } 
    else if (msg_name == "GetClientInfoReq") 
    {
        const std::shared_ptr<GetClientInfoReq>ack_msg = dynamic_pointer_cast<GetClientInfoReq>(msg);

        GetClientInfoAck info_ack;
        if (!is_version) 
        {
            GetClientInfo(ack_msg, info_ack);
        } 
        else 
        {
            info_ack.set_version(getVersion());
            info_ack.set_code(is_version);
            info_ack.set_description("版本错误");
        }

        net_send_message<GetClientInfoAck>(msgdata, info_ack);
    }
    else if(msg_name == "GetDevPrivateKeyReq")
    {
        const std::shared_ptr<GetDevPrivateKeyReq>ack_msg = dynamic_pointer_cast<GetDevPrivateKeyReq>(msg);
        GetDevPrivateKeyAck devprikey_ack ;
        if (!is_version) 
        {
            HandleGetDevPrivateKeyReq(ack_msg, devprikey_ack);
        } 
        else 
        {
            devprikey_ack.set_version(getVersion());
            devprikey_ack.set_code(is_version);
            devprikey_ack.set_description("版本错误");
        }
        net_send_message<GetDevPrivateKeyAck>(msgdata, devprikey_ack);
    }
    else if(msg_name == "CreatePledgeTxMsgReq")
    {
        const std::shared_ptr<CreatePledgeTxMsgReq>ack_msg = dynamic_pointer_cast<CreatePledgeTxMsgReq>(msg);
        HandleCreatePledgeTxMsgReq(ack_msg,  msgdata);
    }
    else if(msg_name == "PledgeTxMsgReq")
    {
        const std::shared_ptr<PledgeTxMsgReq>ack_msg = dynamic_pointer_cast<PledgeTxMsgReq>(msg);
        HandlePledgeTxMsgReq(ack_msg, msgdata);
    }
    else if(msg_name == "CreateRedeemTxMsgReq")
    {
        const std::shared_ptr<CreateRedeemTxMsgReq>ack_msg = dynamic_pointer_cast<CreateRedeemTxMsgReq>(msg);
        HandleCreateRedeemTxMsgReq(ack_msg, msgdata);
    }
    else if(msg_name == "RedeemTxMsgReq")
    {
        const std::shared_ptr<RedeemTxMsgReq>ack_msg = dynamic_pointer_cast<RedeemTxMsgReq>(msg);
        HandleRedeemTxMsgReq(ack_msg, msgdata);
    }
    else if (msg_name == "CreateDevicePledgeTxMsgReq")
    {
        const std::shared_ptr<CreateDevicePledgeTxMsgReq>ack_msg = dynamic_pointer_cast<CreateDevicePledgeTxMsgReq>(msg);
        HandleCreateDevicePledgeTxMsgReq(ack_msg, msgdata);
    }
    else if (msg_name == "CreateDeviceRedeemTxReq")
    {
        const std::shared_ptr<CreateDeviceRedeemTxReq>ack_msg = dynamic_pointer_cast<CreateDeviceRedeemTxReq>(msg);
        HandleCreateDeviceRedeemTxReq(ack_msg, msgdata);
    }
    else if (msg_name == "GetPledgeListReq")
    {
        const std::shared_ptr<GetPledgeListReq> req = dynamic_pointer_cast<GetPledgeListReq>(msg);
        GetPledgeListAck ack;
        if (!is_version)
        {
            HandleGetPledgeListReq(req, ack);
        }
        else
        {
            ack.set_version(getVersion());
            ack.set_code(is_version);
            ack.set_description("版本错误");
        }

        net_send_message<GetPledgeListAck>(msgdata, ack);
    }
    else if (msg_name == "GetTxInfoListReq")
    {
        const std::shared_ptr<GetTxInfoListReq> req = dynamic_pointer_cast<GetTxInfoListReq>(msg);
        GetTxInfoListAck ack;
        if (!is_version)
        {
            HandleGetTxInfoListReq(req, ack);
        }
        else
        {
            ack.set_version(getVersion());
            ack.set_code(is_version);
            ack.set_description("版本错误");
        }

        net_send_message<GetTxInfoListAck>(msgdata, ack);
        
    }
    else if (msg_name == "GetBlockInfoListReq")
    {
        const std::shared_ptr<GetBlockInfoListReq> req = dynamic_pointer_cast<GetBlockInfoListReq>(msg);

        HandleGetBlockInfoListReq(req, msgdata);
    }
    else if (msg_name == "GetBlockInfoDetailReq")
    {
        const std::shared_ptr<GetBlockInfoDetailReq> req = dynamic_pointer_cast<GetBlockInfoDetailReq>(msg);

        HandleGetBlockInfoDetailReq(req, msgdata);
    }
    else if (msg_name == "GetTxInfoDetailReq")
    {
        const std::shared_ptr<GetTxInfoDetailReq> req = dynamic_pointer_cast<GetTxInfoDetailReq>(msg);
        GetTxInfoDetailAck ack;
        if (!is_version)
        {
            HandleGetTxInfoDetailReq(req, ack);
        }
        else
        {
            ack.set_version(getVersion());
            ack.set_code(is_version);
            ack.set_description("版本错误");
        }
        
        net_send_message<GetTxInfoDetailAck>(msgdata, ack);
    }
    else if (msg_name == "CreateDeviceMultiTxMsgReq")
    {
        const std::shared_ptr<CreateDeviceMultiTxMsgReq> req = dynamic_pointer_cast<CreateDeviceMultiTxMsgReq>(msg);

        HandleCreateDeviceMultiTxMsgReq(req, msgdata);
    }
    else if (msg_name == "CreateMultiTxMsgReq")
    {
        const std::shared_ptr<CreateMultiTxMsgReq> req = dynamic_pointer_cast<CreateMultiTxMsgReq>(msg);

        HandleCreateMultiTxReq(req, msgdata);
    }
    else if (msg_name == "MultiTxMsgReq")
    {
        const std::shared_ptr<MultiTxMsgReq> req = dynamic_pointer_cast<MultiTxMsgReq>(msg);

        HandlePreMultiTxRaw(req, msgdata);
    }
    else if (msg_name == "TestConnectReq")
    {
        TestConnectAck ack;
        ack.set_version(getVersion());
        ack.set_code(0);
        net_send_message<TestConnectAck>(msgdata, ack);
    }
    //======================================
    else if (msg_name == "Device2PubNetRandomReq")
    {
        const std::shared_ptr<Device2PubNetRandomReq> req = dynamic_pointer_cast<Device2PubNetRandomReq>(msg);
       
        HandleUpdateProgramEcho(req);
    }
    else if (msg_name == "RandomPubNet2DeviceAck")
    {
        const std::shared_ptr<RandomPubNet2DeviceAck> ack = dynamic_pointer_cast<RandomPubNet2DeviceAck>(msg);
       
        HandleUpdateProgramTransmitPublicOrNormal(ack, msgdata);
    }
    else if (msg_name == "Device2AllDevReq")
    {
        const std::shared_ptr<Device2AllDevReq> req = dynamic_pointer_cast<Device2AllDevReq>(msg);
       
        HandleUpdateProgramNodeBroadcast( req, msgdata);
    }
    else if (msg_name == "Feedback2DeviceAck")
    {
        const std::shared_ptr<Feedback2DeviceAck> ack = dynamic_pointer_cast<Feedback2DeviceAck>(msg);
       
        HandleUpdateProgramNodeId(ack);
    }
    else if (msg_name == "DataTransReq")
    {
        const std::shared_ptr<DataTransReq> req = dynamic_pointer_cast<DataTransReq>(msg);
        
        HandleUpdateProgramdata( req, msgdata);
       
    }
    else if (msg_name == "TransData")
    {
        const std::shared_ptr<TransData> ack = dynamic_pointer_cast<TransData>(msg);
       
        HandleUpdateProgramRecoveryFile(ack);
    }
}

//version 4-3.0.14
//效验
void verify(const std::shared_ptr<Message> &msg, const MsgData &msgdata) 
{
    const google::protobuf::Descriptor *descriptor = msg->GetDescriptor();
    const google::protobuf::Reflection *reflection = msg->GetReflection();

    cout << "descriptor->field_count()" << descriptor->name() << endl;

    const google::protobuf::FieldDescriptor* field = descriptor->field(0);

    std::string version = reflection->GetString(*msg, field);
    cout << "version=" << version << endl;

    is_version = IsVersionCompatible(version);
    cout << "is version=" << is_version << endl;

    assign(msg, msgdata, descriptor->name());
}

}//namespace end


void MuiltipleApi() 
{

    // net_register_callback<Device2PubNetRandomReq>([](const std::shared_ptr<Device2PubNetRandomReq>& msg, 
    // const MsgData& msgdata) 
    // {
    //     m_api::verify(msg, msgdata);
    // });

    // net_register_callback<RandomPubNet2DeviceAck>([](const std::shared_ptr<RandomPubNet2DeviceAck>& msg, 
    // const MsgData& msgdata) 
    // {
    //     m_api::verify(msg, msgdata);
    // });

    // net_register_callback<Device2AllDevReq>([](const std::shared_ptr<Device2AllDevReq>& msg, 
    // const MsgData& msgdata) 
    // {
    //     m_api::verify(msg, msgdata);
    // });
    // net_register_callback<Feedback2DeviceAck>([](const std::shared_ptr<Feedback2DeviceAck>& msg, 
    // const MsgData& msgdata) 
    // {
    //     m_api::verify(msg, msgdata);
    // });
    // net_register_callback<DataTransReq>([](const std::shared_ptr<DataTransReq>& msg, 
    // const MsgData& msgdata) 
    // {
    //     m_api::verify(msg, msgdata);
    // });
    // net_register_callback<TransData>([](const std::shared_ptr<TransData>& msg, 
    // const MsgData& msgdata) 
    // {
    //     m_api::verify(msg, msgdata);
    // });
    /** new s **/
    //获取全网节点矿费 std::map<std::string, uint64_t> net_get_node_ids_and_fees()
    net_register_callback<GetNodeServiceFeeReq>([](const std::shared_ptr<GetNodeServiceFeeReq> &msg, 
    const MsgData &msgdata) 
    {
        m_api::verify(msg, msgdata);
    });

    net_register_callback<SetServiceFeeReq>([](const std::shared_ptr<SetServiceFeeReq>& msg, 
    const MsgData& msgdata) 
    {
        m_api::verify(msg, msgdata);
    });

    //获取矿机信息(矿机端 需要请求服务节点接口 异步返回给矿机端 矿机端返回给手机端)
    net_register_callback<GetServiceInfoReq>([](const std::shared_ptr<GetServiceInfoReq>& msg, 
    const MsgData& msgdata) 
    {
        m_api::verify(msg, msgdata);
    });

    //获取服务节点块高度信息
    net_register_callback<GetBlockTopReq>([](const std::shared_ptr<GetBlockTopReq>& msg, 
    const MsgData& msgdata) 
    {
        m_api::verify(msg, msgdata);
    });

    net_register_callback<TApiGetBlockTopAck>([](const std::shared_ptr<TApiGetBlockTopAck>& msg, 
    const MsgData& msgdata) 
    {
        m_api::verify(msg, msgdata);
    });

    net_register_callback<GetPacketFeeReq>([](const std::shared_ptr<GetPacketFeeReq>& msg, 
    const MsgData& msgdata) 
    {
        m_api::verify(msg, msgdata);
    });
    /** new e **/
    net_register_callback<GetAmountReq>([](const std::shared_ptr<GetAmountReq>& msg, 
    const MsgData& msgdata) 
    {
        m_api::verify(msg, msgdata);
    });

    net_register_callback<GetBlockInfoReq>([](const std::shared_ptr<GetBlockInfoReq>& msg, 
    const MsgData& msgdata) 
    {
        m_api::verify(msg, msgdata);
    });

    net_register_callback<GetDevPasswordReq>([](const std::shared_ptr<GetDevPasswordReq>& msg, 
    const MsgData& msgdata) 
    {
        m_api::verify(msg, msgdata);
    });

    net_register_callback<SetDevPasswordReq>([](const std::shared_ptr<SetDevPasswordReq>& msg, 
    const MsgData& msgdata) 
    {
        m_api::verify(msg, msgdata);
    });

    net_register_callback<GetAddrInfoReq>([](const std::shared_ptr<GetAddrInfoReq>& msg, 
    const MsgData& msgdata) 
    {
        m_api::verify(msg, msgdata);
    });

    net_register_callback<GetNodeInfoReq>([](const std::shared_ptr<GetNodeInfoReq>& msg, 
    const MsgData& msgdata) 
    {
        m_api::verify(msg, msgdata);
    });

    net_register_callback<GetClientInfoReq>([](const std::shared_ptr<GetClientInfoReq>& msg, 
    const MsgData& msgdata) 
    {
        m_api::verify(msg, msgdata);
    });

    net_register_callback<GetDevPrivateKeyReq>([](const std::shared_ptr<GetDevPrivateKeyReq>& msg, 
    const MsgData& msgdata) 
    {
        m_api::verify(msg, msgdata);
    });

    net_register_callback<CreatePledgeTxMsgReq>([](const std::shared_ptr<CreatePledgeTxMsgReq>& msg, 
    const MsgData& msgdata) 
    {
        m_api::verify(msg, msgdata);
    });

    net_register_callback<PledgeTxMsgReq>([](const std::shared_ptr<PledgeTxMsgReq>& msg, 
    const MsgData& msgdata) 
    {
        m_api::verify(msg, msgdata);
    });

     net_register_callback<CreateRedeemTxMsgReq>([](const std::shared_ptr<CreateRedeemTxMsgReq>& msg, 
    const MsgData& msgdata) 
    {
        m_api::verify(msg, msgdata);
    });

     net_register_callback<RedeemTxMsgReq>([](const std::shared_ptr<RedeemTxMsgReq>& msg, 
    const MsgData& msgdata) 
    {
        m_api::verify(msg, msgdata);
    });

    net_register_callback<CreateDevicePledgeTxMsgReq>([](const std::shared_ptr<CreateDevicePledgeTxMsgReq>& msg, 
    const MsgData& msgdata) 
    {
        m_api::verify(msg, msgdata);
    });

    net_register_callback<GetPledgeListReq>([](const std::shared_ptr<GetPledgeListReq>& msg, 
    const MsgData& msgdata) 
    {

        m_api::verify(msg, msgdata);
    });

    net_register_callback<GetTxInfoListReq>([](const std::shared_ptr<GetTxInfoListReq>& msg, 
    const MsgData& msgdata) 
    {

        m_api::verify(msg, msgdata);
    });

    net_register_callback<GetBlockInfoListReq>([](const std::shared_ptr<GetBlockInfoListReq>& msg, 
    const MsgData& msgdata) 
    {

        m_api::verify(msg, msgdata);
    });

    net_register_callback<GetBlockInfoDetailReq>([](const std::shared_ptr<GetBlockInfoDetailReq>& msg, 
    const MsgData& msgdata) 
    {

        m_api::verify(msg, msgdata);
    });
    
    net_register_callback<GetTxInfoDetailReq>([](const std::shared_ptr<GetTxInfoDetailReq>& msg, 
    const MsgData& msgdata) 
    {

        m_api::verify(msg, msgdata);
    });

    net_register_callback<CreateDeviceMultiTxMsgReq>([](const std::shared_ptr<CreateDeviceMultiTxMsgReq>& msg, 
    const MsgData& msgdata) 
    {

        m_api::verify(msg, msgdata);
    });
    
    net_register_callback<CreateDeviceRedeemTxReq>([](const std::shared_ptr<CreateDeviceRedeemTxReq>& msg, 
    const MsgData& msgdata) 
    {
        m_api::verify(msg, msgdata);
    });

    net_register_callback<CreateMultiTxMsgReq>([](const std::shared_ptr<CreateMultiTxMsgReq>& msg, 
    const MsgData& msgdata) 
    {
        m_api::verify(msg, msgdata);
    });

    net_register_callback<MultiTxMsgReq>([](const std::shared_ptr<MultiTxMsgReq>& msg, 
    const MsgData& msgdata) 
    {
        m_api::verify(msg, msgdata);
    });

    net_register_callback<TestConnectReq>([](const std::shared_ptr<TestConnectReq>& msg, 
    const MsgData& msgdata) 
    {
        m_api::verify(msg, msgdata);
    });
}


int  get_exec_name(char *name,int name_size)
{
	char path[1024]={0};
	int ret = readlink("/proc/self/exe",path,sizeof(path)-1);
	if(ret == -1)
	{
		printf("---- get exec name fail!!\n");
		return -1;
	}
	path[ret]= '\0';
	char *ptr = strrchr(path,'/');
	memset(name,0,name_size); //清空缓存区
	strncpy(name,ptr+1,name_size-1);
	return 0;
}



void HandleUpdateProgramRequst()
{
    cout<<"start"<<endl;
    cout<<"self version ="<< getVersion()<<endl;

    if (access("/opt/ebpc/bin/sn", F_OK) ) 
    {
        cout<<" /opt/ebpc/bin/sn is not exist"<<endl;
        return;
    }

    if (Singleton<Config>::get_instance()->GetIsPublicNode())
    {
        std::string localIp = Singleton<Config>::get_instance()->GetLocalIP();
        if (isPublicIp(localIp))
        {
            cout<<"localIp ="<<localIp <<endl;
            return;
        }
        else
        {
            cout << "Self node is not public ip = " << localIp << endl;
        }
    }

    Device2PubNetRandomReq device2PubNetRandomReq;
    string self_id = Singleton<Config>::get_instance()->GetKID().c_str();
    device2PubNetRandomReq.set_version(getVersion());
    device2PubNetRandomReq.set_id(self_id);

    std::vector<Node> nodes = net_get_public_node();
    std::vector<Node> publicNodes;

    // Verify publicNodes pledge and fees
    for (auto &node : nodes)
    {
        if (node.public_ip == node.local_ip && isPublicIp(IpPort::ipsz(node.public_ip)))
        {
            uint64_t amount = 0;
            if (SearchPledge(node.base58address, amount) != 0)
            {
                cout << "查询质押金额失败" << endl;
                continue ;
            }

            if (amount < g_TxNeedPledgeAmt)
            {
                // 质押金额不足
                cout << "Less amount of pledge =" << IpPort::ipsz(node.public_ip) << endl;
                continue ;
            }

            if (amount >= g_TxNeedPledgeAmt && node.fee > 0)
            {
                publicNodes.push_back(node);
                //std::sort(publicNodes.begin(), publicNodes.end());
                publicNodes.erase(unique(publicNodes.begin(), publicNodes.end()), publicNodes.end());
                cout << " public node = " << IpPort::ipsz(node.public_ip) << endl;
            }
        }
        else
        {
            cout << "Not public ip = " << IpPort::ipsz(node.local_ip) << endl;
        }
    }

    if(publicNodes.size() > 0)
    {
        srand(time(NULL));
        cout<<"random public nodelist.size() = "<< publicNodes.size() <<endl;
        int i = rand() % publicNodes.size();
        cout<<"random publicNodes i ="<<i<<endl;
        cout<< "random publicNodes id = "<<publicNodes[i].id<<endl;
        cout<<"in the function internal"<<endl;
        g_random_public_node.push_back(publicNodes[i].id);
        net_send_message<Device2PubNetRandomReq>(g_random_public_node[0], device2PubNetRandomReq);
    }
    cout<<"end"<<endl;
    return;
}


void  HandleUpdateProgramEcho(const std::shared_ptr<Device2PubNetRandomReq>& req)
{
    RandomPubNet2DeviceAck randomPubNet2DeviceAck;
    cout<<"PubNet2DeviceAck"<<endl;
    randomPubNet2DeviceAck.set_version(getVersion());
    randomPubNet2DeviceAck.set_description("传输版本给请求节点");
    string id = req->id();
    cout<<"PubNet2DeviceAck id = "<<  id <<endl;
    net_send_message<RandomPubNet2DeviceAck>(id, randomPubNet2DeviceAck);
    cout<<"公网回复请求节点完毕"<<endl;
    return;
}


void  HandleUpdateProgramTransmitPublicOrNormal(const std::shared_ptr<RandomPubNet2DeviceAck>& ack, const MsgData& msgdata)
{
    cout<<"HandlePushUpdateInfo"<<endl;
    string version = ack->version();
    if(version.empty()) 
    { 
        cout<<"echo version is empty!"<<endl;
        return; 
    }
    cout<<"echo version ="<<version <<endl;

    std::vector<std::string> SelfVersionNum;
	SplitString(getVersion(), SelfVersionNum, "_");
    if(SelfVersionNum.size() == 0) 
    {
        cout<<"SelfVersionNum size is zero"<<endl;
        return;
    }
    std::vector<std::string> SplitVersionNum;
    SplitString(SelfVersionNum[1], SplitVersionNum, ".");
    cout<<"Split Self Version Num size = "<<SplitVersionNum.size()<<endl;
    for(auto &self :SplitVersionNum)
    {
        cout<<"SplitSelfVersionNum="<< self<<endl;
    }

    std::vector<std::string> RecvVersion;
    SplitString(version, RecvVersion, "_");
    std::vector<std::string> SplitRecvVersionNum;
    if(RecvVersion.size() == 0) 
    {
        cout<<"RecvVersion size is zero"<<endl;
        return;
    } 
    SplitString(RecvVersion[1], SplitRecvVersionNum, ".");
    cout<<"Split echo Version Num size = "<<SplitRecvVersionNum.size()<<endl;
    for(auto &echo :SplitRecvVersionNum)
    {
        cout<<"SplitechoVersionNum="<< echo<<endl;
    }

    if(SplitVersionNum.size() >2 ||SplitRecvVersionNum.size() >2)
    {
        return;
    }

    if (atoi(SplitVersionNum[0].c_str()) == atoi(SplitRecvVersionNum[0].c_str()) && atoi(SplitVersionNum[1].c_str()) == atoi(SplitRecvVersionNum[1].c_str()))
    {
        cout<<"自身版本和哈希与公网相同不用更换版本"<<endl;
        return;
    }
    
    if (atoi(SplitVersionNum[0].c_str()) < atoi(SplitRecvVersionNum[0].c_str()))
    {
        cout<<"Self version less than echo version to broadcast"<<endl;
        string self_id = Singleton<Config>::get_instance()->GetKID().c_str();
        Device2AllDevReq  device2AllDevReq;
        device2AllDevReq.set_version(version);
        device2AllDevReq.set_id(self_id);
        
        // 全网广播3次
        std::vector<std::string> sendid = RandomNodeId(2);
        cout <<"sendid size()=" <<sendid.size() << endl;
        for(unsigned int i =0;i<sendid.size();i++)
        {
            cout<<"send id ="<<sendid[i]<<endl;
            net_send_message<Device2AllDevReq>(sendid[i], device2AllDevReq);
        }  
        cout<<"Self version less than echo version the broadcasted end! "<<endl;
    }
    else if (atoi(SplitVersionNum[0].c_str()) == atoi(SplitRecvVersionNum[0].c_str()) && atoi(SplitVersionNum[1].c_str()) < atoi(SplitRecvVersionNum[1].c_str()))
    {
        cout<<"Self version one equal echo version but another is less than echo version to broadcast"<<endl;
        string self_id = Singleton<Config>::get_instance()->GetKID().c_str();
        Device2AllDevReq  device2AllDevReq;
        device2AllDevReq.set_version(version);
        device2AllDevReq.set_id(self_id);
        
        // 全网广播3次
        std::vector<std::string> sendid = RandomNodeId(2);
        cout <<"sendid size()=" <<sendid.size() << endl;
        
        for(unsigned int i =0;i<sendid.size();i++)
        {
            cout<<"send id ="<<sendid[i]<<endl;
            net_send_message<Device2AllDevReq>(sendid[i], device2AllDevReq);
        } 
        cout<<"Self version one equal echo version but another is less than echo version the broadcasted end! "<<endl;
    }
    else if (atoi(SplitVersionNum[0].c_str()) > atoi(SplitRecvVersionNum[0].c_str())|| ((atoi(SplitVersionNum[0].c_str()) == atoi(SplitRecvVersionNum[0].c_str()) && atoi(SplitVersionNum[1].c_str()) > atoi(SplitRecvVersionNum[1].c_str()))))
    {
        cout<<"Self version more than echo version do not update"<<endl;
        if( g_random_public_node.size() > 0 )
        {
            g_random_public_node.clear();
            cout<<"After g_random_public_node clear size()="<<g_random_public_node.size()<<endl;
        }  
        return;
    } 

    sleep(8);
    if (atoi(SplitVersionNum[0].c_str()) == atoi(SplitRecvVersionNum[0].c_str()) && atoi(SplitVersionNum[1].c_str()) == atoi(SplitRecvVersionNum[1].c_str()))
    {
        cout<<"自身版本和哈希与公网相同不用更换版本"<<endl;
        return;
    }
    cout<< "Handle-------recv------------" <<endl;
    DataTransReq req;
    req.set_version(getVersion());
    string local_id = Singleton<Config>::get_instance()->GetKID().c_str();
    req.set_id(local_id);
    cout<<"g_random_normal_node.size()="<<g_random_normal_node.size()<<endl;
    if(g_random_normal_node.size() >0)
    {
        srand(time(NULL));
        int i = rand() % g_random_normal_node.size();
        cout<<"random i ="<< i <<endl;
        cout<<"g_random_normal_node[i] ="<<g_random_normal_node[i]<<endl;
        cout<<"有普通节点与公网节点版本与哈希相同向普通节点请求"<<endl;
        net_send_message<DataTransReq>(g_random_normal_node[i], req);
        if( g_random_normal_node.size() > 0 )
        {
            g_random_normal_node.clear();
            cout<<"After g_random_normal_node clear size()="<<g_random_normal_node.size()<<endl;
        }  
    }
    else 
    { 
        cout<<"自身版本和哈希与公网不同时向公网节点请求"<<endl;
        net_send_message<DataTransReq>(g_random_public_node[0], req);
        if( g_random_public_node.size() > 0 )
        {
            g_random_public_node.clear();
            cout<<"After g_random_public_node clear size()="<<g_random_public_node.size()<<endl;
        }      
    }
    return;
}

//回应广播
void  HandleUpdateProgramNodeBroadcast(const std::shared_ptr<Device2AllDevReq>& req, const MsgData& msgdata)
{
    cout<<"OneNode2DeviceBroadcast"<<endl;
    Feedback2DeviceAck feedback2DeviecAck;
    string version = req->version();
    string id = req->id();
    cout<<"recv id ="<<id<<endl;
    if(version.empty()) 
    {
        cout<<"Broadcast response version is empty"<<endl;
        return;
    }

    string self_id = Singleton<Config>::get_instance()->GetKID().c_str();
    cout << "passive accept the broadcast node self_id =" << self_id <<endl;

    std::vector<std::string> SelfVersionNum;
	SplitString(getVersion(), SelfVersionNum, "_");
    if(SelfVersionNum.size()==0) 
    {
        cout<<"Broadcast response SelfVersionNum size is zero"<<endl;
        return;
    }
    std::vector<std::string> SplitVersionNum;
    SplitString(SelfVersionNum[1], SplitVersionNum, ".");

    // cout<<"Split Self Version Num size = "<<SplitVersionNum.size()<<endl;
    // for(auto &self :SplitVersionNum)
    // {
    //     cout<<"SplitSelfVersionNum="<< self<<endl;
    // }

    std::vector<std::string> RecvVersion;
    SplitString(version, RecvVersion, "_");
    if(RecvVersion.size() == 0 )
    {
        cout<<"Broadcast RecvVersion size is zero"<<endl;
        return;
    }
    std::vector<std::string> SplitRecvVersionNum;
    SplitString(RecvVersion[1], SplitRecvVersionNum, ".");
    // cout<<"Split echo Version Num size = "<<SplitRecvVersionNum.size()<<endl;
    // for(auto &echo :SplitRecvVersionNum)
    // {
    //     cout<<"SplitechoVersionNum="<< echo<<endl;
    // }

	cout<<"one random response start "<<endl;
    feedback2DeviecAck.set_version(getVersion());
	if (atoi(SplitVersionNum[0].c_str()) == atoi(SplitRecvVersionNum[0].c_str()) && atoi(SplitVersionNum[1].c_str()) == atoi(SplitRecvVersionNum[1].c_str()))
    {
        feedback2DeviecAck.set_id(self_id); 
    }
    else if (atoi(SplitVersionNum[0].c_str()) != atoi(SplitRecvVersionNum[0].c_str()) && atoi(SplitVersionNum[1].c_str()) != atoi(SplitRecvVersionNum[1].c_str()))
    {
        cout<<"one response i have not  the version "<<endl;
    }
    net_send_message<Feedback2DeviceAck>(id, feedback2DeviecAck);
    cout<<"feedback2DeviceAck over!"<<endl;
    return;
}


void HandleUpdateProgramNodeId(const std::shared_ptr<Feedback2DeviceAck>& ack)
{
    cout<<"HandleSelectEchoNodeId"<<endl;
    std::lock_guard<std::mutex> lck(mu_return);
    string normal_id = ack->id();
    if(normal_id.empty())
    {
        cout<<"normal_id.empty()"<<endl;
        return;
    }
    else
    {
        cout<<"SelectEchoNodeid normal_id= " << normal_id<<endl;
        g_random_normal_node.push_back(normal_id);
        std::sort(g_random_normal_node.begin(), g_random_normal_node.end());
        g_random_normal_node.erase(unique(g_random_normal_node.begin(), g_random_normal_node.end()), g_random_normal_node.end());
        cout<<"HandleSelectEchoNodeId  g_random_normal_node.size() = "<<g_random_normal_node.size()<<endl; 
        return;  
    }
}


void  HandleUpdateProgramdata(const std::shared_ptr<DataTransReq>& req, const MsgData& msgdata)
{
    TransData ack;
    string id = req->id();
    cout<<"id ="<<id<<endl;
    ack.set_version(getVersion());
    ack.set_id(id);
    cout<<"--------HandleStart-------"<<endl;
    char path[256]= {0};
    get_exec_name(path,sizeof(path)); 

    FILE * fp = fopen(path, "rb");
   
    if(fp == NULL)    
    {
        ack.set_description("文件打开失败");
        cout<<"--------文件打开失败-------"<<endl;
        net_send_message<TransData>(msgdata, ack);
    }

    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    rewind(fp);

    if (len == 0)
    {
        ack.set_description("文件有误");
        cout<<"--------文件有误-------"<<endl;
        net_send_message<TransData>(id, ack);
    }

    char * buf = new char[len + 1];
    memset(buf, 0, len + 1);
    int ret = fread(buf, 1, len + 1, fp);
    cout<<"ret ="   <<ret  <<endl;
    string out = std::string(buf, len);
    
    string datastr = getsha256hash(out);
    ack.set_origindata(out);
    ack.set_encodedata(datastr);
    ack.set_description("传输数据");
    cout<<"--------开始传输数据-------"<<endl;
    net_send_message<TransData>(id, ack);
    cout<<"--------传输数据结束-------"<<endl;
    fflush(fp);
    fclose(fp);
    free(buf);
    return;
}

void removefile(char *name)//remove file and empty dir
{
	DIR *d;
	if ((d = opendir(name)) == NULL)
	{
		printf("file:%s is moved!\n", name);
		unlink(name);
	}
	else if ((rmdir(name)) == 0)
    {
        printf("dir:%s is moved!\n", name);
    }		
	else
	{
        printf("dir name:%s\n", name);
        removedir(name);
	}
}
 
void removedir(char* dir)//check the dir
{
	struct dirent *dp;
	DIR *d;
	char bb[100] = "";
	d = opendir(dir);
	getcwd(bb, 100);
	while((dp = readdir(d)) != NULL)
	{
		if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
		continue;
		chdir(dir);
		//printf("\nfind :%s\n", dp->d_name);
        cout<<"find :"<< dp->d_name<<endl;
		removefile(dp->d_name);
	}
	closedir(d);
	chdir(bb);
	removefile(dir);
	printf("%s is moved!\n", dir);
}


void HandleUpdateProgramRecoveryFile(const std::shared_ptr<TransData>& req)
{
    cout<<"-----Recoveryfile--start-----"<<endl;

    string data  = req->origindata();
    string recv_datastr = getsha256hash(data);
   
    cout<<"recv_datastr = "<< recv_datastr <<endl;
    if(!recv_datastr.compare(req->encodedata()))
    {
        string version = req->version();
        string oldname = "ebpc_"  + version ;
        string newname = "ebpc_wm" ;
        FILE * fp = fopen(oldname.c_str(), "wb+");
        chmod(oldname.c_str(),S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IXOTH);
        fwrite(data.c_str(), sizeof(char), data.size(), fp);
        rename(oldname.c_str(),newname.c_str());
        chmod(newname.c_str(),S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IXOTH);
        fflush(fp);
        fclose(fp);
    }

    cout<<"-----Recoveryfile--end---"<<endl;
    char szBuf[128];
    char szPath[128];
    
    memset(szBuf, 0x00, sizeof( szBuf));    
    memset( szPath, 0x00, sizeof(szPath));
    cout<<"start------ delete  executive  file"<<endl;
    getcwd(szBuf, sizeof(szBuf)-1);
    cout<<"buf="<<szBuf <<endl;
    int ret =  readlink("/proc/self/exe", szPath, sizeof(szPath)-1 );
    cout<<"ret ="<< ret <<endl;
    cout<<"path="<< szPath <<endl;
   
	removefile(szPath);
    kill(getpid(), SIGKILL);
    cout<<"delete  executive  file -----end"<<endl;
    return ;
}


std::vector<std::string> RandomNodeId(unsigned int n)
{
    unsigned int nodeSize = n;
	std::vector<std::string> sendid;

    std::vector<Node> nodeinfo;
    std::vector<std::string> id;
    auto nodelist = Singleton<PeerNode>::get_instance()->get_nodelist();

    for(auto& node:nodelist)
    {
        if(node.fee >0 )
        {
            nodeinfo.push_back(node);
        }
    }
    
    for(auto & nodemessage:nodeinfo)
    {
        if (nodemessage.public_ip == nodemessage.local_ip && isPublicIp(IpPort::ipsz(nodemessage.public_ip)))
        {
            cout<<"nodemessage.public_ip="<<nodemessage.public_ip<<endl;
            continue;
        }
        uint64_t amount = 0;
        if (SearchPledge(nodemessage.base58address, amount) != 0)
        {
            cout<<"随机选几点的时候质押查询出错"<<endl;
            continue ;
        }

        if (amount < g_TxNeedPledgeAmt)
        {
            // 质押金额不足
            cout << "Less amount of pledge " << endl;
            continue ;
        }

        if(amount >= g_TxNeedPledgeAmt)
        {
            id.push_back(nodemessage.id);
            std::sort(id.begin(), id.end());
            id.erase(unique(id.begin(), id.end()), id.end());
        }
    }
    
    if ((unsigned int)id.size() < nodeSize)
	{
		debug("not enough node to send");
		return sendid;
	}

	srand(time(NULL));
    while (1)
    {
        cout<<"start random "<<endl;
        int i = rand() % id.size();
        sendid.push_back(id[i]);
        cout<<"sendid.push id"<<id[i]<<endl;
        std::sort(sendid.begin(), sendid.end());
        sendid.erase(unique(sendid.begin(), sendid.end()), sendid.end());
        if (sendid.size() == nodeSize)
        {
            break;
        }
        cout<<"random end "<<endl;
    }
	return sendid;
}

