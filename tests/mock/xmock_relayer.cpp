#include "xmock_relayer.hpp"


namespace top
{
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


 void   relayer_mock::relayer_transaction_create(){

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
    if (!next_epoch_id.empty())
    {
        header.next_epoch_id.copyFromStr(next_epoch_id);
     }  else {
        xerror("next_epoch_id  is NULL");
     }
    
    std:::string next_epoch_id = key_info["inner_lite"]["next_epoch_id"].asString(); 
    if (!next_epoch_id.empty())
    {
        header.next_epoch_id.copyFromStr(next_epoch_id);
    }  else {
        xerror("next_epoch_id  is NULL");
    }


    
    




     if (key_info_js["db_path_num"] > 1) {   
            db_path_num = key_info_js["db_path_num"].asInt();
            for (int i = 0; i < db_path_num; i++) {
                std::string key_db_path = "db_path_" + std::to_string(i+1);
                std::string key_db_size = "db_path_size_" + std::to_string(i+1);
                std::string db_path_result =  key_info_js[key_db_path].asString();
                uint64_t db_size_result = key_info_js[key_db_size].asUInt64(); 
                if (db_path_result.empty() || db_size_result < 1) {
                    db_path_num = 1;
                    xwarn("xtop_application::read db %i path %s size %lld config failed!", i , db_path_result.c_str(), db_size_result);
                    break;
                }
                xinfo("xtop_application::read db  %i path %s size %lld sucess!",i , db_path_result.c_str(), db_size_result);
                db_data_paths.emplace_back(db_path_result, db_size_result);
            }
        }


    }

}
}