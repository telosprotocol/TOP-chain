#pragma once

#include "nlohmann/fifo_map.hpp"
#include "nlohmann/json.hpp"
#include "xbase/xobject_ptr.h"
#include "xvledger/xvstate.h"

#include <sys/stat.h>
#include <sys/types.h>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <thread>

NS_BEG2(top, db_export)

template <class K, class V, class dummy_compare, class A>
using my_workaround_fifo_map = nlohmann::fifo_map<K, V, nlohmann::fifo_map_compare<K>, A>;

using unordered_json = nlohmann::basic_json<my_workaround_fifo_map>;
using json = unordered_json;

json property_json(xobject_ptr_t<base::xvbstate_t> const & state);

NS_END2