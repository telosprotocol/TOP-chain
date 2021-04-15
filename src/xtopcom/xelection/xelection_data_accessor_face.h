// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

NS_BEG2(top, election)

class xtop_election_data_accessor_face {
public:
    xtop_election_data_accessor_face()                                            = default;
    xtop_election_data_accessor_face(xtop_election_data_accessor_face const &)             = default;
    xtop_election_data_accessor_face & operator=(xtop_election_data_accessor_face const &) = default;
    xtop_election_data_accessor_face(xtop_election_data_accessor_face &&)                  = default;
    xtop_election_data_accessor_face & operator=(xtop_election_data_accessor_face &&)      = default;
    virtual ~xtop_election_data_accessor_face()                                   = default;


};
using xelection_data_accessor_face_t = xtop_election_data_accessor_face;

NS_END2
