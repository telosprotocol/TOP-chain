// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_common/trie/xsecure_trie.h"

#include "xbasic/xstring.h"
#include "xutility/xhash.h"

NS_BEG3(top, evm_common, trie)

std::shared_ptr<xtop_secure_trie> xtop_secure_trie::build_from(xh256_t const & root, xtrie_db_ptr_t db, std::error_code & ec) {
    assert(!ec);

    if (db == nullptr) {
        xerror("build secure trie from null db");
    }
    auto trie = xtrie_t::build_from(root, std::move(db), ec);
    if (ec) {
        xwarn("secure trie new failed: %s", ec.message().c_str());
        return nullptr;
    }
    return std::shared_ptr<xtop_secure_trie>(new xtop_secure_trie{std::move(trie)});
}

xbytes_t xtop_secure_trie::get(xbytes_t const & key) const {
    std::error_code ec;
    auto result = try_get(key, ec);
    if (ec) {
        xerror("secure trie error: %s", ec.message().c_str());
    }
    return result;
}

xbytes_t xtop_secure_trie::try_get(xbytes_t const & key, std::error_code & ec) const {
    assert(m_trie != nullptr);
    return m_trie->try_get(hash_key(key), ec);
}


std::pair<xbytes_t, std::size_t> xtop_secure_trie::try_get_node(xbytes_t const & path, std::error_code & ec){
    assert(m_trie != nullptr);
    return m_trie->try_get_node(path, ec);
}

void xtop_secure_trie::update(xbytes_t const & key, xbytes_t const & value) {
    std::error_code ec;
    try_update(key, value, ec);
    if (ec) {
        xerror("secure trie error: %s", ec.message().c_str());
    }
    return;
}

void xtop_secure_trie::try_update(xbytes_t const & key, xbytes_t const & value, std::error_code & ec) {
    auto const hk = hash_key(key);
    assert(m_trie != nullptr);

    m_trie->try_update(hk, value, ec);
    if (ec) {
        return;
    }

    (*get_sec_key_cache())[top::to_string(hk)] = key;
}

void xtop_secure_trie::Delete(xbytes_t const & key) {
    std::error_code ec;
    try_delete(key, ec);
    if (ec) {
        xerror("secure trie error: %s", ec.message().c_str());
    }
    return;
}

void xtop_secure_trie::try_delete(xbytes_t const & key, std::error_code & ec) {
    auto hk = hash_key(key);
    get_sec_key_cache()->erase(top::to_string(hk));
    assert(m_trie != nullptr);
    m_trie->try_delete(hk, ec);
}

xbytes_t xtop_secure_trie::get_key(xbytes_t const & shaKey) {
    auto sc = get_sec_key_cache();
    if (sc->find(top::to_string(shaKey)) != sc->end()) {
        return sc->at(top::to_string(shaKey));
    }
    xdbg("xtop_secure_trie::GetKey find key from trie_db preimage");
    assert(m_trie != nullptr);
    return m_trie->trie_db()->preimage(xh256_t{shaKey});
}

std::pair<xh256_t, int32_t> xtop_secure_trie::commit(std::error_code & ec) {
    assert(m_trie != nullptr);
    // Write all the pre-images to the actual disk database
    auto const sc = get_sec_key_cache();
    if (!sc->empty()) {
        for (auto const & scp : *sc) {
            m_trie->trie_db()->insertPreimage(xh256_t{top::to_bytes(top::get<std::string const>(scp))}, top::get<xbytes_t>(scp));
        }
        sc->clear();
    }
    return m_trie->commit(ec);
}

xh256_t xtop_secure_trie::hash() {
    assert(m_trie != nullptr);
    return m_trie->hash();
}

xbytes_t xtop_secure_trie::hash_key(xbytes_t const & key) const {
    xdbg("xtop_secure_trie::hashKey hashData:(%zu) %s ", key.size(), top::to_hex(key).c_str());
    xbytes_t hashbuf;
    utl::xkeccak256_t hasher;
    hasher.update(key.data(), key.size());
    hasher.get_hash(hashbuf);
    xdbg("xtop_secure_trie::hashKey -> hashed data:(%zu) %s", hashbuf.size(), top::to_hex(hashbuf).c_str());
    return hashbuf;
}

void xtop_secure_trie::prune(xh256_t const & old_trie_root_hash, std::error_code & ec) {
    assert(!ec);
    assert(m_trie != nullptr);

    m_trie->prune(old_trie_root_hash, ec);
}

void xtop_secure_trie::commit_pruned(std::error_code & ec) {
    assert(!ec);
    assert(m_trie != nullptr);

    m_trie->commit_pruned(ec);
}


NS_END3
