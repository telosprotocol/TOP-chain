// Copyright (c) 2017-2022 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xsync/xsync_message.h"
#include "xsync/xsync_log.h"
#include "xsync/xsync_util.h"

NS_BEG2(top, sync)

//xsync_msg_t
xsync_msg_t::xsync_msg_t()
{
    sessionID = RandomUint64();
}

xsync_msg_t::xsync_msg_t(uint64_t _sessionID)
{
    sessionID = _sessionID;
}

int32_t xsync_msg_t::do_write(base::xstream_t& stream)
{
    const int32_t begin_size = stream.size();
    stream << sessionID;
    return (stream.size() - begin_size);
}

int32_t xsync_msg_t::do_read(base::xstream_t& stream)
{
    const int32_t begin_size = stream.size();
    stream >> sessionID;
    return (begin_size - stream.size());
}

//xsync_msg_block_t 

const std::string& xsync_msg_block_t::get_address() const
{
    return address;
}

std::uint32_t xsync_msg_block_t::get_request_type() const
{
    return (request_option & 0xff);
}

std::uint32_t xsync_msg_block_t::get_requeset_param_type() const
{
    return ((request_option >> 8) & 0xFF);
}

std::uint32_t xsync_msg_block_t::get_data_type() const
{
    return ((request_option >> 16) & 0xFF);
}

std::uint32_t xsync_msg_block_t::get_block_object_type() const
{
    return ((request_option >> 24) & 0xF);
}

std::uint32_t xsync_msg_block_t::get_extend_bits() const
{
    return ((request_option >> 28) & 0xF);
}

int32_t xsync_msg_block_t::do_write(base::xstream_t& stream)
{
    const int32_t begin_size = stream.size();
    xsync_msg_t::do_write(stream);
    stream << address;
    stream << request_option;
    return (stream.size() - begin_size);
}

int32_t xsync_msg_block_t::do_read(base::xstream_t& stream)
{
    const int32_t begin_size = stream.size();
    xsync_msg_t::do_read(stream);
    stream >> address;
    stream >> request_option;
    return (begin_size - stream.size());
}


//xsync_msg_block_request_t

xsync_msg_block_request_t::xsync_msg_block_request_t(std::string const& _address, uint32_t _request_option, 
                                                    uint64_t _height, uint32_t _count, std::string const& _param)
    : xsync_msg_block_t(_address, _request_option)
    , height_start(_height)
    , count(_count)
    , param(_param)
{
    tm = base::xtime_utl::gmttime_ms();
}

int32_t xsync_msg_block_request_t::do_write(base::xstream_t& stream)
{
    const int32_t begin_size = stream.size();
    xsync_msg_block_t::do_write(stream);
    stream << height_start;
    stream << count;
    stream << param;
    return (stream.size() - begin_size);
}

int32_t xsync_msg_block_request_t::do_read(base::xstream_t& stream)
{
    const int32_t begin_size = stream.size();
    xsync_msg_block_t::do_read(stream);
    stream >> height_start;
    stream >> count;
    stream >> param;
    return (begin_size - stream.size());
}

std::string xsync_msg_block_request_t::dump() const
{
    std::stringstream ss;
    ss << "{msg_block_request: address: ";
    ss << get_address();
    ss << ", type:";
    ss << get_request_type();
    ss << ", param:";
    ss << get_requeset_param_type();
    ss << ", data_type:";
    ss << get_data_type();
    ss << ", object_typ:";
    ss << get_block_object_type();
    ss << ", extend_bit:";
    ss << get_extend_bits();
    ss << ", height:";
    ss << height_start;
    ss << ", count:";
    ss << count;
    ss << ", param:";
    ss << param;
    ss << "}";
    return ss.str();
}

//xsync_msg_block_response_t

xsync_msg_block_response_t::xsync_msg_block_response_t(uint64_t _sessionID, std::string const& _address, uint32_t _request_option, 
    std::vector<std::string>& _blocks_data, uint32_t _response_option , std::string const& _extend_data)
    : xsync_msg_block_t(_sessionID, _address, _request_option)
    , response_option(_response_option)
    , extend_data(_extend_data)
    , blocks_data(_blocks_data)
{
}

int32_t xsync_msg_block_response_t::do_write(base::xstream_t& stream)
{
    const int32_t begin_size = stream.size();
    xsync_msg_block_t::do_write(stream);
    stream << response_option;
    stream << extend_data;
    const uint32_t data_count = (uint32_t)blocks_data.size();
    stream << data_count;
    for (auto& data : blocks_data) {
        stream << data;
    }

    return (stream.size() - begin_size);
}

int32_t xsync_msg_block_response_t::do_read(base::xstream_t& stream)
{
    const int32_t begin_size = stream.size();
    xsync_msg_block_t::do_read(stream);
    stream >> response_option;
    stream >> extend_data;
    uint32_t data_count = 0;
    stream >> data_count;
    for (uint32_t i = 0; i < data_count; i++) {
        std::string data;
        stream >> data;
        blocks_data.push_back(data);
    }

    return (begin_size - stream.size());
}

std::vector<data::xblock_ptr_t> xsync_msg_block_response_t::get_all_xblock_ptr() const
{
 return convert_stream_to_blocks(get_data_type(), blocks_data);
}

//xsync_msg_block_push_t

xsync_msg_block_push_t::xsync_msg_block_push_t(std::string const& _address, uint32_t _request_option,
        std::vector<std::string> &_blocks_data,uint32_t _push_option, std::string const& _extend_data)
    : xsync_msg_block_t(_address, _request_option)
    , push_option(_push_option)
    , extend_data(_extend_data)
    , blocks_data(_blocks_data)
{
}

int32_t xsync_msg_block_push_t::do_write(base::xstream_t& stream)
{
    const int32_t begin_size = stream.size();
    xsync_msg_block_t::do_write(stream);
    stream << push_option;
    stream << extend_data;
    const uint32_t data_count = (uint32_t)blocks_data.size();
    stream << data_count;
    for (auto& data : blocks_data) {
        stream << data;
    }
    return (stream.size() - begin_size);
}

int32_t xsync_msg_block_push_t::do_read(base::xstream_t& stream)
{
    const int32_t begin_size = stream.size();
    xsync_msg_block_t::do_read(stream);
    stream >> push_option;
    stream >> extend_data;
    uint32_t data_count = 0;
    stream >> data_count;
    for (uint32_t i = 0; i < data_count; i++) {
        std::string data;
        stream >> data;
        blocks_data.push_back(data);
    }
    return (begin_size - stream.size());
}

std::vector<data::xblock_ptr_t> xsync_msg_block_push_t::get_all_xblock_ptr() const
{
    return convert_stream_to_blocks(get_data_type(), blocks_data);
}


NS_END2
