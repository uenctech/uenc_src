#include "ca_MultipleApi.h"
#include "ca_txhelper.h"
#include "ca_phonetx.h"
using std::cout;
using std::endl;
using std::string;


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
            if (v.second != 0 &&  v.second >= 1000  &&  v.second <=100000) 
            {

                node_fee_list.push_back(v.second);
            }
        }

        sort(node_fee_list.begin(), node_fee_list.end());
        uint32_t fee_list_count = static_cast<uint32_t>(node_fee_list.size());

        uint32_t list_begin = fee_list_count * 0.51;
        uint32_t list_end = fee_list_count;

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
        if(min_fee == 0)
        {
            min_fee = 1000;
        }
        if( min_fee < 1|| max_fee > 100000)
        {
            node_fee_ack.set_code(-1);
            node_fee_ack.set_description("单节点签名费错误");
            return ;
        }
    }

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
    double nodesignfee = stod(fee_req->service_fee());
    if(nodesignfee < 0.001 || nodesignfee > 0.1)
    {
        fee_ack.set_code(-7);
        fee_ack.set_description("滑动条数值显示错误");
        return;
    }
    uint64_t service_fee = (uint64_t)(stod(fee_req->service_fee()) * DECIMAL_NUM);
    
    auto rdb_ptr = MagicSingleton<Rocksdb>::GetInstance();

    auto status = rdb_ptr->SetDeviceSignatureFee(service_fee);
    if (!status) 
    {
        net_update_fee_and_broadcast(service_fee);
    }

    fee_ack.set_code(status);
    fee_ack.set_description("设置成功");
    
    return;
}

void GetServiceInfo(const std::shared_ptr<TApiGetBlockTopAck> &blktop_ack, GetServiceInfoAck &response_ack, bool is_public_node)
{
    int db_status = 0; 
    auto rdb_ptr = MagicSingleton<Rocksdb>::GetInstance();
    Transaction* txn = rdb_ptr->TransactionInit();
    if (txn == NULL) 
    {

    }

    ON_SCOPE_EXIT 
    {
        rdb_ptr->TransactionDelete(txn, false);
    };

    unsigned top = 0;
    rdb_ptr->GetBlockTop(txn, top);

    unsigned s_top = blktop_ack->top();

    if (top + 5 >= s_top) 
    {
        response_ack.set_is_sync(GetServiceInfoAck::TRUE);
    } 
    else 
    {
        response_ack.set_is_sync(GetServiceInfoAck::FALSE);
    }

    
    if (is_public_node) response_ack.set_is_sync(GetServiceInfoAck::TRUE);

    
    std::vector<std::string> hash;
    db_status = rdb_ptr->GetBlockHashsByBlockHeight(txn, top, hash);
    if (db_status) 
    {

    }
    std::string block_hash = hash[0];

    

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

            }
            block.ParseFromString(serialize_block);

            
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
                            break;
                        }
                    }
                }
            }
            if (i == (int32_t)top) 
            {
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
    }

    uint64_t show_service_fee = 0;

    db_status =rdb_ptr->GetDeviceSignatureFee(show_service_fee);
    if (db_status) 
    {
        
    }

    if (!show_service_fee) 
    {
        
        def_fee = 0;
    }
    else
    {
        def_fee = show_service_fee;
    }
    if (top == 100) 
    {
        avg_fee = count/100;
    } 
    else 
    {
        avg_fee = (max_fee + min_fee)/2;
    }

    min_fee = 1000;
    max_fee = 100000;

  
    response_ack.set_version(getVersion());
    response_ack.set_code(0);
    response_ack.set_description("获取成功");

    response_ack.set_mac_hash("test_hash"); 
    response_ack.set_device_version(g_LinuxCompatible); 

    auto service_fee = response_ack.add_service_fee_info();
    service_fee->set_max_fee(to_string(((double)max_fee)/DECIMAL_NUM));
    service_fee->set_min_fee(to_string(((double)min_fee)/DECIMAL_NUM));
    service_fee->set_service_fee(to_string(((double)def_fee)/DECIMAL_NUM));
    service_fee->set_avg_fee(to_string(((double)avg_fee)/DECIMAL_NUM));

    return;
}

uint64_t getAvgFee()
 {
    int db_status = 0; 
    auto rdb_ptr = MagicSingleton<Rocksdb>::GetInstance();
    Transaction* txn = rdb_ptr->TransactionInit();
    if (txn == NULL) 
    {
        
    }

    ON_SCOPE_EXIT 
    {
        rdb_ptr->TransactionDelete(txn, false);
    };

    unsigned top = 0;
    rdb_ptr->GetBlockTop(txn, top);

    
    std::vector<std::string> hash;
    db_status = rdb_ptr->GetBlockHashsByBlockHeight(txn, top, hash);
    if (db_status) 
    {
        
    }
    std::string block_hash = hash[0];

    

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
            
        }
        block.ParseFromString(serialize_block);

        
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
                        break;
                    }
                }
            }
        }

        if (i == (int32_t)top) 
        {
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
        
        def_fee = 0;
    } 
    else 
    {
        def_fee = show_service_fee;
    }
    (void)def_fee;
    if (top == 100)
    {
        avg_fee = count/100;
    } 
    else
    {
        avg_fee = (max_fee + min_fee)/2;
    }

    return avg_fee;
}

void HandleGetServiceInfoReq(const std::shared_ptr<GetServiceInfoReq>& msg, 
    const MsgData& msgdata) 
    {

        if (is_version) 
        {
            GetServiceInfoAck ack;

            ack.set_version(getVersion());
            ack.set_code(is_version);
            ack.set_description("版本错误");

            net_send_message<GetServiceInfoAck>(msgdata, ack);
            return;
        }

        std::string node_id;

        GetBlockTopReq get_blktop_req;
        
        get_blktop_req.set_version(getVersion());
        get_blktop_req.set_fd(msgdata.fd);
        get_blktop_req.set_port(msgdata.port);
        get_blktop_req.set_ip(msgdata.ip);

        if (Singleton<Config>::get_instance()->GetIsPublicNode() || msg->is_show()) 
        {
            Node node = net_get_self_node();
            if (IpPort::ipsz(node.public_ip) == msg->public_net_ip() || msg->is_show()) 
            {
                GetServiceInfoAck get_sinfo_ack;
                std::shared_ptr<TApiGetBlockTopAck> ack_msg = make_shared<TApiGetBlockTopAck>();
                ack_msg->set_version(getVersion());
                ack_msg->set_code(0);
                ack_msg->set_description("公网节点");

                ack_msg->set_top(0);
                ack_msg->set_fd(0);
                ack_msg->set_port(0);
                ack_msg->set_ip(0);

                GetServiceInfo(ack_msg, get_sinfo_ack, true);
                net_send_message<GetServiceInfoAck>(msgdata, get_sinfo_ack);
                return;
            }
        } 
        else 
        {
            node_id = net_get_ID_by_ip(msg->public_net_ip());

            
            if (node_id.empty())
            {
                std::vector<Node> vNode = net_get_public_node();
                if (vNode.size() != 0)
                {
                    std::random_shuffle(vNode.begin(), vNode.end());
                    node_id = vNode[0].id;
                }
            }
        }


        if (node_id.empty())
        {
            GetServiceInfoAck response_ack;
            response_ack.set_version(getVersion());
            response_ack.set_code(-404);
            response_ack.set_description("连接失败超时");

            response_ack.set_mac_hash("test_hash"); 
            response_ack.set_device_version(g_LinuxCompatible); 

            uint64_t service_fees = 0;

            auto rdb_ptr = MagicSingleton<Rocksdb>::GetInstance();
            rdb_ptr->GetDeviceSignatureFee(service_fees);

            auto service_fee = response_ack.add_service_fee_info();
            service_fee->set_max_fee(to_string(0.1));
            service_fee->set_min_fee(to_string(0.001));
            service_fee->set_service_fee(to_string(((double)service_fees)/DECIMAL_NUM));
            service_fee->set_avg_fee(to_string(((double)getAvgFee())/DECIMAL_NUM));
            response_ack.set_is_sync(GetServiceInfoAck::FAIL);

            net_send_message<GetServiceInfoAck>(msgdata, response_ack);
        } 
        else 
        {
            net_send_message<GetBlockTopReq>(node_id, get_blktop_req);
        }
}

void HandleGetBlockTopReq(const std::shared_ptr<GetBlockTopReq>& msg, 
    const MsgData& msgdata) 
{

    TApiGetBlockTopAck get_blktop_ack;
    get_blktop_ack.set_version(getVersion());
    get_blktop_ack.set_fd(msg->fd());
    get_blktop_ack.set_port(msg->port());
    get_blktop_ack.set_ip(msg->ip());

    auto rdb_ptr = MagicSingleton<Rocksdb>::GetInstance();
    Transaction* txn = rdb_ptr->TransactionInit();
    if (txn == NULL) 
    {
        
    }

    ON_SCOPE_EXIT {
        rdb_ptr->TransactionDelete(txn, false);
    };

    unsigned top = 0;
    rdb_ptr->GetBlockTop(txn, top);
    get_blktop_ack.set_top(top);

    net_send_message<TApiGetBlockTopAck>(msgdata, get_blktop_ack);
}

void TApiGetBlockTopAckFunc(const std::shared_ptr<TApiGetBlockTopAck>& msg, 
    const MsgData& msgdata) 
{
   
    GetServiceInfoAck get_sinfo_ack;
    GetServiceInfo(msg, get_sinfo_ack, false);

    MsgData phone_msgdata;
    phone_msgdata.fd = msg->fd();
    phone_msgdata.port = msg->port();
    phone_msgdata.ip = msg->ip();

    net_send_message<GetServiceInfoAck>(phone_msgdata, get_sinfo_ack);
}

void GetPacketFee(const std::shared_ptr<GetPacketFeeReq> &packet_req, GetPacketFeeAck &packet_ack) 
{
    using namespace std;

    auto rdb_ptr = MagicSingleton<Rocksdb>::GetInstance();
    Transaction* txn = rdb_ptr->TransactionInit();
    if (txn == NULL) 
    {
        
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
        
    }

    bool bRollback = false;
    ON_SCOPE_EXIT{
        rdb_ptr->TransactionDelete(txn, bRollback);
    };

    
    string serialize_header;
    rdb_ptr->GetBlockByBlockHash(txn, block.hash(), serialize_header);
    CBlock cblock;
    cblock.ParseFromString(serialize_header);

    
    auto blocks = block_info_ack.add_block_info_list();

    blocks->set_height(block.height());
    blocks->set_hash_merkle_root(cblock.merkleroot());
    blocks->set_hash_prev_block(cblock.prevhash());
    blocks->set_block_hash(cblock.hash());
    blocks->set_ntime(cblock.time());

    for (int32_t i = 0; i < cblock.txs_size(); i++) 
    {
        CTransaction tx = cblock.txs(i);

        
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

        
        for (int32_t j = 0; j < tx.vin_size(); j++) 
        {
            auto vin_list = tx_info->add_vin_list();

            CTxin txin = tx.vin(j);
            CScriptSig scriptSig = txin.scriptsig();
            int scriptSigLen = scriptSig.sign().size();
            char * hexStr = new char[scriptSigLen * 2 + 2]{0};

            std::string sign_str(scriptSig.sign().data(), scriptSig.sign().size());

            if (sign_str == FEE_SIGN_STR || sign_str == EXTRA_AWARD_SIGN_STR) 
            {
                vin_list->set_script_sig(sign_str);
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
    

    return;
}

void BlockInfoReq(const std::shared_ptr<GetBlockInfoReq> &block_info_req, GetBlockInfoAck &block_info_ack) 
{
    using std::cout;
    using std::endl;

    int32_t height = block_info_req->height();
    int32_t count = block_info_req->count();

    int db_status;
    auto rdb_ptr = MagicSingleton<Rocksdb>::GetInstance();
    Transaction* txn = rdb_ptr->TransactionInit();
    if (txn == NULL) 
    {
        
    }

    ON_SCOPE_EXIT {
        rdb_ptr->TransactionDelete(txn, false);
    };

    
    std::string bestChainHash;
    db_status = rdb_ptr->GetBestChainHash(txn, bestChainHash);
    if (db_status != 0) 
    {
        
    }
    if (bestChainHash.size() == 0) 
    { 
        return;
    }
    

    uint32_t top;
    db_status = rdb_ptr->GetBlockTop(txn, top);
    if (db_status) 
    {
        
    }

    
    height = height > static_cast<int32_t>(top) ? top : height;
    height = height < 0 ? top : height;
    count = count > height ? height : count;
    count = count < 0 ? top : count; 

    
    std::vector<std::string> hash;

    
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
            
        }
        for (auto hs : hash)
        {
            db_status = rdb_ptr->GetBlockHeaderByBlockHash(txn, hs, serialize_block);
            if (db_status) 
            {
                
            }
            block.ParseFromString(serialize_block);
            
            AddBlockInfo(block_info_ack, block);
        }
        hash.clear();
        
        if (i == 0) 
        {
            break;
        }
    }

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

    if (originPass != targetPass) {
        pass_ack.set_version(getVersion());
        pass_ack.set_code(-2);
        pass_ack.set_description("密码错误");
        return;
    }

    pass_ack.set_version(getVersion());
    pass_ack.set_code(code);
    pass_ack.set_description(description);

    pass_ack.set_address(g_AccountInfo.DefaultKeyBs58Addr);

    return;
}

void DevPasswordReq(const std::shared_ptr<SetDevPasswordReq> &pass_req, SetDevPasswordAck &pass_ack) 
{
    using std::cout;
    using std::endl;

    std::string old_pass = pass_req->old_pass();
    std::string new_pass = pass_req->new_pass();

    pass_ack.set_version(getVersion());
    if (old_pass.empty() || new_pass.empty()) {
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

    int db_status = 0; 
    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
    Transaction* txn = pRocksDb->TransactionInit();
    if(!txn) 
    {
       
    }

    std::string retString;
    ON_SCOPE_EXIT{
        pRocksDb->TransactionDelete(txn, false);
    };

    std::string bestChainHash;
    db_status = pRocksDb->GetBestChainHash(txn, bestChainHash);
    if (db_status) 
    {
       
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
      
    }

    std::reverse(vTxHashs.begin(), vTxHashs.end());

    std::vector<std::string> vBlockHashs;
    for (auto strTxHash : vTxHashs) 
    {
        std::string blockHash;
        db_status = pRocksDb->GetBlockHashByTransactionHash(txn, strTxHash, blockHash);
        if (!db_status) 
        {
          
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
         
        }
        CBlockHeader block;
        block.ParseFromString(serBlock);
        
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
    uint64_t amount = std::stoi(msg->amt().c_str()) * DECIMAL_NUM;
    uint32_t needverifyprehashcount  = std::stoi(msg->needverifyprehashcount()) ;
  
    if(msg->addr().size()<= 0 || amount <= 0 || needverifyprehashcount < 3 || gasFee <= 0)
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
   
    if(fromAddr.size()<= 0 || amount <= 0 || needverifyprehashcount < 3 || gasFee <= 0||txhash.empty())
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
    extra["PackageFee"] = packageFee; 
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

    
	if( 0 != IsVersionCompatible(getVersion() ) )
	{
		txMsgAck.set_code(-101);
		txMsgAck.set_message("Version incompatible!");
        net_send_message<TxMsgAck>(msgdata, txMsgAck);
		return ;
	}
	
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
        
        PledgeItem * pItem = ack.add_list();
        
        pItem->set_blockhash(block.hash());
        pItem->set_blockheight(block.height());
        pItem->set_utxo(strUtxo);
        pItem->set_time(tx.time());
        
        pItem->set_fromaddr(tx.txowner());

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

    reverse(txHashs.begin(), txHashs.end());

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
            
            TxInfoItem * pItem = ack.add_list();
            pItem->set_txhash(tx.hash());
            pItem->set_time(tx.time());

            std::vector<std::string> owners = TxHelper::GetTxOwner(tx);
            if (owners.size() == 1 && tx.vout_size() == 2)
            {
                
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

                if (owners[0] != tx.txowner() && tx.txowner().length() != 0)
                {
                    owners[0] = tx.txowner();
                }

                if (owners[0] == addr && 
                    owners[0] == tx.vout(0).scriptpubkey() && 
                    tx.vout(0).scriptpubkey() == tx.vout(1).scriptpubkey())
                {
                    
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
                    
                    if (tmpTxOut.scriptpubkey() != addr)
                    {
                        amount += tmpTxOut.value();
                    }  
                }
                else
                {
                    
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
        i--; 
    }

    
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


void HandleGetBlockInfoListReq(const std::shared_ptr<GetBlockInfoListReq>& msg, const MsgData& msgdata)
{
    
    GetBlockInfoListAck getBlockInfoListAck;
    getBlockInfoListAck.set_version( getVersion() );

    
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

            for (auto & owner : owners)
            {
                for (auto & txOut : tx.vout())
                {
                    if (owner != txOut.scriptpubkey())
                    {
                        pBlockInfoItem->add_toaddr(txOut.scriptpubkey());
                        amount += txOut.value();
                    }
                }
            }

            pBlockInfoItem->set_amount(to_string((double_t) amount / DECIMAL_NUM));
        }
    }

    getBlockInfoListAck.set_code(0);
    getBlockInfoListAck.set_description("successful");
    net_send_message<GetBlockInfoListAck>(msgdata, getBlockInfoListAck);
}


void HandleGetBlockInfoDetailReq(const std::shared_ptr<GetBlockInfoDetailReq>& msg, const MsgData& msgdata)
{
    GetBlockInfoDetailAck getBlockInfoDetailAck;
    getBlockInfoDetailAck.set_version( getVersion() );

    
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

    uint64_t totalAmount = 0;
    
    
    std::vector<std::string> txOwners;
    txOwners = TxHelper::GetTxOwner(tx);
    
    
    bool isRedeem = false;
    if (tx.vout_size() == 2)
    {
        if (tx.vout(0).scriptpubkey() == tx.vout(1).scriptpubkey() && 
            tx.txowner() == tx.vout(0).scriptpubkey())
        {
            isRedeem = true;
        }
    }

    if (isRedeem)
    {
        uint64_t amount = tx.vout(0).value();

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
    
        uint64_t gas = 0;
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
        
        for (int j = 0; j < tx.vout_size(); ++j)
        {
            CTxout txout = tx.vout(j);
            if (txOwners.end() == find (txOwners.begin(), txOwners.end(), txout.scriptpubkey() ) )
            {
                
                totalAmount += txout.value();

                
                BlockInfoOutAddr * pBlockInfoOutAddr = getBlockInfoDetailAck.add_blockinfooutaddr();
                pBlockInfoOutAddr->set_addr(txout.scriptpubkey());

                uint64_t value = txout.value();
                std::string amountStr = std::to_string( (double)value / DECIMAL_NUM );
                pBlockInfoOutAddr->set_amount( amountStr );
            }
        }
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

    uint64_t amount = 0;

    
    bool isRedeem = false;
    if (tx.vout_size() == 2)
    {
        if (tx.vout(0).scriptpubkey() == tx.vout(1).scriptpubkey() && 
            tx.txowner() == tx.vout(0).scriptpubkey())
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

        uint64_t gas = 0;
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

        toaddr->set_amt(to_string( (double_t)amount / DECIMAL_NUM) );

        
        
    }
    else
    {
        for (auto & txout : tx.vout())
        {
            std::string targetAddr = txout.scriptpubkey();
            for (auto owner : owners)
            {
                if (owner != txout.scriptpubkey())
                {
                    amount += txout.value();
                    ToAddr * toaddr = ack.add_toaddr();
                    toaddr->set_toaddr(targetAddr);
                    toaddr->set_amt(to_string( (double_t)txout.value() / DECIMAL_NUM) );
                }
            }
        }
    }
 
    ack.set_amount(to_string((double_t)amount / DECIMAL_NUM));

    uint64_t totalAward = 0; 
    uint64_t awardAmount = 0; 
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

    uint64_t gas = 0;
    uint64_t awardGas = 0;
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

    ack.set_code(0);
    ack.set_description("success");
}





void HandleCreateDevicePledgeTxMsgReq(const std::shared_ptr<CreateDevicePledgeTxMsgReq>& msg, const MsgData &msgdata )
{
    CreatePledgeTransaction(msg->addr(), msg->amt(), std::stoi(msg->needverifyprehashcount()), msg->gasfees(), msgdata );
    return ;
}


void HandleCreateDeviceRedeemTxReq(const std::shared_ptr<CreateDeviceRedeemTxReq> &msg, const MsgData &msgdata )
{
    CreateRedeemTransaction(msg->addr(), std::stoi(msg->needverifyprehashcount()), msg->gasfees(), msg->utxo(), msgdata );
    return ;
}


void assign(const std::shared_ptr<Message> &msg, const MsgData &msgdata, const std::string msg_name) 
{
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

        HandleGetBlockTopReq(ack_msg, msgdata);
    } 
    else if (msg_name == "TApiGetBlockTopAck") 
    {
        const std::shared_ptr<TApiGetBlockTopAck>ack_msg = dynamic_pointer_cast<TApiGetBlockTopAck>(msg);

        TApiGetBlockTopAckFunc(ack_msg, msgdata);
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
}



void verify(const std::shared_ptr<Message> &msg, const MsgData &msgdata) 
{
    const google::protobuf::Descriptor *descriptor = msg->GetDescriptor();
    const google::protobuf::Reflection *reflection = msg->GetReflection();

    const google::protobuf::FieldDescriptor* field = descriptor->field(0);

    std::string version = reflection->GetString(*msg, field);

    is_version = IsVersionCompatible(version);

    assign(msg, msgdata, descriptor->name());
}

}

void MuiltipleApi() 
{
    
    
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

    
    net_register_callback<GetServiceInfoReq>([](const std::shared_ptr<GetServiceInfoReq>& msg, 
    const MsgData& msgdata) 
    {
        m_api::verify(msg, msgdata);
    });

    
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