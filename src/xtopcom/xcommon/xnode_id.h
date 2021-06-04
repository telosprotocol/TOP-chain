// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wpedantic"
#elif defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wpedantic"
#elif defined(_MSC_VER)
#    pragma warning(push, 0)
#endif

#include "xbase/xmem.h"
#include "xvledger/xvaccount.h"

#if defined(__clang__)
#    pragma clang diagnostic pop
#elif defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#    pragma warning(pop)
#endif

#include "xbasic/xhashable.hpp"
#include "xbasic/xrandomizable.h"
#include "xbasic/xserializable_based_on.h"
#include "xcommon/xstring_id.hpp"

#include <ostream>

NS_BEG2(top, common)
class xtop_node_id;

std::int32_t
operator <<(top::base::xstream_t & stream, top::common::xtop_node_id const & node_id);

std::int32_t
operator >>(top::base::xstream_t & stream, top::common::xtop_node_id & node_id);

std::int32_t operator<<(top::base::xbuffer_t & stream, top::common::xtop_node_id const & node_id);

std::int32_t operator>>(top::base::xbuffer_t & stream, top::common::xtop_node_id & node_id);

class xtop_node_id final : public xstring_id_t<xtop_node_id>
                         , public xrandomizable_t<xtop_node_id>
                         , public xserializable_based_on<void>
{
private:
    using id_base_t = xstring_id_t<xtop_node_id>;

    mutable base::enum_vaccount_addr_type m_type{base::enum_vaccount_addr_type_invalid};

public:
    xtop_node_id()                                 = default;
    xtop_node_id(xtop_node_id const &)             = default;
    xtop_node_id & operator=(xtop_node_id const &) = default;
    xtop_node_id(xtop_node_id &&)                  = default;
    xtop_node_id & operator=(xtop_node_id &&)      = default;
    ~xtop_node_id() override                       = default;

    explicit
    xtop_node_id(std::string value);

    explicit
    xtop_node_id(char const * value);

    using id_base_t::empty;
    using id_base_t::has_value;
    using id_base_t::value;
    using id_base_t::hash;

    using id_base_t::operator bool;

    void
    swap(xtop_node_id & other) noexcept;

    bool
    operator<(xtop_node_id const & other) const noexcept;

    bool
    operator==(xtop_node_id const & other) const noexcept;

    bool
    operator!=(xtop_node_id const & other) const noexcept;

    void
    random() override;

    std::size_t
    length() const noexcept;

    std::size_t
    size() const noexcept;

    char const *
    c_str() const noexcept;

    base::enum_vaccount_addr_type type() const noexcept;

    friend std::int32_t operator <<(base::xstream_t & stream, xtop_node_id const & node_id);
    friend std::int32_t operator >>(base::xstream_t & stream, xtop_node_id & node_id);
    friend std::int32_t operator<<(base::xbuffer_t & stream, xtop_node_id const & node_id);
    friend std::int32_t operator>>(base::xbuffer_t & stream, xtop_node_id & node_id);

    std::int32_t serialize_to(base::xstream_t & stream) const override;
    std::int32_t serialize_from(base::xstream_t & stream) override;
    std::int32_t serialize_to(base::xbuffer_t & buffer) const override;
    std::int32_t serialize_from(base::xbuffer_t & buffer) override;
private:
    std::int32_t
    do_read(base::xstream_t & stream) override;

    std::int32_t
    do_write(base::xstream_t & stream) const override;
};

using xnode_id_t = xtop_node_id;

NS_END2

NS_BEG1(std)

template <>
struct hash<top::common::xnode_id_t> final
{
    std::size_t
    operator()(top::common::xnode_id_t const & id) const noexcept;
};

NS_END1
