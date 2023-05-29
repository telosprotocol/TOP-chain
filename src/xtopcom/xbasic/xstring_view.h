// Copyright (c) 2022-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

#if defined(XCXX17)

#include <string_view>

NS_BEG1(top)

using xstring_view_t = std::string_view;

NS_END1

#else

#include <cassert>
#include <cstddef>
#include <string>
#include <iterator>
#include <stdexcept>
#include <limits>

NS_BEG1(top)

template <typename CharT, typename TraitsT = std::char_traits<CharT>>
class xtop_basic_string_view;

template <typename CharT, typename TraitsT = std::char_traits<CharT>>
using xbasic_string_view_t = xtop_basic_string_view<CharT, TraitsT>;

NS_BEG1(details)

template <class TraitsT>
class xtop_string_view_iterator {
public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = typename TraitsT::char_type;
    using difference_type = ptrdiff_t;
    using pointer = value_type const *;
    using reference = value_type const &;

    constexpr xtop_string_view_iterator() noexcept = default;

private:
    pointer offset_{nullptr};

    friend xbasic_string_view_t<value_type, TraitsT>;

    constexpr xtop_string_view_iterator(pointer const offset) noexcept : offset_(offset) {
    }

public:
    XATTRIBUTE_NODISCARD constexpr reference operator*() const noexcept {
        return *offset_;
    }

    XATTRIBUTE_NODISCARD constexpr pointer operator->() const noexcept {
        return offset_;
    }

    xtop_string_view_iterator & operator++() noexcept {
        ++offset_;
        return *this;
    }

    xtop_string_view_iterator operator++(int) noexcept {
        xtop_string_view_iterator tmp{*this};
        ++*this;
        return tmp;
    }

    xtop_string_view_iterator & operator--() noexcept {
        --offset_;
        return *this;
    }

    xtop_string_view_iterator operator--(int) noexcept {
        xtop_string_view_iterator tmp{*this};
        --*this;
        return tmp;
    }

    xtop_string_view_iterator & operator+=(difference_type const offset) noexcept {
        offset_ += static_cast<size_t>(offset);
        return *this;
    }

    XATTRIBUTE_NODISCARD xtop_string_view_iterator operator+(difference_type const offset) const noexcept {
        xtop_string_view_iterator tmp{*this};
        tmp += offset;
        return tmp;
    }

    XATTRIBUTE_NODISCARD friend xtop_string_view_iterator operator+(difference_type const offset, xtop_string_view_iterator right) noexcept {
        right += offset;
        return right;
    }

    xtop_string_view_iterator & operator-=(difference_type const offset) noexcept {
        offset_ -= static_cast<size_t>(offset);

        return *this;
    }

    XATTRIBUTE_NODISCARD xtop_string_view_iterator operator-(difference_type const offset) const noexcept {
        xtop_string_view_iterator tmp{*this};
        tmp -= offset;
        return tmp;
    }

    XATTRIBUTE_NODISCARD difference_type operator-(xtop_string_view_iterator const & right) const noexcept {
        return static_cast<difference_type>(offset_ - right.offset_);
    }

    XATTRIBUTE_NODISCARD reference operator[](difference_type const offset) const noexcept {
        return *(*this + offset);
    }

    XATTRIBUTE_NODISCARD constexpr bool operator==(xtop_string_view_iterator const & right) const noexcept {
        return offset_ == right.offset_;
    }

    XATTRIBUTE_NODISCARD constexpr bool operator!=(xtop_string_view_iterator const & right) const noexcept {
        return !(*this == right);
    }

    XATTRIBUTE_NODISCARD constexpr bool operator<(xtop_string_view_iterator const & right) const noexcept {
        return offset_ < right.offset_;
    }

    XATTRIBUTE_NODISCARD constexpr bool operator>(xtop_string_view_iterator const & right) const noexcept {
        return right < *this;
    }

    XATTRIBUTE_NODISCARD constexpr bool operator<=(xtop_string_view_iterator const & right) const noexcept {
        return !(right < *this);
    }

    XATTRIBUTE_NODISCARD constexpr bool operator>=(xtop_string_view_iterator const & right) const noexcept {
        return !(*this < right);
    }
};

template <typename TraitsT>
using xstring_view_iterator_t = xtop_string_view_iterator<TraitsT>;

NS_END1

template <typename CharT, class TraitsT>
class xtop_basic_string_view {
public:
    using traits_type            = TraitsT;
    using value_type             = CharT;
    using pointer                = CharT *;
    using const_pointer          = CharT const *;
    using reference              = CharT &;
    using const_reference        = CharT const &;
    using const_iterator         = details::xstring_view_iterator_t<TraitsT>;
    using iterator               = const_iterator;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using reverse_iterator       = const_reverse_iterator;
    using size_type              = size_t;
    using difference_type        = ptrdiff_t;

private:
    const_pointer data_{nullptr};
    size_type size_{0};

public:

    static constexpr size_type npos{std::numeric_limits<size_type>::max()};

    constexpr xtop_basic_string_view() = default;
    xtop_basic_string_view(xtop_basic_string_view const &) = default;
    xtop_basic_string_view & operator=(xtop_basic_string_view const &) = default;
    xtop_basic_string_view(xtop_basic_string_view &&) = default;
    xtop_basic_string_view & operator=(xtop_basic_string_view &&) = default;
    ~xtop_basic_string_view() = default;

    xtop_basic_string_view(std::basic_string<CharT, TraitsT> const & str) noexcept : data_{str.data()}, size_{str.size()} {
    }

    xtop_basic_string_view(const_pointer const cstr) noexcept : data_{cstr}, size_{TraitsT::length(cstr)} {
    }

    xtop_basic_string_view(const_pointer const cstr, size_type const size) noexcept : data_{cstr}, size_{size} {
        assert(cstr);
    }


    XATTRIBUTE_NODISCARD const_iterator begin() const noexcept {
        return const_iterator{data_};
    }

    XATTRIBUTE_NODISCARD const_iterator end() const noexcept {
        return const_iterator{data_ + size_};
    }

    XATTRIBUTE_NODISCARD constexpr const_iterator cbegin() const noexcept {
        return begin();
    }

    XATTRIBUTE_NODISCARD constexpr const_iterator cend() const noexcept {
        return end();
    }

    XATTRIBUTE_NODISCARD constexpr const_reverse_iterator rbegin() const noexcept {
        return const_reverse_iterator{end()};
    }

    XATTRIBUTE_NODISCARD constexpr const_reverse_iterator rend() const noexcept {
        return const_reverse_iterator{begin()};
    }

    XATTRIBUTE_NODISCARD constexpr const_reverse_iterator crbegin() const noexcept {
        return rbegin();
    }

    XATTRIBUTE_NODISCARD constexpr const_reverse_iterator crend() const noexcept {
        return rend();
    }

    XATTRIBUTE_NODISCARD constexpr size_type size() const noexcept {
        return size_;
    }

    XATTRIBUTE_NODISCARD constexpr size_type length() const noexcept {
        return size_;
    }

    XATTRIBUTE_NODISCARD constexpr bool empty() const noexcept {
        return size_ == 0;
    }

    XATTRIBUTE_NODISCARD constexpr const_pointer data() const noexcept {
        return data_;
    }


    XATTRIBUTE_NODISCARD const_reference operator[](size_type const offset) const noexcept {
        assert(offset < size_);
        return data_[offset];
    }

    XATTRIBUTE_NODISCARD const_reference at(size_type const offset) const {
        check_offset_exclusive(offset);
        return data_[offset];
    }

    XATTRIBUTE_NODISCARD
#if defined(XCXX14)
    constexpr
#endif
    const_reference front() const noexcept {
        assert(size_ != 0);
        return data_[0];
    }

    XATTRIBUTE_NODISCARD constexpr const_reference back() const noexcept {
        assert(size_ != 0);
        return data_[size_ - 1];
    }

    void remove_prefix(size_type const size) noexcept {
        assert(size_ >= size);
        data_ += size;
        size_ -= size;
    }

    void remove_suffix(size_type const size) noexcept {
        assert(size_ >= size);
        size_ -= size;
    }

    void swap(xtop_basic_string_view & other) noexcept {
        xtop_basic_string_view const tmp{other};
        other = *this;
        *this = tmp;
    }

    size_type copy(value_type * const ptr, size_type size, size_type const offset = 0) const {
        // copy [offset, offset + Count) to [ptr, ptr + size)
        check_offset(offset);
        size = clamp_suffix_size(offset, size);
        TraitsT::copy(ptr, data_ + offset, size);
        return size;
    }

    XATTRIBUTE_NODISCARD xtop_basic_string_view substr(size_type const offset = 0, size_type size = npos) const {
        check_offset(offset);
        size = clamp_suffix_size(offset, size);
        return xtop_basic_string_view{data_ + offset, size};
    }

    XATTRIBUTE_NODISCARD int compare(xtop_basic_string_view const right) const noexcept {
        const int result = TraitsT::compare(data_, right.data_, std::min(size_, right.size_));

        if (result != 0) {
            return result;
        }

        if (size_ < right.size_) {
            return -1;
        }

        if (size_ > right.size_) {
            return 1;
        }

        return 0;
    }

    XATTRIBUTE_NODISCARD constexpr int compare(size_type const pos1, size_type const count1, xtop_basic_string_view const other) const {
        // compare [pos1, pos1 + count1) with other
        return substr(pos1, count1).compare(other);
    }

    XATTRIBUTE_NODISCARD constexpr int compare(size_type const pos1,
                                               size_type const count1,
                                               xtop_basic_string_view const other,
                                               size_type const pos2,
                                               size_type const count2) const {
        // compare [pos1, pos1 + count1) with other [pos2, pos2 + count2)
        return substr(pos1, count1).compare(other.substr(pos2, count2));
    }

    XATTRIBUTE_NODISCARD constexpr int compare(value_type const * const cstr) const {
        return compare(xtop_basic_string_view{cstr});
    }

    XATTRIBUTE_NODISCARD constexpr int compare(size_type const pos1, size_type const count1, value_type const * const cstr) const {
        // compare [pos1, pos1 + count1) with [cstr, <null>)
        return substr(pos1, count1).compare(xtop_basic_string_view(cstr));
    }

    XATTRIBUTE_NODISCARD constexpr int compare(size_type const pos1, size_type const count1, value_type const * const cstr, size_type const count2) const {
        // compare [pos1, pos1 + count1) with [cstr, cstr + count2)
        return substr(pos1, count1).compare(xtop_basic_string_view(cstr, count2));
    }

    XATTRIBUTE_NODISCARD bool starts_with(xtop_basic_string_view const other) const noexcept {
        if (size_ < other.size_) {
            return false;
        }
        return TraitsT::compare(data_, other.data_, other.size_) == 0;
    }

    XATTRIBUTE_NODISCARD constexpr bool starts_with(value_type const other) const noexcept {
        return !empty() && TraitsT::eq(front(), other);
    }

    XATTRIBUTE_NODISCARD constexpr bool starts_with(value_type const * const cstr) const noexcept {
        return starts_with(xtop_basic_string_view{cstr});
    }

    XATTRIBUTE_NODISCARD bool ends_with(xtop_basic_string_view const other) const noexcept {
        if (size_ < other.size_) {
            return false;
        }
        return TraitsT::compare(data_ + (size_ - other.size_), other.data_, other.size_) == 0;
    }

    XATTRIBUTE_NODISCARD constexpr bool ends_with(value_type const other) const noexcept {
        return !empty() && TraitsT::eq(back(), other);
    }

    XATTRIBUTE_NODISCARD constexpr bool ends_with(value_type const * const other) const noexcept {
        return ends_with(xtop_basic_string_view(other));
    }

    XATTRIBUTE_NODISCARD constexpr bool contains(xtop_basic_string_view const other) const noexcept {
        return find(other) != npos;
    }

    XATTRIBUTE_NODISCARD constexpr bool contains(value_type const other) const noexcept {
        return find(other) != npos;
    }

    XATTRIBUTE_NODISCARD constexpr bool contains(value_type const * const other) const noexcept {
        return find(other) != npos;
    }

    //XATTRIBUTE_NODISCARD constexpr size_type find(xtop_basic_string_view const other, size_type const offset = 0) const noexcept {
    //}

    //XATTRIBUTE_NODISCARD constexpr size_type find(value_type const ch, size_type const offset = 0) const noexcept {
    //}

    //XATTRIBUTE_NODISCARD constexpr size_type find(value_type const * const cstr, size_type const offset, size_type const size) const noexcept {
    //}

    //XATTRIBUTE_NODISCARD constexpr size_type find(value_type const * const cstr, size_type const offset = 0) const noexcept
    //{
    //}

    //XATTRIBUTE_NODISCARD constexpr size_type rfind(xtop_basic_string_view const other, size_type const offset = npos) const noexcept {
    //}

    //XATTRIBUTE_NODISCARD constexpr size_type rfind(value_type const ch, size_type const offset = npos) const noexcept {
    //}

    //XATTRIBUTE_NODISCARD constexpr size_type rfind(value_type const * const cstr, size_type const offset, size_type const size) const noexcept {
    //}

    //XATTRIBUTE_NODISCARD constexpr size_type rfind(value_type const * const cstr, size_type const offset = npos) const noexcept
    //{
    //}

    //XATTRIBUTE_NODISCARD constexpr size_type find_first_of(xtop_basic_string_view const other, size_type const offset = 0) const noexcept {
    //}

    XATTRIBUTE_NODISCARD size_type find_first_of(value_type const ch, size_type const offset = 0) const noexcept {
        for (size_type i = offset; i < size(); ++i) {
            if (at(i) == ch) {
                return i;
            }
        }

        return npos;
    }

    //XATTRIBUTE_NODISCARD constexpr size_type find_first_of(value_type const * const cstr, size_type const offset, size_type const size) const noexcept {
    //}

    //XATTRIBUTE_NODISCARD constexpr size_type find_first_of(value_type const * const cstr, size_type const offset = 0) const noexcept {
    //}

    //XATTRIBUTE_NODISCARD constexpr size_type find_last_of(xtop_basic_string_view const other, size_type const offset = npos) const noexcept {
    //}

    //XATTRIBUTE_NODISCARD constexpr size_type find_last_of(value_type const ch, size_type const offset = npos) const noexcept {
    //}

    //XATTRIBUTE_NODISCARD constexpr size_type find_last_of(value_type const * const cstr, size_type const offset, size_type const size) const noexcept {
    //}

    //XATTRIBUTE_NODISCARD constexpr size_type find_last_of(value_type const * const cstr, size_type const offset = npos) const noexcept {
    //}

    //XATTRIBUTE_NODISCARD constexpr size_type find_first_not_of(xtop_basic_string_view const other, size_type const offset = 0) const noexcept {
    //}

    //XATTRIBUTE_NODISCARD constexpr size_type find_first_not_of(value_type const ch, size_type const offset = 0) const noexcept {
    //}

    //XATTRIBUTE_NODISCARD constexpr size_type find_first_not_of(value_type const * const cstr, size_type const offset, size_type const size) const noexcept {
    //}

    //XATTRIBUTE_NODISCARD constexpr size_type find_first_not_of(value_type const * const cstr, size_type const offset = 0) const noexcept {
    //}

    //XATTRIBUTE_NODISCARD constexpr size_type find_last_not_of(xtop_basic_string_view const other, size_type const offset = npos) const noexcept {
    //}

    //XATTRIBUTE_NODISCARD constexpr size_type find_last_not_of(value_type const ch, size_type const offset = npos) const noexcept {
    //}

    //XATTRIBUTE_NODISCARD constexpr size_type find_last_not_of(value_type const * const cstr, size_type const offset, size_type const size) const noexcept {
    //}

    //XATTRIBUTE_NODISCARD constexpr size_type find_last_not_of(value_type const * const cstr, size_type const offset = npos) const noexcept {
    //}

private:
    void check_offset(size_type const offset) const {  // checks whether offset is in the bounds of [0, size()]
        if (size_ < offset) {
            throw std::out_of_range{"invalid xstring_view_t position"};
        }
    }

    void check_offset_exclusive(size_type const offset) const {
        // checks whether offset is in the bounds of [0, size())
        if (size_ <= offset) {
            throw std::out_of_range{"invalid xstring_view_t position"};
        }
    }

    constexpr size_type clamp_suffix_size(size_type const offset, size_type const size) const noexcept {
        return std::min(size, size_ - offset);
    }
};

template <typename CharT, typename TraistsT>
constexpr typename xtop_basic_string_view<CharT, TraistsT>::size_type xtop_basic_string_view<CharT, TraistsT>::npos;

using xstring_view_t = xbasic_string_view_t<char>;

NS_END1

#endif
