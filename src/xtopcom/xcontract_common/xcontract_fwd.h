#pragma once

#include "xbase/xns_macro.h"

NS_BEG2(top, contract_common)

class xtop_contract_face;
using xcontract_face_t = xtop_contract_face;

enum class xtop_contract_type: std::uint8_t;
using xcontract_type_t = xtop_contract_type;

struct xtop_contract_metadata;
using xcontract_metadata_t = xtop_contract_metadata;

class xtop_basic_contract;
using xbasic_contract_t = xtop_basic_contract;



NS_END2
