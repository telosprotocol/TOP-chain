#include "xaction_param.h"
#include "api_method_imp.h"


namespace xChainSDK {


    xaction_param::xaction_param(api_method_imp* method)
        : m_method_ptr(method)
    {

    }


    xaction_asset_param::xaction_asset_param(api_method_imp* method,
        const std::string& token_name, uint64_t amount)
        : xaction_param(method)
        , m_token_name(token_name)
        , m_amount(amount)
    {

    }

    std::string xaction_asset_param::create()
    {
        top::base::xstream_t stream(top::base::xcontext_t::instance());
        std::string param = m_method_ptr->stream_params(stream, m_token_name, m_amount);
        return param;
    }

    xaction_pledge_token_vote_param::xaction_pledge_token_vote_param(api_method_imp* method,
        uint64_t amount, uint16_t lock_duration)
        : xaction_param(method)
        , m_vote_num(amount)
        , m_lock_duration(lock_duration)
    {

    }

    std::string xaction_pledge_token_vote_param::create()
    {
        top::base::xstream_t stream(top::base::xcontext_t::instance());
        std::string param = m_method_ptr->stream_params(stream, m_vote_num, m_lock_duration);
        return param;
    }

    xaction_redeem_token_vote_param::xaction_redeem_token_vote_param(api_method_imp* method,
        uint64_t amount)
        : xaction_param(method)
        , m_vote_num(amount)
    {
    }

    std::string xaction_redeem_token_vote_param::create()
    {
        top::base::xstream_t stream(top::base::xcontext_t::instance());
        std::string param = m_method_ptr->stream_params(stream, m_vote_num);
        return param;
    }

}
