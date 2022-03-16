// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcodec/xmsgpack_codec.hpp"
#include "xcommon/xsharding_info.h"
#include "xconfig/xconfig_register.h"
#include "xdata/xcodec/xmsgpack/xelection_association_result_store_codec.hpp"
#include "xdata/xelection/xelection_association_result_store.h"
#include "xconfig/xpredefined_configurations.h"
#include "xvm/xserialization/xserialization.h"
#include "xvm/xsystem_contracts/xelection/xzec/xzec_group_association_contract.h"

using top::data::election::xelection_association_result_store_t;

NS_BEG4(top, xvm, system_contracts, zec)

xtop_group_association_contract::xtop_group_association_contract(common::xnetwork_id_t const & network_id)
    : xbase_t{ network_id } {
}

xcontract::xcontract_base *
xtop_group_association_contract::clone() {
    return new xtop_group_association_contract{ network_id() };
}

void
xtop_group_association_contract::setup() {
    auto const & config_register = top::config::xconfig_register_t::get_instance();

    auto const validator_group_count = XGET_CONFIG(validator_group_count);
    auto const auditor_group_count = XGET_CONFIG(auditor_group_count);

    auto const validator_group_count_per_auditor_group = validator_group_count / auditor_group_count;

    // static initialize assocation between consensus group and advance group.
    data::election::xelection_association_result_store_t election_association_result_store;
    auto & election_association_result = election_association_result_store.result_of(common::xdefault_cluster_id);
    election_association_result.cluster_version(common::xelection_round_t{ 0 });   // version starts from zero
    for (std::uint16_t i = 0u; i < validator_group_count; ++i) {
        common::xgroup_id_t consensus_gid{
            static_cast<common::xgroup_id_t::value_type>(common::xvalidator_group_id_value_begin + i)
        };
        assert(consensus_gid.value() >= common::xvalidator_group_id_value_begin);
        assert(consensus_gid.value() < common::xvalidator_group_id_value_end);
        assert(static_cast<std::uint16_t>(consensus_gid.value() - common::xvalidator_group_id_value_begin) < validator_group_count);

        auto const advance_group_id_offset = static_cast<std::uint16_t>(i / validator_group_count_per_auditor_group);

        common::xgroup_id_t associated_advance_gid{
            static_cast<common::xgroup_id_t::value_type>(common::xauditor_group_id_value_begin + advance_group_id_offset)
        };
        assert(associated_advance_gid.value() >= common::xauditor_group_id_value_begin);
        assert(associated_advance_gid.value() < common::xauditor_group_id_value_end);
        assert(static_cast<std::uint16_t>(associated_advance_gid.value() - common::xauditor_group_id_value_begin) < auditor_group_count);

        election_association_result.insert({ consensus_gid, associated_advance_gid });
        xkinfo("[zec contract][group_association] default association: %s belongs to %s",
               consensus_gid.to_string().c_str(),
               associated_advance_gid.to_string().c_str());
    }

    STRING_CREATE(data::XPROPERTY_CONTRACT_GROUP_ASSOC_KEY);
    serialization::xmsgpack_t<xelection_association_result_store_t>::serialize_to_string_prop(*this, data::XPROPERTY_CONTRACT_GROUP_ASSOC_KEY, election_association_result_store);
}


NS_END4
