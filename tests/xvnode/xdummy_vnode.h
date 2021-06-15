// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xaddress.h"
#include "xcommon/xnode_type.h"
#include "xvnode/xvnode_face.h"

NS_BEG3(top, tests, vnode)

template <top::common::xnode_type_t NodeTypeV>
class xdummy_vnode; /* : public top::vnode::xvnode_face_t {
 public:
    xdummy_vnode() = default;
    xdummy_vnode(xdummy_vnode &&) = default;
    xdummy_vnode & operator=(xdummy_vnode &&) = default;

    top::common::xnode_type_t type() const override {
        return NodeTypeV;
    }

    top::common::xversion_t version() const override {
        return {};
    }

    top::common::xnode_address_t address() const override {
        return {};
    }

    void start() override {
    }

    void stop() override {
    }

    void fade() override {
    }

    void synchronize() override {
    }
};*/



#define XDUMMY_VNODE(TYPE)                                                                      \
    template <>                                                                                 \
    class xdummy_vnode<top::common::xnode_type_t::TYPE> : public top::vnode::xvnode_face_t {    \
        top::common::xnode_address_t address_;                                                  \
                                                                                                \
    public:                                                                                     \
        xdummy_vnode(xdummy_vnode &&) = default;                                                \
        xdummy_vnode & operator=(xdummy_vnode &&) = default;                                    \
                                                                                                \
        explicit xdummy_vnode(top::common::xnode_address_t address);                            \
                                                                                                \
        top::common::xnode_type_t type() const override {                                       \
            return address_.type();                                                             \
        }                                                                                       \
                                                                                                \
        top::common::xversion_t version() const override {                                      \
            return address_.version();                                                          \
        }                                                                                       \
                                                                                                \
        top::common::xnode_address_t address() const override {                                 \
            return address_;                                                                    \
        }                                                                                       \
                                                                                                \
        void start() override {                                                                 \
        }                                                                                       \
                                                                                                \
        void stop() override {                                                                  \
        }                                                                                       \
                                                                                                \
        void fade() override {                                                                  \
        }                                                                                       \
                                                                                                \
        void synchronize() override {                                                           \
        }                                                                                       \
    }

XDUMMY_VNODE(committee);
XDUMMY_VNODE(zec);
XDUMMY_VNODE(storage_archive);
XDUMMY_VNODE(edge);
XDUMMY_VNODE(consensus_auditor);
XDUMMY_VNODE(consensus_validator);

#undef XDUMMY_VNODE

NS_END3
