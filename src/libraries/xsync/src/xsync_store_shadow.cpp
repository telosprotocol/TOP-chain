// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xsync/xsync_store_shadow.h"
#include <algorithm>
#include "xsync/xdownloader.h"
#include "xbase/xbase.h"

#include <cinttypes>

NS_BEG2(top, sync)

xcommon_span_t::xcommon_span_t(){    
    m_span_index = 0;
    m_lowest_unused_postion = 0;
    for (int i = 0; i < SEGMENT_SIZE; i++) {
        m_bitmap.push_back(0);
    }
}

xcommon_span_t::xcommon_span_t(uint64_t height){
    uint64_t span_index = index(height);
    m_span_index = span_index;
    m_lowest_unused_postion = span_index;
    for (int i = 0; i < SEGMENT_SIZE; i++) {
        m_bitmap.push_back(0);
    }
}

bool xcommon_span_t::set(uint64_t height, bool & modified) {
    if (!own_to_span(height)){
        return false;
    }

    uint64_t offset = height - m_span_index;

    if ((m_bitmap[SEGMENT_INDEX(offset)] & ((uint64_t)1 << OFFSET_IN_SEGMENT(offset))) != 0) {
        return true;
    }

    m_bitmap[SEGMENT_INDEX(offset)] |= ((uint64_t)1 << OFFSET_IN_SEGMENT(offset));
    modified = true;
    update_unused_position();
    return true;
}

void xcommon_span_t::connect() {
    update_unused_position();
}

bool xcommon_span_t::full() {
    for (int i = 0; i < SEGMENT_SIZE; i++) {
        if (~m_bitmap[i] != 0) {
            return false;
        }
    }
    return true;
}

bool xcommon_span_t::owned(uint64_t height) {
    return own_to_span(height);
}

uint64_t xcommon_span_t::lowest_unused_height() {
    return m_lowest_unused_postion;
}

uint64_t xcommon_span_t::index() {
    return m_span_index;
}

uint64_t xcommon_span_t::index(uint64_t height) {
    return height & (~BITMAP_MASK);
}

int32_t xcommon_span_t::do_write(base::xstream_t & stream){
    KEEP_SIZE();
    SERIALIZE_FIELD_BT(m_bitmap);
    return CALC_LEN();
}

int32_t xcommon_span_t::do_read(base::xstream_t & stream){
    KEEP_SIZE();
    std::vector<uint64_t> bitmap;
    DESERIALIZE_FIELD_BT(bitmap);
    for (uint32_t i = 0; i < SEGMENT_SIZE; i++){
        m_bitmap[i] = bitmap[i];
    }
    return CALC_LEN();
}

void xcommon_span_t::update_unused_position() {
    uint64_t offset = m_lowest_unused_postion - m_span_index;
    if (offset >= m_quota_of_bitmap) {
        return;
    }
    uint64_t k = OFFSET_IN_SEGMENT(offset);
    for (int i = SEGMENT_INDEX(offset); i < SEGMENT_SIZE; i++,k=0) {
        for (int j = k; j < 64; j++) {
            if ((m_bitmap[i] & ((uint64_t)1 << j)) != 0) {
                m_lowest_unused_postion++;
            } else {
                return;
            }
        }
    }
}

bool xcommon_span_t::own_to_span(uint64_t height){
    if ((height & (~(BITMAP_MASK))) != m_span_index) {
        return false;
    }
    return true;
}

bool xcommon_span_t::height_exist(uint64_t height)
{
    if (own_to_span(height)) {
        uint64_t offset = height - m_span_index;
        if ((m_bitmap[SEGMENT_INDEX(offset)] & ((uint64_t)1 << OFFSET_IN_SEGMENT(offset))) != 0) {
            return true;
        }
    }
    return false;
}

std::pair<uint64_t, uint64_t> xcommon_span_t::get_continuous_unused_interval(const std::pair<uint64_t, uint64_t> height_interval){
    uint64_t offset = height_interval.first - m_span_index;
    uint64_t unused_start_height = 0;
    uint64_t unused_end_height = 0;
    uint64_t start_height = height_interval.first;

    uint64_t k = OFFSET_IN_SEGMENT(offset);
    for (int i = SEGMENT_INDEX(offset); i < SEGMENT_SIZE; i++, k=0) {
        for (int j = k; j < 64; j++) {
            if ((m_bitmap[i] & ((uint64_t)1 << j)) == 0) {
                if (unused_start_height == 0) {
                    unused_start_height = start_height;
                }
                unused_end_height = start_height;
            } else {
                if (unused_start_height != 0) {
                    return std::pair<uint64_t, uint64_t>(unused_start_height, unused_end_height);
                }
            }

            start_height++;

            if (start_height > height_interval.second) {
                return std::pair<uint64_t, uint64_t>(unused_start_height, unused_end_height);
            }
        }
    }

    return std::pair<uint64_t, uint64_t>(unused_start_height, unused_end_height);
}

void xsync_span_dao::write_span_to_db(xsync_store_face_t *store, const std::string account, xcommon_span_t* span) {
    base::xstream_t stream(base::xcontext_t::instance());
    span->serialize_to(stream);
    store->set_block_span(account, span->index(), std::string((const char *)stream.data(), stream.size()));
}

const void xsync_span_dao::read_span_from_db(xsync_store_face_t *store,const std::string account, xcommon_span_t* span) {
    auto output = store->get_block_span(account, span->index());
    if (!output.empty()) {
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)output.c_str(), output.size());
        span->serialize_from(stream);
        span->connect();
    }
}

void xsync_span_dao::write_span_height_to_db(xsync_store_face_t *store, const std::string account, uint64_t height){
    base::xstream_t stream(base::xcontext_t::instance());
    stream << height;
    store->set_genesis_height(account, std::string((const char *)stream.data(), stream.size()));
}

const void xsync_span_dao::read_span_height_from_db(xsync_store_face_t *store, const std::string account, uint64_t &height){
    auto output = store->get_genesis_height(account);
    if (!output.empty()) {
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)output.c_str(), output.size());
        stream >> height;
        return;
    }

    height = 0;
    return;
}

void xsync_chain_spans_t::save(){
    if (m_connect_to_genesis_span_modified) {
        xsync_span_dao::write_span_to_db(m_store, m_account, m_connect_to_genesis_span.get());
        m_connect_to_genesis_span_modified = false;
    }

    if (m_connect_to_genesis_height_modified) {
        xsync_span_dao::write_span_height_to_db(m_store, m_account, m_connect_to_genesis_height);
        m_connect_to_genesis_height_modified = false;
        xdbg("xsync_store_shadow_t::save, account:%s, height=%llu",
            m_account.c_str(), m_connect_to_genesis_height);
    }

    for (auto it : m_highest_spans) {
        if (!m_highest_span_modified[it.first]) {
            continue;
        }

        xsync_span_dao::write_span_to_db(m_store, m_account, it.second.get());
        m_highest_span_modified[it.first] = false;
    }
}

uint64_t xsync_chain_spans_t::genesis_connect_height() {
    return m_connect_to_genesis_height;
}

uint64_t xsync_chain_spans_t::genesis_height_refresh_time_ms() {
    return m_genesis_height_refresh_time;
}

std::pair<uint64_t, uint64_t> xsync_chain_spans_t::get_continuous_unused_interval(const std::pair<uint64_t, uint64_t> height_interval) {
    uint64_t start_height = height_interval.first;
    uint64_t end_height = height_interval.first;
    uint64_t unused_start_height = 0;
    uint64_t unused_end_height = 0;

    do {
        uint64_t span_index = xcommon_span_t::index(start_height);
        xcommon_span_t *span;
        if (m_connect_to_genesis_span->owned(span_index)) {
            span = m_connect_to_genesis_span.get();
        } else {
            auto it = m_highest_spans.find(span_index);
            if (it == m_highest_spans.end()) {
                xobject_ptr_t<xcommon_span_t> load_span = make_object_ptr<xcommon_span_t>(span_index);
                xsync_span_dao::read_span_from_db(m_store, m_account, load_span.get());
                m_highest_spans[span_index] = load_span;
            }

            span = m_highest_spans[span_index].get();
        }

        if (height_interval.second >= (span_index + xcommon_span_t::m_quota_of_bitmap - 1)) {
            end_height = span_index + xcommon_span_t::m_quota_of_bitmap - 1;
        } else {
            end_height = height_interval.second;
        }
        auto continuous_unused_interval = span->get_continuous_unused_interval(std::make_pair(start_height, end_height));
        start_height = end_height + 1;
        if (continuous_unused_interval.second == 0) {
            if (unused_start_height == 0) {
                continue;
            }

            break;
        }

        if (unused_start_height == 0) {
            unused_start_height = continuous_unused_interval.first;
        }

        if (unused_end_height == 0) {
            unused_end_height = continuous_unused_interval.second;
        } else {
            if (continuous_unused_interval.first != unused_end_height + 1) {
                break;
            }

            unused_end_height = continuous_unused_interval.second;
        }
    } while (end_height < height_interval.second);

    clear_highest_spans();

    return std::make_pair(unused_start_height, unused_end_height);
}

void xsync_chain_spans_t::set(uint64_t height){
    if ((height <= m_connect_to_genesis_height) && (m_connect_to_genesis_height != 0)) {
        return;
    }    
    
    uint64_t span_index = xcommon_span_t::index(height);
    if (m_connect_to_genesis_span->owned(span_index)) {
        set_genesis_span(height);
        return;
    }

    set_highest_spans(height);
}

void xsync_chain_spans_t::initialize() {
    if (m_initialed) {
        return;
    }

    base::xvaccount_t _vaddr(m_account);

    xdbg("xsync_store_shadow_t::initialize before, account:%s, height=%llu",
        m_account.c_str(), m_connect_to_genesis_height);
    xsync_span_dao::read_span_height_from_db(m_store, m_account, m_connect_to_genesis_height);
    uint64_t height = m_store->get_latest_genesis_connected_block_height(m_account);
    uint64_t height_old = height;
    height = std::max(height, m_connect_to_genesis_height);
    auto const last_deleted_height = m_store->get_latest_deleted_block_height(m_account);
    if (height > last_deleted_height) {
        while (true) {
            base::xauto_ptr<base::xvbindex_t> vbindex = m_store->recover_and_load_commit_index(_vaddr, height + 1);
            if (vbindex == nullptr) {
                break;
            }
            height = vbindex->get_height();
            xinfo("xsync_store_shadow_t::initialize recover height.account:%s,blk height %" PRIu64, m_account.c_str(), height);
        }
    } else {
        xinfo("xsync_store_shadow_t::initialize height %" PRIu64 " last deleted blk height %" PRIu64, height, last_deleted_height);
    }

    xinfo("xsync_store_shadow_t::initialize after, account:%s, height=%llu, updated height=%llu, old = %llu",
        m_account.c_str(), m_connect_to_genesis_height, height, height_old);

    if (height > m_connect_to_genesis_height) {
        xobject_ptr_t<xcommon_span_t> span = make_object_ptr<xcommon_span_t>(height + 1);
        m_connect_to_genesis_span = span;
        bool modified = false;
        for (uint64_t i = xcommon_span_t::index(height + 1); i < (height + 1); i++) {
            m_connect_to_genesis_span->set(i, modified);
        }
        m_connect_to_genesis_height = height;
        m_connect_to_genesis_height_modified = true;
        m_connect_to_genesis_span_modified = true;
    } else {
        xobject_ptr_t<xcommon_span_t> span = make_object_ptr<xcommon_span_t>(m_connect_to_genesis_height);
        m_connect_to_genesis_span = span;
        xsync_span_dao::read_span_from_db(m_store, m_account, m_connect_to_genesis_span.get());
        if (m_connect_to_genesis_span->lowest_unused_height() == 0){
            bool modified = false;
            m_connect_to_genesis_span->set(0, modified);
        }
    }

    uint64_t unused_height = m_connect_to_genesis_span->lowest_unused_height();
    while (true) {
        if (unused_height != m_connect_to_genesis_span->lowest_unused_height()) {
            m_connect_to_genesis_height = m_connect_to_genesis_span->lowest_unused_height() - 1;
            m_connect_to_genesis_height_modified = true;
            unused_height = m_connect_to_genesis_span->lowest_unused_height();
        }

        if (!m_connect_to_genesis_span->full()) {
            break;
        }

        uint64_t span_index = xcommon_span_t::index(m_connect_to_genesis_height + 1);
        auto it = m_highest_spans.find(span_index);
        if (it != m_highest_spans.end()) {
            move_to_genisis(span_index);
        } else {
            xobject_ptr_t<xcommon_span_t> span = make_object_ptr<xcommon_span_t>(m_connect_to_genesis_height + 1);
            xsync_span_dao::read_span_from_db(m_store, m_account, span.get());
            m_connect_to_genesis_span = span;            
        }        
    }
    m_initialed = true;
    m_genesis_height_refresh_time = base::xtime_utl::gmttime_ms();
}

void xsync_chain_spans_t::set_genesis_span(uint64_t height) {
    bool modified = false;
    uint64_t unused_height = m_connect_to_genesis_span->lowest_unused_height();
    m_connect_to_genesis_span->set(height, modified);
    if (!modified) {
        return;
    }
    m_connect_to_genesis_span_modified = true;

    while (true) {
        if (unused_height != m_connect_to_genesis_span->lowest_unused_height()) {
            m_connect_to_genesis_height = m_connect_to_genesis_span->lowest_unused_height() - 1;
            m_connect_to_genesis_height_modified = true;
            unused_height = m_connect_to_genesis_span->lowest_unused_height();
            m_genesis_height_refresh_time = base::xtime_utl::gmttime_ms();
        }

        if (!m_connect_to_genesis_span->full()) {
            break;
        }

        if (m_connect_to_genesis_span_modified) {
            xsync_span_dao::write_span_to_db(m_store, m_account, m_connect_to_genesis_span.get());
            m_connect_to_genesis_span_modified = false;
        }

        if (m_connect_to_genesis_height_modified) {
            xsync_span_dao::write_span_height_to_db(m_store, m_account, m_connect_to_genesis_height);
            m_connect_to_genesis_height_modified = false;
        }

        uint64_t span_index = xcommon_span_t::index(m_connect_to_genesis_height + 1);
        auto it = m_highest_spans.find(span_index);
        if (it != m_highest_spans.end()) {
            move_to_genisis(span_index);
        } else {
            xobject_ptr_t<xcommon_span_t> span = make_object_ptr<xcommon_span_t>(m_connect_to_genesis_height + 1);
            xsync_span_dao::read_span_from_db(m_store, m_account, span.get());
            m_connect_to_genesis_span = span;                
        }
    }
}

void xsync_chain_spans_t::set_highest_spans(uint64_t height) {
    uint64_t span_index = xcommon_span_t::index(height);
    auto it = m_highest_spans.find(span_index);
    if (it != m_highest_spans.end()) {
        xcommon_span_t *span = it->second.get();
        bool modified = false;
        span->set(height, modified);

        if (!modified) {
            return;
        }

        m_highest_span_modified[span_index] = true;
        return;
    }
    
    xobject_ptr_t<xcommon_span_t> span = make_object_ptr<xcommon_span_t>(span_index);
    xsync_span_dao::read_span_from_db(m_store, m_account, span.get());

    bool modified = false;
    span->set(height, modified);
    
    m_highest_spans[span_index] = span;
    m_highest_span_modified[span_index] = modified;

    clear_highest_spans();
}

void xsync_chain_spans_t::clear_highest_spans() {
    for (auto it = m_highest_spans.begin(); (it != m_highest_spans.end()) && (m_span_nums < m_highest_spans.size());) {
        if (m_highest_span_modified[it->first]) {
            xsync_span_dao::write_span_to_db(m_store, m_account, it->second.get());        
        }

        m_highest_span_modified.erase(it->first);
        it = m_highest_spans.erase(it);
    }
}

void xsync_chain_spans_t::move_to_genisis(uint64_t span_index) {
    m_connect_to_genesis_span = m_highest_spans[span_index];
    m_connect_to_genesis_span_modified = m_highest_span_modified[span_index];
    m_highest_span_modified.erase(span_index);
    m_highest_spans.erase(span_index);
    m_connect_to_genesis_span->connect();
}

void xsync_store_shadow_t::set_store(xsync_store_face_t *sync_store) {
    m_sync_store = sync_store;
    m_listener_id = m_sync_store->add_listener(mbus::xevent_major_type_store, 
    std::bind(&xsync_store_shadow_t::xsync_event_cb, this, std::placeholders::_1));
}

void xsync_store_shadow_t::set_downloader(xdownloader_t *downloader) {
    m_downloader = downloader;
}

xsync_store_shadow_t::~xsync_store_shadow_t() {
    std::unordered_map<std::string, std::shared_ptr<xsync_chain_spans_t>> chain_spans;
    {
        std::unique_lock<std::mutex> lck(m_lock);
        chain_spans = m_chain_spans;
    }
    xinfo("xsync_store_shadow_t save span .");
    for (auto span:chain_spans) {
        span.second->save();
    }    
    m_sync_store->remove_listener(mbus::xevent_major_type_store, m_listener_id);
}
void xsync_store_shadow_t::save() {
    xinfo("xsync_store_shadow_t save span.");
    std::unordered_map<std::string, std::shared_ptr<xsync_chain_spans_t>> chain_spans;
    {
        std::unique_lock<std::mutex> lck(m_lock);
        chain_spans = m_chain_spans;
    }
    for (auto span:chain_spans) {
        span.second->save();
    }        
}
void xsync_store_shadow_t::xsync_event_cb(mbus::xevent_ptr_t e) {
    if (e->minor_type != mbus::xevent_store_t::type_block_committed) {
        xdbg("xsync_event_cb type error: %d", e->minor_type);
        return;
    }

    mbus::xevent_store_block_committed_ptr_t block_event = 
        dynamic_xobject_ptr_cast<mbus::xevent_store_block_committed_t>(e);
    if (block_event->blk_level != base::enum_xvblock_level_table && block_event->blk_level != base::enum_xvblock_level_root) {
        xdbg("xsync_event_cb check fail: %s", block_event->owner.c_str());
        return;
    }

    xinfo("xsync_store_shadow_t::xsync_event_cb, account:%s, height=%llu",
            block_event->owner.c_str(), block_event->blk_height);

    m_downloader->push_event(e);
}

void xsync_store_shadow_t::on_chain_event(const std::string &account, uint64_t height) {
    std::shared_ptr<xsync_chain_spans_t> chain_spans{nullptr};
    {
        std::unique_lock<std::mutex> lck(m_lock);
        auto it = m_chain_spans.find(account);
        if (it != m_chain_spans.end()) {
            chain_spans = it->second;
        }
    }

    if (chain_spans != nullptr) {
        chain_spans->set(height);
        xinfo("xsync_store_shadow_t::on_chain_event, account:%s, height=%llu",
        account.c_str(), height);
        return;
    }

    std::shared_ptr<xsync_chain_spans_t> chain_spans_new = std::make_shared<xsync_chain_spans_t>(2, m_sync_store, account);
    chain_spans_new->initialize();
    {
        std::unique_lock<std::mutex> lck(m_lock);
        auto it = m_chain_spans.find(account);
        if (it == m_chain_spans.end()) {
            m_chain_spans[account] = chain_spans_new;
            chain_spans = chain_spans_new;
        } else {
            chain_spans = it->second;
        }
    }

    chain_spans->set(height);
    return;
}

uint64_t xsync_store_shadow_t::genesis_connect_height(const std::string& account) {
    std::shared_ptr<xsync_chain_spans_t> chain_spans{nullptr};
    {
        std::unique_lock<std::mutex> lck(m_lock);
        auto it = m_chain_spans.find(account);
        if (it != m_chain_spans.end()) {
            chain_spans = it->second;
        }
    }

    if (chain_spans != nullptr) {
        return chain_spans->genesis_connect_height();
    }

    std::shared_ptr<xsync_chain_spans_t> chain_spans_new = std::make_shared<xsync_chain_spans_t>(2, m_sync_store, account);
    chain_spans_new->initialize();
    {
        std::unique_lock<std::mutex> lck(m_lock);
        auto it = m_chain_spans.find(account);
        if (it == m_chain_spans.end()) {
            m_chain_spans[account] = chain_spans_new;
            chain_spans = chain_spans_new;
        } else {
            chain_spans = it->second;
        }
    }

    return chain_spans->genesis_connect_height();
}

std::pair<uint64_t, uint64_t> xsync_store_shadow_t::get_continuous_unused_interval(
    const std::string& account, 
    const std::pair<uint64_t, uint64_t> height_interval){
    std::shared_ptr<xsync_chain_spans_t> chain_spans{nullptr};
    {
        std::unique_lock<std::mutex> lck(m_lock);
        auto it = m_chain_spans.find(account);
        if (it != m_chain_spans.end()) {
            chain_spans = it->second;
        }
    }

    if (chain_spans != nullptr) {
        return chain_spans->get_continuous_unused_interval(height_interval);
    }

    std::shared_ptr<xsync_chain_spans_t> chain_spans_new = std::make_shared<xsync_chain_spans_t>(2, m_sync_store, account);
    chain_spans_new->initialize();
    {
        std::unique_lock<std::mutex> lck(m_lock);
        auto it = m_chain_spans.find(account);
        if (it == m_chain_spans.end()) {
            m_chain_spans[account] = chain_spans_new;
            chain_spans = chain_spans_new;
        } else {
            chain_spans = it->second;
        }
    }

    return chain_spans->get_continuous_unused_interval(height_interval);
}

uint64_t xsync_store_shadow_t::genesis_height_refresh_time_ms(const std::string& account) {
    std::shared_ptr<xsync_chain_spans_t> chain_spans{nullptr};
    {
        std::unique_lock<std::mutex> lck(m_lock);
        auto it = m_chain_spans.find(account);
        if (it != m_chain_spans.end()) {
            chain_spans = it->second;
        }
    }

    if (chain_spans != nullptr) {
        return chain_spans->genesis_height_refresh_time_ms();
    }

    std::shared_ptr<xsync_chain_spans_t> chain_spans_new = std::make_shared<xsync_chain_spans_t>(2, m_sync_store, account);
    chain_spans_new->initialize();
    {
        std::unique_lock<std::mutex> lck(m_lock);
        auto it = m_chain_spans.find(account);
        if (it == m_chain_spans.end()) {
            m_chain_spans[account] = chain_spans_new;
            chain_spans = chain_spans_new;
        } else {
            chain_spans = it->second;
        }
    }

    return chain_spans->genesis_height_refresh_time_ms();
}

void xsync_store_shadow_t::on_timer(uint32_t idx) {
    std::unordered_map<std::string, std::shared_ptr<xsync_chain_spans_t>> chain_spans;
    {
        std::unique_lock<std::mutex> lck(m_lock);
        chain_spans = m_chain_spans;
    }

    for (auto span:chain_spans) {
        if (m_downloader->get_idx_by_address(span.first) == idx){
            span.second->save();
        }
    }
}

NS_END2
