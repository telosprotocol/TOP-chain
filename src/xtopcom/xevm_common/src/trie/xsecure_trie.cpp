// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_common/trie/xsecure_trie.h"

#include "xutility/xhash.h"

NS_BEG3(top, evm_common, trie)

std::shared_ptr<xtop_secure_trie> xtop_secure_trie::NewSecure(xhash256_t root, xtrie_db_ptr_t db, std::error_code & ec) {
    if (db == nullptr) {
        xerror("build secure trie from null db");
    }
    auto trie = xtrie_t::New(root, db, ec);
    if (ec) {
        xwarn("secure trie new failed: %s", ec.message().c_str());
        return nullptr;
    }
    return std::make_shared<xtop_secure_trie>(*trie.get());
}

xbytes_t xtop_secure_trie::Get(xbytes_t const & key) const {
    std::error_code ec;
    auto result = TryGet(key, ec);
    if (ec) {
        xerror("secure trie error: %s", ec.message().c_str());
    }
    return result;
}

xbytes_t xtop_secure_trie::TryGet(xbytes_t const & key, std::error_code & ec) const {
    return m_trie.TryGet(hashKey(key), ec);
}

void xtop_secure_trie::Update(xbytes_t const & key, xbytes_t const & value) {
    std::error_code ec;
    TryUpdate(key, value, ec);
    if (ec) {
        xerror("secure trie error: %s", ec.message().c_str());
    }
    return;
}

void xtop_secure_trie::TryUpdate(xbytes_t const & key, xbytes_t const & value, std::error_code & ec) {
    auto hk = hashKey(key);
    m_trie.TryUpdate(hk, value, ec);
    if (ec) {
        return;
    }
    getSecKeyCache()->operator[](top::to_string(hk)) = key;
    return;
}

void xtop_secure_trie::Delete(xbytes_t const & key) {
    std::error_code ec;
    TryDelete(key, ec);
    if (ec) {
        xerror("secure trie error: %s", ec.message().c_str());
    }
    return;
}

void xtop_secure_trie::TryDelete(xbytes_t const & key, std::error_code & ec) {
    auto hk = hashKey(key);
    getSecKeyCache()->erase(top::to_string(hk));
    m_trie.TryDelete(hk, ec);
}

xbytes_t xtop_secure_trie::GetKey(xbytes_t const & shaKey) {
    auto sc = getSecKeyCache();
    if (sc->find(top::to_string(shaKey)) != sc->end()) {
        return sc->at(top::to_string(shaKey));
    }
    return m_trie.trie_db()->preimage(xhash256_t{shaKey});
}

std::pair<xhash256_t, int32_t> xtop_secure_trie::Commit(std::error_code & ec) {
    // Write all the pre-images to the actual disk database
    auto sc = getSecKeyCache();
    if (sc->size() > 0) {
        for (auto scp : *sc.get()) {
            m_trie.trie_db()->insertPreimage(xhash256_t{top::to_bytes(scp.first)}, scp.second);
        }
        sc->clear();
    }
    return m_trie.Commit(ec);
}

xhash256_t xtop_secure_trie::Hash() {
    return m_trie.Hash();
}

xbytes_t xtop_secure_trie::hashKey(xbytes_t const & key) const {
    xdbg("xtop_secure_trie::hashKey hashData:(%zu) %s ", key.size(), top::to_hex(key).c_str());
    xbytes_t hashbuf;
    utl::xkeccak256_t hasher;
    hasher.update(key.data(), key.size());
    hasher.get_hash(hashbuf);
    xdbg("xtop_secure_trie::hashKey -> hashed data:(%zu) %s", hashbuf.size(), top::to_hex(hashbuf).c_str());
    return hashbuf;
}

NS_END3