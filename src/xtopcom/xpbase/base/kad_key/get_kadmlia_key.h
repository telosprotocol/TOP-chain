#pragma once

#include "xpbase/base/kad_key/kadmlia_key.h"

namespace top {

namespace base {

base::KadmliaKeyPtr GetKadmliaKey(const base::XipParser& xip);
base::KadmliaKeyPtr GetKadmliaKey(const base::XipParser& xip, const std::string& node_id);
base::KadmliaKeyPtr GetKadmliaKey(const std::string& node_id, bool hash_tag);
base::KadmliaKeyPtr GetKadmliaKey(uint64_t service_type);
base::KadmliaKeyPtr GetKadmliaKey(const std::string& str_key);
base::KadmliaKeyPtr GetKadmliaKey();

}

}
