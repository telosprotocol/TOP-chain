// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <memory>
#include <vector>
#include <map>
#include "xbase/xns_macro.h"
#include "xbase/xcontext.h"
#include "xbase/xmem.h"

NS_BEG1(top)

using namespace top::base;

class xstream_util_t {
public:

    template<typename T>
    static T from_string(std::string& str) {
        T value;
        xstream_t stream(xcontext_t::instance(), (uint8_t*) str.data(), str.size());
        stream >> value;
        return value;
    }

    template<typename T>
    static T from_string_with_read(std::string& str) {
        T value;
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t*) str.data(), str.size());
        value.do_read(stream);
        return value;
    }

    template<typename T>
    static void from_string_with_read(std::string& str, T& value) {
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t*) str.data(), str.size());
        value.do_read(stream);
    }

    template<typename T>
    static std::string to_string_with_write(T& value) {
        base::xstream_t stream(base::xcontext_t::instance());
        value.do_write(stream);
        return std::string((char*) stream.data(), stream.size());
    }
    
    template<typename ..._Args>
    static std::string to_string(_Args&& ...args) {
        xstream_t stream(xcontext_t::instance());
        do_write(stream, args...);
        return std::string((char*) stream.data(), stream.size());
    }

    template<typename ..._Args>
    static int32_t from_string(const std::string& str, _Args& ...args) {
        xstream_t stream(xcontext_t::instance(),
                (uint8_t*) str.data(), str.size());
        return do_read(stream, args...);
    }
    
    template<typename ..._Args>
    static uint32_t do_write(xstream_t& stream, _Args&& ...args) {
        uint32_t size = 0;
        for (auto& s : {(stream << args)...}) {
            size += s;
        }
        return size;
    }
    
    template<typename ..._Args> 
    static uint32_t do_read(xstream_t& stream, _Args& ...args) {
        uint32_t size = 0;
        for(auto& s : {(stream >> args)...}) {
            size += s;
        }
        return size;
    }
    
    inline
    static std::string stream_to_str(xstream_t& stream) {
        return std::string((char*) stream.data(), stream.size());
    }
    
    template<typename EnumT>
    static uint32_t do_read_enum(xstream_t& stream, EnumT& value) {
        typename std::underlying_type<EnumT>::type temp;
        stream >> temp;
        value = static_cast<EnumT>(temp);
        return sizeof(temp);
    }
    
    template<typename EnumT>
    static uint32_t do_write_enum(xstream_t& stream, EnumT value) {
        stream << static_cast<typename std::underlying_type<EnumT>::type>(value);
        return sizeof(typename std::underlying_type<EnumT>::type);
    }
    
    template<typename T>
    static uint32_t do_read(xstream_t& stream, std::vector<T>& list) {
        uint32_t size = stream.size();
        uint32_t count = 0;
        stream >> count;
        for(uint32_t i=0;i<count;i++) {
            T t;
            t.do_read(stream);
            list.push_back(t);
        }
        return size - stream.size();
    }
    
    template<typename T>
    static uint32_t do_write(xstream_t& stream, const std::vector<T>& list) {
        uint32_t size = stream.size();
        uint32_t count = list.size();
        stream << count;
        for(auto& t : list) {
            t.do_write(stream);
        }
        return stream.size() - size;
    }
    
    template<typename Tk, typename Tv>
    static uint32_t do_read(xstream_t& stream, std::map<Tk, Tv>& map) {
        uint32_t size = stream.size();
        uint32_t count = 0;
        stream >> count;
        for(uint32_t i = 0;i<count;i++) {
            std::pair<Tk, Tv>  _item;
            stream >> _item.first;
            _item.second.do_read(stream);
            map.emplace(_item);
        }
        
        return size - stream.size();
    }
    
    template<typename Tk, typename Tv>
    static uint32_t do_write(xstream_t& stream, const std::map<Tk, Tv>& map) {
        uint32_t size = stream.size();
        uint32_t count = map.size();
        stream << count;
        for(auto& pair : map) {
            stream << pair.first;
            pair.second.do_write(stream);
        }
        
        return stream.size() - size;
    }
};

NS_END1
