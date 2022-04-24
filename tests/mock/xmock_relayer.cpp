#include "xmock_relayer.hpp"


namespace top
{

    typedef  struct RECEIPTSRLP_LOG_DEF {
        h160 contractAddress;
        bytes32[] topics;
        bytes data;
    }receiptrlp_log_def;

    typedef  struct  RECEIPTSRLP_DEF{
        uint8_t status;
        h256 gasUsed;
        bytes logsBloom;
        receiptrlp_log_def logs;
    }receiptrlp_def;


    namespace mock
    {

    relayer_mock::relayer_mock(){
    
    }
    



    }

    relayer_mock::~relayer_mock()
    {


    }


 void   relayer_mock::relayer_init()
    {
            
        std::string  relayer_config =  "/mock_relayer.json"; 

        std::ifstream keyfile(relayer_config, std::ios::in);
        xinfo("relayer_mock::read   relayer_config %s", relayer_config.c_str());
        if (keyfile) {
            xJson::Value key_info_js;
            std::stringstream buffer;
            buffer << keyfile.rdbuf();
            keyfile.close();
            std::string key_info = buffer.str();
            xJson::Reader reader;
            // ignore any error when parse
            reader.parse(key_info, key_info_js);
            geneesis_block_init(key_info_js);

        }else {
            xerror("relayer_mock::read   relayer_config %s failed", relayer_config.c_str());
        }
    }

 void   relayer_mock::relayer_approval_add()
    {



    }


 void   relayer_mock::relayer_receipts_create(uint64_t height, uint32_t tx_count) {
     
    struct timeval beg;
    uint64_t rand_time = 0;

    std::vector<bytes> receipts;
    for (int i = 0; i < tx_count; i++) {
        receiptrlp_def receipt;

        receipt.status = 1;
        receipt.gasUsed = (i+1)*1000;

        receiptrlp_log_def logs;
        logs.


    typedef  struct RECEIPTSRLP_LOG_DEF {
        address contractAddress;
        bytes32[] topics;
        bytes data;
    }receiptrlp_log_def;



        gettimeofday(&beg, NULL);
        rand_time =  beg.tv_sec *1000 + beg.tv_usec;
        auto  rand_str = base::xstring_utl::tostring(rand_time);

        receipt.TxHash = top::utl::xsha2_256_t::digest(rand_str);
        std::cout << "i " << i << " TxHash " << TxHash.data() << std::endl;
        rand_time++;
        rand_str = base::xstring_utl::tostring(rand_time);

        receipt.Address =  top::utl::xsha2_256_t::digest(rand_str, 20);
        std::cout << "i " << i << " Address " << TxHash.data() << std::endl;

        receipt.GasUsed = i*123;

        rand_time++;
        rand_str = base::xstring_utl::tostring(rand_time);
        receipt.BlockHash =  top::utl::xsha2_256_t::digest(rand_str);
        receipt.TransactionIndex = i;
        
        bytes encoded = bytes();
        append(encoded, RLP::encode( receipt.Type));
        append(encoded, RLP::encode(receipt.Status));
        append(encoded, RLP::encode(receipt.CumulativeGasUsed));
        append(encoded, RLP::encode(bytes(receipt.Bloom.begin(), receipt.Bloom.end())));
        append(encoded, RLP::encode(bytes(receipt.TxHash.begin(), receipt.TxHash.end())));
        append(encoded, RLP::encode(bytes(receipt.Address.begin(),receipt.Address.end())));
        append(encoded, RLP::encode(receipt.GasUsed ));
        append(encoded, RLP::encode(bytes(receipt.BlockHash.begin(), receipt.BlockHash.end())));
        append(encoded, RLP::encode(receipt.TransactionIndex));
        receipts.push_back(encoded);
    }

    h256 receiptsRoot = orderedTrieRoot(receipts);
    std::cout << "MPT receipts Hash: " <<  receiptsRoot.hex()  << std::endl;



 }



 void   relayer_mock::relayer_fullOutProoff(){


}

 void    relayer_mock::geneesis_block_init(const xJson::Value &key_info) {

    x_LightClientBlockInnerHeader &header = m_genes_block.header;
    
    std::string prev_block_hash =  key_info["prev_block_hash"].asString();
    if (!prev_block_hash.mepty()) {
        header.prev_block_hash.copyFromStr(prev_block_hash);
     }  else {
        xerror("prev_block_hash   is NULL");
     }

     //next block  header hash
     std::string next_block_inner_hash = key_info["next_block_inner_hash"].asString();
     if (!next_block_inner_hash.mepty()) {
        header.next_block_inner_hash.copyFromStr(next_block_inner_hash);
     }  else {
        xerror("next_block_inner_hash   is NULL");
     }
     
    header.height = key_info["inner_lite"]["height"].asUInt64(); 
    
    std::string epoch_id = key_info["inner_lite"]["epoch_id"].asString();
    if (!epoch_id.mepty()) {
        header.epoch_id.copyFromStr(epoch_id);
     }  else {
        xerror("epoch_id  is NULL");
     }

    std:::string next_epoch_id = key_info["inner_lite"]["next_epoch_id"].asString(); 
    if (!next_epoch_id.empty()) {
        header.next_epoch_id.copyFromStr(next_epoch_id);
     }  else {
        xerror("next_epoch_id  is NULL");
     }
    
    std:::string next_epoch_id = key_info["inner_lite"]["prev_state_root"].asString(); 
    if (!next_epoch_id.empty()) {
        header.next_epoch_id.copyFromStr(next_epoch_id);
    }  else {
        xerror("next_epoch_id  is NULL");
    }



    header.timestamp = key_info["inner_lite"]["timestamp"].asUInt64(); 
    
    std:::string next_bp_hash = key_info["inner_lite"]["next_bp_hash"].asString(); 
    if (!next_bp_hash.empty()) {
        header.next_bp_hash.copyFromStr(next_bp_hash);
    }  else {
        xerror("next_bp_hash  is NULL");
    }
    
    std:::string block_merkle_root = key_info["inner_lite"]["block_merkle_root"].asString(); 
    if (!block_merkle_root.empty()) {
        header.block_merkle_root.copyFromStr(block_merkle_root);
    }  else {
        xerror("block_merkle_root  is NULL");
    }

    //todo
    std:::string outcome_root = key_info["inner_lite"]["outcome_root"].asString(); 
    if (!outcome_root.empty()) {
        header.outcome_root.copyFromStr(outcome_root);
    }  else {
        xerror("next_epoch_id  is NULL");
    }



    }

}
}