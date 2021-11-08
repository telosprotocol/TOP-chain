// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xfulltableblock_account_data.h"

NS_BEG2(top, data)

int32_t xtop_fulltableblock_account_data::do_read(base::xstream_t & stream) {
    auto const size = stream.size();
    stream >> account_data;
    return size - stream.size();
}

int32_t xtop_fulltableblock_account_data::do_write(base::xstream_t & stream) const {
    auto const size = stream.size();
    stream << account_data;
    return stream.size() - size;
}

int32_t operator>>(base::xstream_t& stream, xfulltableblock_account_data_t& data_object) {
    return data_object.serialize_from(stream);
}
int32_t operator<<(base::xstream_t& stream, xfulltableblock_account_data_t const& data_object) {
    return data_object.serialize_to(stream);
}

int32_t operator>>(base::xbuffer_t & buffer, xfulltableblock_account_data_t& data_object) {
    return data_object.serialize_from(buffer);
}
int32_t operator<<(base::xbuffer_t & buffer, xfulltableblock_account_data_t const& data_object) {
    return data_object.serialize_to(buffer);
}

int32_t xtop_fulltableblock_group_data::do_read(base::xstream_t & stream) {
    auto const size = stream.size();
    stream >> group_data;
    return size - stream.size();
}

int32_t xtop_fulltableblock_group_data::do_write(base::xstream_t & stream) const {
    auto const size = stream.size();
    stream << group_data;
    return stream.size() - size;
}

int32_t operator>>(base::xstream_t& stream, xfulltableblock_group_data_t& data_object) {
    return data_object.serialize_from(stream);
}
int32_t operator<<(base::xstream_t& stream, xfulltableblock_group_data_t const& data_object) {
    return data_object.serialize_to(stream);
}

int32_t operator>>(base::xbuffer_t & buffer, xfulltableblock_group_data_t& data_object) {
    return data_object.serialize_from(buffer);
}
int32_t operator<<(base::xbuffer_t & buffer, xfulltableblock_group_data_t const& data_object) {
    return data_object.serialize_to(buffer);
}

int32_t xtop_fulltableblock_statistic_accounts::do_read(base::xstream_t & stream) {
    auto const size = stream.size();
    stream >> accounts_detail;
    return size - stream.size();
}

int32_t xtop_fulltableblock_statistic_accounts::do_write(base::xstream_t & stream) const {
    auto const size = stream.size();
    stream << accounts_detail;
    return stream.size() - size;
}

int32_t operator>>(base::xstream_t& stream, xfulltableblock_statistic_accounts& data_object) {
    return data_object.serialize_from(stream);
}
int32_t operator<<(base::xstream_t& stream, xfulltableblock_statistic_accounts const& data_object) {
    return data_object.serialize_to(stream);
}

int32_t operator>>(base::xbuffer_t & buffer, xfulltableblock_statistic_accounts& data_object) {
    return data_object.serialize_from(buffer);
}
int32_t operator<<(base::xbuffer_t & buffer, xfulltableblock_statistic_accounts const& data_object) {
    return data_object.serialize_to(buffer);
}

NS_END2