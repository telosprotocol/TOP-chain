#include "xevm_common/trie/xtrie.h"
#include "xevm_common/trie/xtrie_proof.h"
#include "xevm_common/xerror/xerror.h"

#include <gtest/gtest.h>

#include "xbasic/xspan.h"

#include <atomic>

NS_BEG4(top, evm_common, trie, tests)

class xmock_disk_db : public xkv_db_face_t {
public:
    void Put(xspan_t<xbyte_t const> key, xbytes_t const & value, std::error_code & ec) override {
        xdbg("xmock_disk_db Put key: %s", top::to_hex(key).c_str());
        m[xbytes_t{std::begin(key), std::end(key)}] = value;
    }

    void Delete(xbytes_t const & key, std::error_code & ec) {
        m.erase(key);
    }

    bool has(xspan_t<xbyte_t const> const key, std::error_code & ec) const {
        return m.find(xbytes_t{key.begin(), key.end()}) != m.end();
    }

    xbytes_t get(xspan_t<xbyte_t const> const key, std::error_code & ec) const override {
        if (!has(key, ec)) {
            ec = error::xerrc_t::trie_proof_missing;
            return xbytes_t{};
        }
        Counter_Get++;
        return m[xbytes_t{key.begin(), key.end()}];
    }

    void debug() {
        for (auto const & p : m) {
            xdbg("%s : %s", top::to_hex(p.first).c_str(), top::to_hex(p.second).c_str());
        }
    }

    void PutDirect(xbytes_t const & key, xbytes_t const & value, std::error_code & ec) override {
    }

    void PutDirectBatch(std::map<xbytes_t, xbytes_t> const & batch, std::error_code & ec) override {
    }

    void DeleteDirect(xbytes_t const & key, std::error_code & ec) override {
    }

    void DeleteDirectBatch(std::vector<xbytes_t> const & batch, std::error_code & ec) override {
    }

    bool HasDirect(xbytes_t const &, std::error_code &) const override {
        return false;
    }

    xbytes_t GetDirect(xbytes_t const &, std::error_code &) const override {
        return {};
    }

    void PutBatch(std::map<xh256_t, xbytes_t> const & batch, std::error_code & ec) override {
        for (auto & p : batch) {
            m[p.first.to_bytes()] = p.second;
        }
    }

    void DeleteBatch(std::vector<xspan_t<xbyte_t const>> const & batch, std::error_code &) override {
        for (auto const & key : batch) {
            m.erase(xbytes_t{std::begin(key), std::end(key)});
        }
    }

    mutable std::map<xbytes_t, xbytes_t> m;

    mutable std::atomic<uint64_t> Counter_Get{0};

    bool empty() const noexcept {
        return m.empty();
    }

    size_t size() const noexcept {
        return m.size();
    }

    common::xtable_address_t table_address() const override {
        return common::xtable_address_t::build_from(xstring_view_t{"Ta0000@0"});
    }
};
using xmock_disk_db_ptr = std::shared_ptr<xmock_disk_db>;

class xmock_prove_db : public xkv_db_face_t {
public:
    void Put(xspan_t<xbyte_t const> key, xbytes_t const & value, std::error_code & ec) override {
        xdbg("xmock_prove_db Put key: %s", top::to_hex(key).c_str());
        m[xbytes_t{std::begin(key), std::end(key)}] = value;
    }
    void Delete(xbytes_t const & key, std::error_code & ec) {
        m.erase(key);
    }

    bool has(xspan_t<xbyte_t const> const key, std::error_code & ec) const override {
        return m.find(xbytes_t{std::begin(key), std::end(key)}) != m.end();
    }

    xbytes_t get(xspan_t<xbyte_t const> const key, std::error_code & ec) const override {
        if (!has(key, ec)) {
            ec = error::xerrc_t::trie_proof_missing;
            return xbytes_t{};
        }
        return m.at(xbytes_t{std::begin(key), std::end(key)});
    }

    void PutDirect(xbytes_t const & key, xbytes_t const & value, std::error_code & ec) override {

    }
    void PutDirectBatch(std::map<xbytes_t, xbytes_t> const & batch, std::error_code & ec) override {

    }
    void DeleteDirect(xbytes_t const & key, std::error_code & ec) override {

    }
    void DeleteDirectBatch(std::vector<xbytes_t> const & batch, std::error_code & ec) override {

    }
    bool HasDirect(xbytes_t const &, std::error_code &) const override {
        return false;
    }

    xbytes_t GetDirect(xbytes_t const &, std::error_code &) const override {
        return {};
    }
    void PutBatch(std::map<xh256_t, xbytes_t> const & batch, std::error_code & ec) override {
    }

    void DeleteBatch(std::vector<xspan_t<xbyte_t const>> const & batch, std::error_code & ec) override {
        for (auto const & key : batch) {
            m.erase(xbytes_t{std::begin(key), std::end(key)});
        }
    }

    common::xtable_address_t table_address() const override {
        return common::xtable_address_t::build_from(xstring_view_t{"Ta0000@0"});
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
