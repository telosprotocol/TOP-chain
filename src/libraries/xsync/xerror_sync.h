#pragma once
#include "xsync_time_rejecter.h"
#include "xsync_message.h"
#include "xdata/xblock.h"

NS_BEG2(top, sync)

enum  xenum_error_sync{
    kErrorNoError = 0,
    kErrorSign = 1,
    kErrorLowerRequestHeight = 2,
    kErrorSuperRequestHiehgt = 3,
    kErrorNotResponse = 4,
    kErrorQcertNonce = 5,
};

enum  xenum_error_height  {
    kErrorHeightNormal = 0,
    kErrorHeightSuper = 1,
};

class XErrorSync_obj
{
public:
    static XErrorSync_obj & instance();
    XErrorSync_obj():m_error_type(0),m_error_height_type(0) {
    };
    ~XErrorSync_obj();

    std::vector<top::data::xblock_ptr_t> set_error_block_vector(std::vector<top::data::xblock_ptr_t> const & vector_blocks);
    void set_error_height(xchain_state_info_t & info);

private:
    std::vector<top::data::xblock_ptr_t> set_block_valid_sign(std::vector<top::data::xblock_ptr_t> const & vector_blocks);
    std::vector<top::data::xblock_ptr_t> set_block_lower_height(std::vector<top::data::xblock_ptr_t> const & vector_blocks);
    std::vector<top::data::xblock_ptr_t> set_block_super_height(std::vector<top::data::xblock_ptr_t> const & vector_blocks);
    std::vector<top::data::xblock_ptr_t> set_block_error_nonce(std::vector<top::data::xblock_ptr_t> const & vector_blocks); 
private:
    int m_error_type;
    int m_error_height_type;
    xsync_time_rejecter_t m_time_error_rejecter{1000*60};       // 3 minutes change error_type
    xsync_time_rejecter_t m_time_height_rejecter{1000*60};       // 3 minutes change error_type
};

NS_END2
