// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xmem.h"
#include "xbasic/xserializable_based_on.h"
#include "xcommon/xnode_id.h"
#include "xcommon/xnode_type.h"

NS_BEG2(top, data)

#define SLASH_DELETE_PROPERTY  "SLASH_DELETE_PROPERTY"

struct xnode_vote_percent_t final : public xserializable_based_on<void> {
    uint32_t block_count;
    uint32_t subset_count;

private:
    std::int32_t do_write(base::xstream_t & stream) const override;

    std::int32_t do_read(base::xstream_t & stream) override;
};

struct xunqualified_node_info_t final : public xserializable_based_on<void> {
    std::map<common::xnode_id_t, xnode_vote_percent_t> auditor_info;
    std::map<common::xnode_id_t, xnode_vote_percent_t> validator_info;

private:
    std::int32_t do_write(base::xstream_t & stream) const override;

    std::int32_t do_read(base::xstream_t & stream) override;
};

struct xunqualified_round_info_t final : public xserializable_based_on<void> {
    std::vector<uint64_t> rounds;
    std::map<uint64_t, xunqualified_node_info_t> round_info;

private:
    std::int32_t do_write(base::xstream_t & stream) const override;

    std::int32_t do_read(base::xstream_t & stream) override;
};

struct xunqualified_filter_info_t final : public xserializable_based_on<void> {
    common::xnode_id_t node_id;
    common::xnode_type_t node_type;
    std::uint32_t vote_percent;

private:
    std::int32_t do_write(base::xstream_t & stream) const override;

    std::int32_t do_read(base::xstream_t & stream) override;
};

struct xaction_node_info_t final : public xserializable_based_on<void> {
    common::xnode_id_t node_id;
    common::xnode_type_t node_type;
    bool action_type;  // default true for punish
    xaction_node_info_t() : node_id(common::xnode_id_t{}), node_type(common::xnode_type_t::invalid), action_type(true) {}
    xaction_node_info_t(common::xnode_id_t _node_id, common::xnode_type_t _node_type, bool type = true) : node_id(_node_id), node_type(_node_type), action_type(type) {}

private:
    std::int32_t do_write(base::xstream_t & stream) const override;

    std::int32_t do_read(base::xstream_t & stream) override;
};

NS_END2
