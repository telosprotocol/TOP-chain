// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#define private public

#include "xcommon/xnode_id.h"
#include "xelection/xcache/xnetwork_element.h"
#include "tests/xvnode/xdummy_vnode_factory.h"
#include "tests/xvnode/xdummy_vnode_proxy.h"
#include "xvnode/xvnode_manager.h"
#include "xvnetwork/xvhost_face.h"

#include <gtest/gtest.h>

NS_BEG3(top, tests, vnode)

class xvnode_factory : public xdummy_vnode_factory {
public:
    xvnode_factory();

    std::shared_ptr<top::vnode::xvnode_face_t> create_vnode_at(std::shared_ptr<election::cache::xgroup_element_t> const & group_element) const override;
};

class xvnode_role_proxy : public xdummy_vnode_proxy {
public:
    xvnode_role_proxy();
};

class xvnode_sniff_proxy : public xdummy_vnode_sniff_proxy {
public:
    xvnode_sniff_proxy();
};

class xvnode_manager_fixture : public testing::Test, public top::vnode::xvnode_manager_t {
protected:
    observer_ptr<top::vnetwork::xvhost_face_t> vhost_;
    std::shared_ptr<election::cache::xnetwork_element_t> network_;

public:
    xvnode_manager_fixture();

    void SetUp() override;
    void TearDown() override;
};

NS_END3
