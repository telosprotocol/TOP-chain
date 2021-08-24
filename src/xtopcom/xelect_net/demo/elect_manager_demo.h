#pragma once
#define private public
#include "xdata/xelection/xelection_result_store.h"
#include "xelect_net/include/elect_manager.h"
#include "xpbase/base/top_config.h"
#include "xtransport/proto/transport.pb.h"
#include "xtransport/transport.h"

#include <memory>
// nlohmann_json
#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace top {

namespace elect {

class ElectManagerDemo : public ElectManager {
public:
    ElectManagerDemo(ElectManagerDemo const &) = delete;
    ElectManagerDemo & operator=(ElectManagerDemo const &) = delete;
    ElectManagerDemo(ElectManagerDemo &&) = delete;
    ElectManagerDemo & operator=(ElectManagerDemo &&) = delete;

    ElectManagerDemo(transport::TransportPtr transport, const base::Config & config);

    ~ElectManagerDemo() = default;

public:
    void OnElectUpdated(json all_info);
};

}  // namespace elect

}  // namespace top
