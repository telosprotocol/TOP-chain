#include "xevm_common/trie/xtrie.h"
#include "xevm_common/trie/xtrie_proof.h"
#include "xevm_common/xerror/xerror.h"

#include <gtest/gtest.h>

NS_BEG4(top, evm_common, trie, tests)

class xmock_disk_db : public xkv_db_face_t {
public:
    void Put(xbytes_t const & key, xbytes_t const & value, std::error_code & ec) {
        xdbg("xmock_disk_db Put key: %s", top::to_hex(key).c_str());
        m[key] = value;
    }
    void Delete(xbytes_t const & key, std::error_code & ec) {
        m.erase(key);
    }
    bool Has(xbytes_t const & key, std::error_code & ec) {
        return m.find(key) != m.end();
    }
    xbytes_t Get(xbytes_t const & key, std::error_code & ec) {
        if (!Has(key, ec)) {
            ec = error::xerrc_t::trie_proof_missing;
            return xbytes_t{};
        }
        return m[key];
    }
    void debug() {
        for (auto const & p : m) {
            xdbg("%s : %s", top::to_hex(p.first).c_str(), top::to_hex(p.second).c_str());
        }
    }

    std::map<xbytes_t, xbytes_t> m;
};
using xmock_disk_db_ptr = std::shared_ptr<xmock_disk_db>;

class xmock_prove_db : public xkv_db_face_t {
public:
    void Put(xbytes_t const & key, xbytes_t const & value, std::error_code & ec) {
        xdbg("xmock_prove_db Put key: %s", top::to_hex(key).c_str());
        m[key] = value;
    }
    void Delete(xbytes_t const & key, std::error_code & ec) {
        m.erase(key);
    }
    bool Has(xbytes_t const & key, std::error_code & ec) {
        return m.find(key) != m.end();
    }
    xbytes_t Get(xbytes_t const & key, std::error_code & ec) {
        if (!Has(key, ec)) {
            ec = error::xerrc_t::trie_proof_missing;
            return xbytes_t{};
        }
        return m[key];
    }

    std::map<xbytes_t, xbytes_t> m;
};
using xmock_prove_db_ptr = std::shared_ptr<xmock_prove_db>;

class xtest_trie_fixture : public testing::Test {
public:
    xtest_trie_fixture() = default;
    xtest_trie_fixture(xtest_trie_fixture const &) = delete;
    xtest_trie_fixture & operator=(xtest_trie_fixture const &) = delete;
    xtest_trie_fixture(xtest_trie_fixture &&) = default;
    xtest_trie_fixture & operator=(xtest_trie_fixture &&) = default;
    ~xtest_trie_fixture() override = default;

protected:
    void SetUp() override {
        test_disk_db_ptr = std::make_shared<xmock_disk_db>();
        test_trie_db_ptr = xtrie_db_t::NewDatabase(test_disk_db_ptr);
        test_proof_db_ptr = std::make_shared<xmock_prove_db>();
        Test::SetUp();
    }

    void TearDown() override {
        test_disk_db_ptr.reset();
        test_trie_db_ptr.reset();
        test_proof_db_ptr.reset();
        Test::TearDown();
    }

public:
    xmock_disk_db_ptr test_disk_db_ptr{nullptr};
    xtrie_db_ptr_t test_trie_db_ptr{nullptr};
    xmock_prove_db_ptr test_proof_db_ptr{nullptr};
};

NS_END4