// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <unordered_map>
#include <memory>

#include "xsync/xaccount_face.h"

NS_BEG2(top, sync)

#define WALK_MAP_ALWAYS_STAY    0x01
#define WALK_MAP_ACTIVE         0x02
#define WALK_MAP_DONE           0x04

class xaccount_cache_t {
    using walk_func = std::function<void(const std::string &, const xaccount_face_ptr_t &)>;
    using walk_remove_func = std::function<bool(const std::string &, const xaccount_face_ptr_t &)>;
public:
    xaccount_cache_t(uint32_t max_size);

    virtual ~xaccount_cache_t();

    void add(std::string const& key, xaccount_face_ptr_t const& account, bool always_stay);
    void remove(std::string const& key, uint32_t which);
    void walk(walk_func _func, uint32_t which);
    void walk_remove(walk_remove_func _func, uint32_t which);
    void active(std::string const& key);
    void done(std::string const& key);
    xaccount_face_ptr_t get(const std::string &key, uint32_t which);

protected:
    void clear();

protected:
    std::mutex m_lock;
    uint32_t m_max_size{};
    std::unordered_map<std::string, xaccount_face_ptr_t> m_always_stay_map;
    std::unordered_map<std::string, xaccount_face_ptr_t> m_active_map;
    std::unordered_map<std::string, xaccount_face_ptr_t> m_done_map;
};

NS_END2