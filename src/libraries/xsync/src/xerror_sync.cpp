
#include "xsync/xerror_sync.h"

#include "xbase/xlog.h"
#include "xrpc/xuint_format.h"
#include "xsync/xsync_util.h"

NS_BEG2(top, sync)

XErrorSync_obj & XErrorSync_obj::instance() {
    static XErrorSync_obj _g_XErrorSync_obj_instance;
    return _g_XErrorSync_obj_instance;
}

XErrorSync_obj::~XErrorSync_obj() {
}

void XErrorSync_obj::set_error_height(xchain_state_info_t & info) {
    if (!m_time_height_rejecter.reject()) {
        m_error_height_type++;
        if (m_error_height_type > (int)(kErrorHeightSuper)) {
            m_error_height_type = (int)kErrorHeightNormal;
        }
    }

    switch (m_error_height_type) {
    case kErrorHeightSuper:
        info.end_height += 200;
        xinfo("XErrorSync_obj::error height end_height +200");
        break;
    case kErrorHeightNormal:
        xinfo("XErrorSync_obj::error height end_height normal");
        break;
    }
}

std::vector<top::data::xblock_ptr_t> XErrorSync_obj::set_error_block_vector(std::vector<top::data::xblock_ptr_t> const & vector_blocks) {
    if (!m_time_error_rejecter.reject()) {
        m_error_type++;
        if (m_error_type > (int)(kErrorQcertNonce)) {
            m_error_type = (int)kErrorNoError;
        }
    }

    switch ((xenum_error_sync)m_error_type) {
    case kErrorNoError:
        xinfo("XErrorSync_obj::set_error_block_vector no error");
        return vector_blocks;
    case kErrorSign:
        xinfo("XErrorSync_obj::set_error_block_vector sign error");
        return set_block_valid_sign(vector_blocks);
    case kErrorLowerRequestHeight:
        xinfo("XErrorSync_obj::set_error_block_vector RequestHeight");
        return set_block_lower_height(vector_blocks);
    case kErrorSuperRequestHiehgt:
        xinfo("XErrorSync_obj::set_error_block_vector SuperRequestHiehgt");
        return set_block_super_height(vector_blocks);
    case kErrorNotResponse:
        xinfo("XErrorSync_obj::set_error_block_vector no response");
        break;
    case kErrorQcertNonce:
        xinfo("XErrorSync_obj::set_error_block_vector Qcert nonce");
        return set_block_error_nonce(vector_blocks);
    default:
        xinfo("XErrorSync_obj::set_error_block_vector ");
        break;
    }
    return {};
}

std::vector<top::data::xblock_ptr_t> XErrorSync_obj::set_block_valid_sign(std::vector<top::data::xblock_ptr_t> const & vector_blocks) {
    if (vector_blocks.size() > 0) {
        auto block_str_vec = convert_blocks_to_stream(enum_sync_data_all, vector_blocks);
        auto new_block_vec = convert_stream_to_blocks(enum_sync_data_all, block_str_vec);

        for (auto & block : new_block_vec) {
            std::string hex_sig =
                "0x740000005800000002000000000000002100025701e98c80198da9c2c620866ec4eff40ff81512bd09e86993621a7a10fe2ea3200086e97a10412fd226d2fa066a981cb7877d6931490f4fc089c1e89e"
                "159eeed60b04000f";
            std::string muti_signature = xrpc::hex_to_uint8_str(hex_sig);
            block->reset_block_flags(block->get_block_flags() & base::enum_xvblock_flags_low4bit_mask);  // reset all status flags and redo it from authenticated status
            block->set_verify_signature(muti_signature);
            block->set_block_flag(base::enum_xvblock_flag_authenticated);
            xinfo("XErrorSync_obj::set_block_valid_sign set block valid sign block %s", block->dump().c_str());
        }
        return new_block_vec;
    }
    return {};
}

std::vector<top::data::xblock_ptr_t> XErrorSync_obj::set_block_lower_height(std::vector<top::data::xblock_ptr_t> const & vector_blocks) {
    if (vector_blocks.size() > 0) {
        auto block_str_vec = convert_blocks_to_stream(enum_sync_data_all, vector_blocks);
        auto new_block_vec = convert_stream_to_blocks(enum_sync_data_all, block_str_vec);

        auto & block = new_block_vec[0];
        uint64_t block_height = block->get_height();
        uint64_t height_interval = block_height / 2;

        for (auto & block : new_block_vec) {
            block->get_header()->set_height(block->get_height() - height_interval);
            xinfo("XErrorSync_obj::set_block_lower_height set block valid sign block %s", block->dump().c_str());
        }
        return new_block_vec;
    }
    return {};
}

std::vector<top::data::xblock_ptr_t> XErrorSync_obj::set_block_super_height(std::vector<top::data::xblock_ptr_t> const & vector_blocks) {
    if (vector_blocks.size() > 0) {
        auto block_str_vec = convert_blocks_to_stream(enum_sync_data_all, vector_blocks);
        auto new_block_vec = convert_stream_to_blocks(enum_sync_data_all, block_str_vec);

        for (auto & block : new_block_vec) {
            block->get_header()->set_height(block->get_height() + 200);
            xinfo("XErrorSync_obj::set_block_super_height set block valid sign block %s", block->dump().c_str());
        }
        return new_block_vec;
    }
    return {};
}

std::vector<top::data::xblock_ptr_t> XErrorSync_obj::set_block_error_nonce(std::vector<top::data::xblock_ptr_t> const & vector_blocks) {
    if (vector_blocks.size() > 0) {
        auto block_str_vec = convert_blocks_to_stream(enum_sync_data_all, vector_blocks);
        auto new_block_vec = convert_stream_to_blocks(enum_sync_data_all, block_str_vec);

        for (auto & block : new_block_vec) {          
            block->get_cert()->set_error_nonce(block->get_cert()->get_nonce() + 10);
            xinfo("XErrorSync_obj::set_block_error_nonce set block error nonce  block %s", block->dump().c_str());
        }
        return new_block_vec;
    }
    return {};
}

NS_END2