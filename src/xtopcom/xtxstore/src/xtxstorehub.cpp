// Copyright (c) 2018-2020 Telos Foundation & contributors
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxstore/xtxstore_face.h"
#include "xtxstore/xtxstoreimpl.h"

NS_BEG2(top, txstore)

class xtxstorehub {
public:
    xtxstorehub();
    xtxstorehub(xtxstorehub const &) = delete;
    xtxstorehub & operator=(xtxstorehub const &) = delete;
    xtxstorehub(xtxstorehub &&) = delete;
    xtxstorehub & operator=(xtxstorehub &&) = delete;
    ~xtxstorehub();

public:
    base::xvtxstore_t * get_txstore();
};

xtxstorehub::xtxstorehub() {
}
xtxstorehub::~xtxstorehub() {
}

base::xvtxstore_t * xtxstorehub::get_txstore() {
    static xtxstoreimpl * _static_txstore = nullptr;
    if (_static_txstore)
        return _static_txstore;

    _static_txstore = new xtxstoreimpl();

    base::xvchain_t::instance().set_xtxstore(_static_txstore);
    return _static_txstore;
}

base::xvtxstore_t * get_txstore() {
    static xtxstorehub _static_txstore_hub;
    return _static_txstore_hub.get_txstore();
}

NS_END2