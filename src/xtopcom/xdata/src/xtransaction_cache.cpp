#include "xdata/xtransaction_cache.h"
#include "xpbase/base/top_utils.h"

namespace top { namespace data {

int xtransaction_cache_t::tx_add(const std::string& tx_hash, const xtransaction_ptr_t tx) {
    std::lock_guard<std::mutex> lock(m_mutex);
    xtransaction_cache_data_t cache_data;
    cache_data.tran = tx;
    m_trans[tx_hash] = cache_data;
    xdbg("add cache: %s, size:%d", top::HexEncode(tx_hash).c_str(), sizeof(tx_hash) + sizeof(cache_data));
    XMETRICS_COUNTER_INCREMENT("xtransaction_cache_count", 1);
    return 0;
}
int xtransaction_cache_t::tx_find(const std::string& tx_hash) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_trans.find(tx_hash) == m_trans.end())
        return 0;
    return 1;
}
int xtransaction_cache_t::tx_get_json(const std::string& tx_hash, xJson::Value & jv) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_trans.find(tx_hash) == m_trans.end())
        return 0;
    jv = m_trans[tx_hash].jv;
    return 1;
}
int xtransaction_cache_t::tx_get(const std::string& tx_hash, xtransaction_cache_data_t& cache_data){
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_trans.find(tx_hash) == m_trans.end())
        return 0;
    cache_data = m_trans[tx_hash];
    return 1;
}
int xtransaction_cache_t::tx_set_json(const std::string& tx_hash, const xJson::Value & jv) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_trans.find(tx_hash) == m_trans.end())
        return 1;
    m_trans[tx_hash].jv = jv;
    xdbg("add cache json: %s", top::HexEncode(tx_hash).c_str());
    return 0;
}
int xtransaction_cache_t::tx_set_recv_txinfo(const std::string& tx_hash, const data::xlightunit_action_ptr_t tx_info){
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_trans.find(tx_hash) == m_trans.end())
        return 1;
    m_trans[tx_hash].recv_txinfo = tx_info;
    xdbg("add cache recv_txinfo: %s", top::HexEncode(tx_hash).c_str());
    return 0;
}
int xtransaction_cache_t::tx_erase(const std::string& tx_hash) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::map<std::string, xtransaction_cache_data_t>::iterator it = m_trans.find(tx_hash);
    if (it == m_trans.end())
        return 1;
    m_trans.erase(it);
    xdbg("erase cache: %s", top::HexEncode(tx_hash).c_str());
    XMETRICS_COUNTER_DECREMENT("xtransaction_cache_count", 1);
    return 0;
}
int xtransaction_cache_t::tx_clean() {
    struct timeval val;
    base::xtime_utl::gettimeofday(&val);
    std::lock_guard<std::mutex> lock(m_mutex);
    for (std::map<std::string, xtransaction_cache_data_t>::iterator it= m_trans.begin(); it != m_trans.end(); ) {
        if (it->second.tran->get_fire_timestamp() + it->second.tran->get_expire_duration() < (uint64_t)val.tv_sec) {
            xdbg("erase tx: %lld,%d,%lld", it->second.tran->get_fire_timestamp() , it->second.tran->get_expire_duration() , (uint64_t)val.tv_sec);
            m_trans.erase(it++);
            XMETRICS_COUNTER_DECREMENT("xtransaction_cache_count", 1);
            continue;
        }
        xdbg("not erase tx: %lld,%d,%lld", it->second.tran->get_fire_timestamp() , it->second.tran->get_expire_duration() , (uint64_t)val.tv_sec);
        ++it;
    }
    return 0;
}
int xtransaction_cache_t::tx_clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_trans.clear();
    xinfo("cleat tx cache all.");
    return 0;
}
}
}
