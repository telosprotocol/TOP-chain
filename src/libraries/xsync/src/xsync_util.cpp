#include "xsync/xsync_util.h"
#include "xdata/xnative_contract_address.h"
#include "xsync/xsync_message.h"

NS_BEG2(top, sync)

data::xblock_ptr_t autoptr_to_blockptr(base::xauto_ptr<base::xvblock_t> &autoptr) {
    if (autoptr == nullptr)
        return nullptr;

    autoptr->add_ref();
    base::xvblock_t *vblock = autoptr.get();
    data::xblock_t *block = (data::xblock_t*)vblock;
    data::xblock_ptr_t block_ptr = nullptr;
    block_ptr.attach(block);

    return block_ptr;
}

bool is_beacon_table(const std::string &address) {

    std::string account_prefix;
    uint32_t table_id = 0;
    if (!data::xdatautil::extract_parts(address, account_prefix, table_id)) {
        return false;
    }

    if (account_prefix != common::rec_table_base_address.to_string())
        return false;

    return true;
}

enum_result_code check_auth(const observer_ptr<base::xvcertauth_t> &certauth, data::xblock_ptr_t &block) {
    XMETRICS_TIME_RECORD("xsync_store_check_auth");

    // No.1 safe rule: clean all flags first when sync/replicated one block
    block->reset_block_flags();

    XMETRICS_GAUGE(metrics::cpu_ca_verify_multi_sign_sync, 1);
    base::enum_vcert_auth_result  result = certauth->verify_muti_sign(block.get());
    if (result != base::enum_vcert_auth_result ::enum_successful) {
        if (result == base::enum_vcert_auth_result ::enum_verify_fail || result == base::enum_vcert_auth_result ::enum_bad_cert 
         || result == base::enum_vcert_auth_result ::enum_bad_block || result == base::enum_vcert_auth_result ::enum_bad_address 
         || result == base::enum_vcert_auth_result ::enum_bad_signature || result == base::enum_vcert_auth_result ::enum_bad_scheme 
         || result == base::enum_vcert_auth_result ::enum_bad_consensus) {
            xwarn("xsync check_auth failed, %s,result=%d", block->dump().c_str(), result);
            return enum_result_code::auth_failed;
        } else {
            xwarn("xsync check_auth failed-wait data, %s,result=%d", block->dump().c_str(), result);
            return enum_result_code::wait_auth_data;
        }
    }

    block->set_block_flag(base::enum_xvblock_flag_authenticated);
    xinfo("xsync check_auth ok, %s", block->dump().c_str());
    return success;
}

uint32_t vrf_value(const std::string& hash) {

    uint32_t value = 0;
    const uint8_t* data = (const uint8_t*)hash.data();
    for (size_t i = 0; i < hash.size(); i++) {
        value += data[i];
    }

    return value;
}
 
uint64_t derministic_height(uint64_t my_height, std::pair<uint64_t, uint64_t> neighbor_heights) {
    uint64_t height = my_height;

    if (my_height >= neighbor_heights.first + xsync_store_t::m_undeterministic_heights) {
        height = my_height - xsync_store_t::m_undeterministic_heights;
    } else {
        height = neighbor_heights.first;
    }
    return height;
}

std::vector<std::string> convert_blocks_to_stream(uint32_t data_type, const std::vector<data::xblock_ptr_t>& block_vec)
{
    std::vector<std::string> blocks_str_vec;

    for (auto& block : block_vec) {
        base::xstream_t stream(base::xcontext_t::instance());

        if (data_type == enum_sync_data_all) {
            block->full_block_serialize_to(stream);
        } else {
            if (data_type & enum_sync_data_header) {
                std::string block_object_bin;
                block->serialize_to_string(block_object_bin);
                stream << block_object_bin;
            }
            if (data_type & enum_sync_data_input) {
                stream << block->get_input_data();
            }
            if (data_type & enum_sync_data_output) {
                stream << block->get_output_data();
            }
            if (data_type & enum_sync_data_offdata) {
                if (!block->get_output_offdata_hash().empty()) {
                    stream << block->get_output_offdata();
                }
            }
        }
        blocks_str_vec.push_back(std::string((const char*)stream.data(), stream.size()));
    }

    return blocks_str_vec;
}


std::vector<data::xblock_ptr_t> convert_stream_to_blocks(uint32_t data_type, const std::vector<std::string>& blocks_data)
{
    std::vector<data::xblock_ptr_t> blocks_ptr_vec;
    for (auto& _block_data : blocks_data) {
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t*)_block_data.c_str(), _block_data.size());
        xassert(stream.size() > 0);
        data::xblock_t* _data_obj = nullptr;
        if (data_type == enum_sync_data_all) {
            _data_obj = dynamic_cast<data::xblock_t*>(data::xblock_t::full_block_read_from(stream));
        } else {
            std::string block_object_bin;
            stream >> block_object_bin;
            base::xvblock_t* new_block = base::xvblock_t::create_block_object(block_object_bin);
            if (new_block == nullptr) {
                xerror("get_all_xblock_ptr not valid block.object_bin=%ld", base::xhash64_t::digest(block_object_bin));
                new_block->release_ref();
                break;
            }

            if (new_block->get_header()->get_block_class() == base::enum_xvblock_class_nil) {
                new_block->release_ref();
                continue;
            }

            if (data_type & enum_sync_data_input) {
                std::string _input_content;
                stream >> _input_content;
                if (false == new_block->set_input_data(_input_content)) {
                    xerror("get_all_xblock_ptr set_input_data fail.block=%s,ir=%ld", new_block->dump().c_str(), base::xhash64_t::digest(_input_content));
                    new_block->release_ref();
                    continue;
                }
            }

            if (data_type & enum_sync_data_output) {
                std::string _output_content;
                stream >> _output_content;
                if (false == new_block->set_output_data(_output_content)) {
                    xerror("get_all_xblock_ptr set_input_data fail.block=%s,ir=%ld", new_block->dump().c_str(), base::xhash64_t::digest(_output_content));
                    new_block->release_ref();
                    break;
                }
            }

            if (data_type & enum_sync_data_offdata) {
                if (!new_block->get_output_offdata_hash().empty()) {
                    if (stream.size() == 0) {
                        xerror("get_all_xblock_ptrnot include output offdata failblock=%s", new_block->dump().c_str());
                        new_block->release_ref();
                        break;
                    }
                    std::string _out_offdata;
                    stream >> _out_offdata;
                    if (false == new_block->set_output_offdata(_out_offdata)) {
                        xerror("get_all_xblock_ptr set_output_offdata failblock=%s,offdata_size=%zu", new_block->dump().c_str(), _out_offdata.size());
                        new_block->release_ref();
                        break;
                    }
                }
            }
            _data_obj = dynamic_cast<data::xblock_t*>(new_block);
        }

        if (_data_obj != nullptr) {
            data::xblock_ptr_t block_ptr {};
            block_ptr.attach(_data_obj);
            block_ptr->reset_block_flags();
            blocks_ptr_vec.push_back(block_ptr);
        }
    }
    return blocks_ptr_vec;
}


bool sync_blocks_continue_check(const std::vector<data::xblock_ptr_t> &block_vec, const std::string account, bool check_hash)
{
    std::string check_account;
    auto it = block_vec.begin();
    auto last_hash = it->get()->get_block_hash();
    uint64_t last_height = it->get()->get_height();
    if (account.empty()) {
       check_account = it->get()->get_account();
    } else {
        check_account = account;
    }
    
    it++;
    for (; it != block_vec.end(); it++) {
        auto block = it->get();
        if (block->get_account() != check_account) {
            xwarn("sync_blocks_continue_check address error,%s,%s", block->get_account().c_str(), check_account.c_str());
            return false;
        }

        if (check_hash && block->get_last_block_hash() != last_hash) {
            xwarn("sync_blocks_continue_check hash error %s,h=%ld,%ld,%s,%s",
                account.c_str(), block->get_height(), last_height, block->get_last_block_hash().c_str(), last_hash.c_str());
            return false;
        }
        last_hash = block->get_block_hash();
        last_height = block->get_height();
    }
    return true;
}


NS_END2
