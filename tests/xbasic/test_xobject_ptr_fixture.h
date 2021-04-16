// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xobject.h"
#include "xbase/xns_macro.h"

NS_BEG3(top, tests, basic)

struct xbase_object1 : top::base::xobject_t {
public:
    xbase_object1() = default;

protected:
    ~xbase_object1() override = default;
};

struct xbase_object2 : top::base::xobject_t {
public:
    xbase_object2() = default;

protected:
    ~xbase_object2() override = default;
};

struct xderived_object1 : xbase_object1 {
public:
    xderived_object1() = default;

protected:
    ~xderived_object1() override = default;
};

struct xderived_object2
  : xbase_object1
  , xbase_object2 {
public:
    xderived_object2() = default;

protected:
    ~xderived_object2() override = default;
};

NS_END3
