#include "xdata/xblock_statistics_cons_data.h"

namespace top {
namespace data {

/// xaccount_statistics_cons_data_t
std::string xtop_account_statistics_cons_data::to_json_string() const {
    return to_json_object<Json::Value>().asString();
}

int32_t xtop_account_statistics_cons_data::do_read(base::xstream_t & stream) {
    auto const size = stream.size();
    stream >> burn_gas_value;
    return size - stream.size();
}

int32_t xtop_account_statistics_cons_data::do_write(base::xstream_t & stream) const {
    auto const size = stream.size();
    stream << burn_gas_value;
    return stream.size() - size;
}

int32_t operator>>(base::xstream_t & stream, xaccount_statistics_cons_data_t & data_object) {
    return data_object.serialize_from(stream);
}

int32_t operator<<(base::xstream_t & stream, xaccount_statistics_cons_data_t const & data_object) {
    return data_object.serialize_to(stream);
}

int32_t operator>>(base::xbuffer_t & buffer, xaccount_statistics_cons_data_t & data_object) {
    return data_object.serialize_from(buffer);
}

int32_t operator<<(base::xbuffer_t & stream, xaccount_statistics_cons_data_t const & data_object) {
    return data_object.serialize_to(stream);
}

/// xgroup_statistics_cons_data_t
std::string xtop_group_statistics_cons_data::to_json_string() const {
    return to_json_object<Json::Value>().asString();
}

int32_t xtop_group_statistics_cons_data::do_read(base::xstream_t & stream) {
    auto const size = stream.size();
    stream >> account_statistics_data;
    return size - stream.size();
}

int32_t xtop_group_statistics_cons_data::do_write(base::xstream_t & stream) const {
    auto const size = stream.size();
    stream << account_statistics_data;
    return stream.size() - size;
}

int32_t operator>>(base::xstream_t & stream, xgroup_statistics_cons_data_t & data_object) {
    return data_object.serialize_from(stream);
}

int32_t operator<<(base::xstream_t & stream, xgroup_statistics_cons_data_t const & data_object) {
    return data_object.serialize_to(stream);
}

int32_t operator>>(base::xbuffer_t & buffer, xgroup_statistics_cons_data_t & data_object) {
    return data_object.serialize_from(buffer);
}

int32_t operator<<(base::xbuffer_t & buffer, xgroup_statistics_cons_data_t const & data_object) {
    return data_object.serialize_to(buffer);
}

/// xelection_statistics_cons_data_t
std::string xtop_election_statistics_cons_data::to_json_string() const {
    return to_json_object<Json::Value>().asString();
}

int32_t xtop_election_statistics_cons_data::do_read(base::xstream_t & stream) {
    auto const size = stream.size();
    stream >> group_statistics_data;
    return size - stream.size();
}

int32_t xtop_election_statistics_cons_data::do_write(base::xstream_t & stream) const {
    auto const size = stream.size();
    stream << group_statistics_data;
    return stream.size() - size;
}

int32_t operator>>(base::xstream_t & stream, xelection_statistics_cons_data_t & data_object) {
    return data_object.serialize_from(stream);
}

int32_t operator<<(base::xstream_t & stream, xelection_statistics_cons_data_t const & data_object) {
    return data_object.serialize_to(stream);
}

int32_t operator>>(base::xbuffer_t & buffer, xelection_statistics_cons_data_t & data_object) {
    return data_object.serialize_from(buffer);
}

int32_t operator<<(base::xbuffer_t & buffer, xelection_statistics_cons_data_t const & data_object) {
    return data_object.serialize_to(buffer);
}

/// xstatistics_cons_data_t
std::string xtop_statistics_cons_data::to_json_string() const {
    return to_json_object<Json::Value>().asString();
}

int32_t xtop_statistics_cons_data::do_read(base::xstream_t & stream) {
    auto const size = stream.size();
    stream >> detail;
    stream >> total_gas_burn;
    return size - stream.size();
}

int32_t xtop_statistics_cons_data::do_write(base::xstream_t & stream) const {
    auto const size = stream.size();
    stream << detail;
    stream << total_gas_burn;
    return stream.size() - size;
}

int32_t operator>>(base::xstream_t & stream, xstatistics_cons_data_t & data_object) {
    return data_object.serialize_from(stream);
}

int32_t operator<<(base::xstream_t & stream, xstatistics_cons_data_t const & data_object) {
    return data_object.serialize_to(stream);
}

}
}
