// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <vector>
#include <map>
#include <unordered_map>
#include <mutex>
#include "xbase/xns_macro.h"
#include "xsync/xsync_store.h"
#include "xmbus/xevent_store.h"
#include "xbasic/xserialize_face.h"

NS_BEG2(top, sync)
class xcommon_span_t : public top::basic::xserialize_face_t{
#define BITMAP_MASK (m_quota_of_bitmap - 1)
#define OFFSET_IN_SEGMENT(offset) ((offset) & 0x3f)
#define SEGMENT_INDEX(offset) (((offset) & (m_quota_of_bitmap - 1)) >> 6)
#define SEGMENT_SIZE (m_quota_of_bitmap >> 6)
public:
    xcommon_span_t();
    xcommon_span_t(uint64_t height);
    bool set(uint64_t height, bool & modified);
    void connect();
    bool full();
    bool owned(uint64_t height);
    uint64_t lowest_unused_height();
    uint64_t index();
    std::pair<uint64_t, uint64_t> get_continuous_unused_interval(const std::pair<uint64_t, uint64_t> height_interval);
    bool height_exist(uint64_t height);
public:
    static uint64_t index(uint64_t height);
    static const int32_t m_quota_of_bitmap = 1024;
protected:
    int32_t do_write(base::xstream_t & stream) override;
    int32_t do_read(base::xstream_t & stream) override;
private:
    void update_unused_position();
    bool own_to_span(uint64_t height);
private:
    uint64_t m_span_index{0};
    uint64_t m_lowest_unused_postion{0};
    std::vector<uint64_t> m_bitmap;
};

class xsync_span_dao {
public:
    static void write_span_to_db(xsync_store_face_t *store, const std::string account, xcommon_span_t* span);
    static const void read_span_from_db(xsync_store_face_t *store,const std::string account, xcommon_span_t* span);
    static void write_span_height_to_db(xsync_store_face_t *store, const std::string account, uint64_t height);
    static const void read_span_height_from_db(xsync_store_face_t *store, const std::string account, uint64_t &height);
};

class xsync_chain_spans_t {
public:
    xsync_chain_spans_t(){
    };

    xsync_chain_spans_t(uint32_t span_nums, xsync_store_face_t *sync_store, std::string account): 
        m_span_nums(span_nums), m_store(sync_store), m_account(account){
        m_connect_to_genesis_span = make_object_ptr<xcommon_span_t>();
    };
    
    void save();
    void set(uint64_t height);
    void initialize();
    uint64_t genesis_connect_height();
    std::pair<uint64_t, uint64_t> get_continuous_unused_interval(const std::pair<uint64_t, uint64_t> height_interval);
    uint64_t genesis_height_refresh_time_ms();
private:
    void set_genesis_span(uint64_t height);
    void set_highest_spans(uint64_t height);
    void clear_highest_spans();
    void move_to_genisis(uint64_t span_index);

private:
    bool m_initialed{false};
    uint32_t m_span_nums = 0;
    xsync_store_face_t *m_store;
    std::string m_account;    
    
    //
    std::map<uint64_t, xobject_ptr_t<xcommon_span_t>> m_highest_spans;
    std::map<uint64_t, bool> m_highest_span_modified;
    
    //
    uint64_t m_genesis_height_refresh_time = 0;
    uint64_t m_connect_to_genesis_height = 0;
    bool m_connect_to_genesis_height_modified = 0;
    xobject_ptr_t<xcommon_span_t> m_connect_to_genesis_span;
    bool m_connect_to_genesis_span_modified{false};
};

class xdownloader_t;
class xsync_store_shadow_t{
public:
    xsync_store_shadow_t() {};
    ~xsync_store_shadow_t();
    void xsync_event_cb(mbus::xevent_ptr_t e);
    void on_chain_event(const std::string &account, uint64_t height);
    uint64_t genesis_connect_height(const std::string& account);
    std::pair<uint64_t, uint64_t> get_continuous_unused_interval(const std::string& account, const std::pair<uint64_t, uint64_t> height_interval);
    void on_timer(uint32_t idx);
    void set_store(xsync_store_face_t *sync_store);
    void set_downloader(xdownloader_t *downloader);
    uint64_t genesis_height_refresh_time_ms(const std::string& account);
    void save();
private:
    xdownloader_t *m_downloader;
    xsync_store_face_t *m_sync_store;
    std::unordered_map<std::string, std::shared_ptr<xsync_chain_spans_t>> m_chain_spans;
    uint32_t m_listener_id;
    std::mutex m_lock;
};
NS_END2