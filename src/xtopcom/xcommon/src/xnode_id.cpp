// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stdint.h>
#include <cassert>
#include <iterator>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "xbasic/xbyte_buffer.h"
#include "xcommon/xnode_id.h"
#include "xbase/xmem.h"
#include "xbase/xns_macro.h"
#include "xcommon/xstring_id.hpp"
#include "xvledger/xvaccount.h"

NS_BEG2(top, common)

xtop_node_id::xtop_node_id(char const * v)
    : id_base_t{ v }
{
}

xtop_node_id::xtop_node_id(std::string v)
    : id_base_t{ std::move(v) }
{
}

void
xtop_node_id::swap(xtop_node_id & other) noexcept {
    id_base_t::swap(other);
}

bool
xtop_node_id::operator<(xtop_node_id const & other) const noexcept {
    return id_base_t::operator<(other);
}

bool
xtop_node_id::operator==(xtop_node_id const & other) const noexcept {
    return id_base_t::operator==(other);
}

bool
xtop_node_id::operator!=(xtop_node_id const & other) const noexcept {
    return id_base_t::operator!=(other);
}

void
xtop_node_id::random() {
    auto ranbytes = random_base58_bytes(33);
    m_id = "T-" + std::string{ std::begin(ranbytes), std::end(ranbytes) };
}

std::size_t
xtop_node_id::length() const noexcept {
    return this->m_id.length();
}

std::size_t
xtop_node_id::size() const noexcept {
    return this->m_id.size();
}

char const *
xtop_node_id::c_str() const noexcept {
    return this->m_id.c_str();
}

base::enum_vaccount_addr_type xtop_node_id::type() const noexcept {
    if (m_type == base::enum_vaccount_addr_type::enum_vaccount_addr_type_invalid) {
        m_type = base::xvaccount_t::get_addrtype_from_account(m_id);
    }
    assert(m_type != base::enum_vaccount_addr_type::enum_vaccount_addr_type_invalid);
    return m_type;
}

std::int32_t
xtop_node_id::do_read(base::xstream_t & stream) {
    auto const begin_size = stream.size();
    stream >> m_id;
    return begin_size - stream.size();
}

std::int32_t
xtop_node_id::do_write(base::xstream_t & stream) const {
    auto const begin_size = stream.size();
    stream << m_id;
    return stream.size() - begin_size;
}

std::int32_t
operator <<(top::base::xstream_t & stream, top::common::xnode_id_t const & node_id) {
    return node_id.serialize_to(stream);
}

std::int32_t
operator >>(top::base::xstream_t & stream, top::common::xnode_id_t & node_id) {
    return node_id.serialize_from(stream);
}

std::int32_t operator<<(top::base::xbuffer_t & buffer, top::common::xnode_id_t const & node_id) {
    return node_id.serialize_to(buffer);
}

std::int32_t operator>>(top::base::xbuffer_t & buffer, top::common::xnode_id_t & node_id) {
    return node_id.serialize_from(buffer);
}

std::ostream &
operator<<(std::ostream & o, xnode_id_t const & node_id) {
    if (node_id.empty()) {
        o << u8"(null)";
    } else {
        o << node_id.value();
    }
    return o;
}

NS_END2

NS_BEG1(std)

std::size_t
hash<top::common::xnode_id_t>::operator()(top::common::xnode_id_t const & id) const noexcept {
    return std::hash<top::common::xstring_id_t<top::common::xnode_id_t>>{}(id);
}

NS_END1
