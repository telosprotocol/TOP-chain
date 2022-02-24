// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "tests/xvnode/xvnode_manager_fixture.h"

#include "tests/xvnode/xdummy_chain_timer_dida.h"
#include "tests/xvnode/xdummy_vhost.h"
#include "tests/xvnode/xdummy_vnode.h"
#include "tests/xvnode/xdummy_vnode_factory.h"

NS_BEG3(top, tests, vnode)

xvnode_factory::xvnode_factory() : xdummy_vnode_factory{top::make_observer(&xdummy_vhost)} {
}

std::shared_ptr<top::vnode::xvnode_face_t> xvnode_factory::create_vnode_at(std::shared_ptr<election::cache::xgroup_element_t> const & group_element) const {
    auto node_element = group_element->node_element(vhost_->host_node_id());
    assert(node_element);
    auto address = node_element->address();

    switch (common::real_part_type(group_element->type())) {
    case top::common::xnode_type_t::committee:
        return std::make_shared<xdummy_vnode<top::common::xnode_type_t::committee>>(std::move(address));

    case top::common::xnode_type_t::zec:
        return std::make_shared<xdummy_vnode<top::common::xnode_type_t::zec>>(std::move(address));

    case top::common::xnode_type_t::storage_archive:
        return std::make_shared<xdummy_vnode<top::common::xnode_type_t::storage_archive>>(std::move(address));

    case top::common::xnode_type_t::edge:
        return std::make_shared<xdummy_vnode<top::common::xnode_type_t::edge>>(std::move(address));

    case top::common::xnode_type_t::consensus_auditor:
        return std::make_shared<xdummy_vnode<top::common::xnode_type_t::consensus_auditor>>(std::move(address));

    case top::common::xnode_type_t::consensus_validator:
        return std::make_shared<xdummy_vnode<top::common::xnode_type_t::consensus_validator>>(std::move(address));

    default:
        assert(false);
        return {};
    }
}

xvnode_role_proxy::xvnode_role_proxy(){}
xvnode_sniff_proxy::xvnode_sniff_proxy(){}

xvnode_manager_fixture::xvnode_manager_fixture()
  : top::vnode::xvnode_manager_t{top::make_observer(&xdummy_chain_timer), top::make_observer(&xdummy_vhost), top::make_unique<xvnode_factory>(), top::make_unique<xvnode_role_proxy>(), top::make_unique<xvnode_sniff_proxy>()}
  , vhost_{top::make_observer(&xdummy_vhost)} {
}

void xvnode_manager_fixture::SetUp() {
    network_.reset();
}

void xvnode_manager_fixture::TearDown() {
    network_.reset();
}

NS_END3
