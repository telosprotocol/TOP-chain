#include "xdata/xblock_statistics_data.h"
#include "xcertauth/xcertauth_face.h"

#include "xvledger/xvblock.h"
// TODO(jimmy) #include "xbase/xvledger.h"
#include "xbase/xobject_ptr.h"

namespace top {
namespace data {

/// xblock_data_t

std::string xtop_block_data::to_json_string() const {
    return to_json_object<xJson::Value>().asString();
}

int32_t xtop_block_data::do_read(base::xstream_t & stream) {
    auto const size = stream.size();
    stream >> transaction_count;
    stream >> block_count;
    return size - stream.size();
}

int32_t xtop_block_data::do_write(base::xstream_t & stream) const {
    auto const size = stream.size();
    stream << transaction_count;
    stream << block_count;
    return stream.size() - size;
}

int32_t operator>>(base::xstream_t & stream, xblock_data_t & data_object) {
    return data_object.serialize_from(stream);
}

int32_t operator<<(base::xstream_t & stream, xblock_data_t const & data_object) {
    return data_object.serialize_to(stream);
}

/// xconsensus_vote_data_t
std::string xtop_consensus_vote_data::to_json_string() const {
    return to_json_object<xJson::Value>().asString();
}

int32_t xtop_consensus_vote_data::do_read(base::xstream_t & stream) {
    auto const size = stream.size();
    stream >> block_count;
    stream >> vote_count;
    return size - stream.size();
}

int32_t xtop_consensus_vote_data::do_write(base::xstream_t & stream) const {
    auto const size = stream.size();
    stream << block_count;
    stream << vote_count;
    return stream.size() - size;
}

int32_t operator>>(base::xstream_t & stream, xconsensus_vote_data_t & data_object) {
    return data_object.serialize_from(stream);
}

int32_t operator<<(base::xstream_t & stream, xconsensus_vote_data_t const & data_object) {
    return data_object.serialize_to(stream);
}

/// xaccount_related_statistics_data_t
std::string xtop_account_related_statistics_data::to_json_string() const {
    return to_json_object<xJson::Value>().asString();
}

int32_t xtop_account_related_statistics_data::do_read(base::xstream_t & stream) {
    auto const size = stream.size();
    stream >> block_data;
    stream >> vote_data;
    return size - stream.size();
}

int32_t xtop_account_related_statistics_data::do_write(base::xstream_t & stream) const {
    auto const size = stream.size();
    stream << block_data;
    stream << vote_data;
    return stream.size() - size;
}

int32_t operator>>(base::xstream_t & stream, xaccount_related_statistics_data_t & data_object) {
    return data_object.serialize_from(stream);
}

int32_t operator<<(base::xstream_t & stream, xaccount_related_statistics_data_t const & data_object) {
    return data_object.serialize_to(stream);
}

int32_t operator>>(base::xbuffer_t & buffer, xaccount_related_statistics_data_t & data_object) {
    return data_object.serialize_from(buffer);
}

int32_t operator<<(base::xbuffer_t & stream, xaccount_related_statistics_data_t const & data_object) {
    return data_object.serialize_to(stream);
}

/// xgroup_related_statistics_data_t
std::string xtop_group_releated_statistics_data::to_json_string() const {
    return to_json_object<xJson::Value>().asString();
}

int32_t xtop_group_releated_statistics_data::do_read(base::xstream_t & stream) {
    auto const size = stream.size();
    stream >> account_statistics_data;
    return size - stream.size();
}

int32_t xtop_group_releated_statistics_data::do_write(base::xstream_t & stream) const {
    auto const size = stream.size();
    stream << account_statistics_data;
    return stream.size() - size;
}

int32_t operator>>(base::xstream_t & stream, xgroup_related_statistics_data_t & data_object) {
    return data_object.serialize_from(stream);
}

int32_t operator<<(base::xstream_t & stream, xgroup_related_statistics_data_t const & data_object) {
    return data_object.serialize_to(stream);
}

int32_t operator>>(base::xbuffer_t & buffer, xgroup_related_statistics_data_t & data_object) {
    return data_object.serialize_from(buffer);
}

int32_t operator<<(base::xbuffer_t & buffer, xgroup_related_statistics_data_t const & data_object) {
    return data_object.serialize_to(buffer);
}

/// xelection_related_statistics_data_t
std::string xtop_election_related_statistics_data::to_json_string() const {
    return to_json_object<xJson::Value>().asString();
}

int32_t xtop_election_related_statistics_data::do_read(base::xstream_t & stream) {
    auto const size = stream.size();
    stream >> group_statistics_data;
    return size - stream.size();
}

int32_t xtop_election_related_statistics_data::do_write(base::xstream_t & stream) const {
    auto const size = stream.size();
    stream << group_statistics_data;
    return stream.size() - size;
}

int32_t operator>>(base::xstream_t & stream, xelection_related_statistics_data_t & data_object) {
    return data_object.serialize_from(stream);
}

int32_t operator<<(base::xstream_t & stream, xelection_related_statistics_data_t const & data_object) {
    return data_object.serialize_to(stream);
}

int32_t operator>>(base::xbuffer_t & buffer, xelection_related_statistics_data_t & data_object) {
    return data_object.serialize_from(buffer);
}

int32_t operator<<(base::xbuffer_t & buffer, xelection_related_statistics_data_t const & data_object) {
    return data_object.serialize_to(buffer);
}

/// xstatistics_data_t
std::string xtop_statistics_data::to_json_string() const {
    return to_json_object<xJson::Value>().asString();
}

int32_t xtop_statistics_data::do_read(base::xstream_t & stream) {
    auto const size = stream.size();
    stream >> detail;
    return size - stream.size();
}

int32_t xtop_statistics_data::do_write(base::xstream_t & stream) const {
    auto const size = stream.size();
    stream << detail;
    return stream.size() - size;
}

int32_t operator>>(base::xstream_t & stream, xstatistics_data_t & data_object) {
    return data_object.serialize_from(stream);
}

int32_t operator<<(base::xstream_t & stream, xstatistics_data_t const & data_object) {
    return data_object.serialize_to(stream);
}

}
}
