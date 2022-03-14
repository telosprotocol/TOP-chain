
#include "xtxpool_service_v2/xtxpool_service_face.h"

using namespace top;

class xtop_dummy_txpool_proxy_face : public top::xtxpool_service_v2::xtxpool_proxy_face {
public:
    bool start() override { return false; }
    bool unreg() override { return false; }
    bool fade() override { return false; }
    int32_t request_transaction_consensus(const top::data::xtransaction_ptr_t & trans, bool local) override { return -1; }
    data::xcons_transaction_ptr_t query_tx(const std::string & account, const uint256_t & hash) const override {
        return nullptr;
    }
};
