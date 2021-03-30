// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.#pragma once

#pragma once

#include <string>
#include <vector>
#include <atomic>
#include <map>
#include <mutex>
#include "xbase/xint.h"
#include "xstore/xstore_face.h"
#include "xdata/xtransaction.h"
#include "json/json.h"

NS_BEG2(top, elect)

class xelect_client_imp {
 public:
    xelect_client_imp() = default;
    ~xelect_client_imp() = default;

    void bootstrap_node_join();
    void tx_to_json(xJson::Value& tx_json, const data::xtransaction_ptr_t& tx);
};

NS_END2
