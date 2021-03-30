// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "xdata/xproposal_data.h"
#include "xbase/xutl.h"

NS_BEG2(top, tcc)

 int32_t proposal_info::serialize(base::xstream_t & stream) const {
    const int32_t begin_size = stream.size();
    stream << proposal_id;
    stream << parameter;
    stream << new_value;
    stream << modification_description;
    stream << proposal_client_address;
    stream << type;
    stream << deposit;
    stream << effective_timer_height;
    stream << priority;
    stream << cosigning_status;
    stream << voting_status;
    stream << end_time;
    const int32_t end_size = stream.size();
    return (begin_size - end_size);
 }


 int32_t proposal_info::deserialize(base::xstream_t & stream) {
    const int32_t begin_size = stream.size();
    stream >> proposal_id;
    stream >> parameter;
    stream >> new_value;
    stream >> modification_description;
    stream >> proposal_client_address;
    stream >> type;
    stream >> deposit;
    stream >> effective_timer_height;
    stream >> priority;
    stream >> cosigning_status;
    stream >> voting_status;
    stream >> end_time;
    const int32_t end_size = stream.size();
    return (end_size - begin_size);
 }

NS_END2