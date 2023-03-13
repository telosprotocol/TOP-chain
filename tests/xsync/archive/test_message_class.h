
#pragma once
#include <string>
#include<random>
#include "xbase/xmem.h"

class message_class_test {
public:

    message_class_test() {}
    message_class_test(uint64_t _max_size)
        : m_max_size(_max_size)
    {
        std::random_device rd;
        std::default_random_engine random(rd());

        for (uint64_t len = 0; len < _max_size; len++) {
            char c = 'a' + rand() % 26;
            m_string_a.push_back(c);
            c = 'A' + rand() % 26;
            m_string_b.push_back(c);
        }
    }

    ~message_class_test() {}
public:
    const uint64_t get_max_size() const{ return m_max_size; }
    const std::string& get_string_a() const { return m_string_a; }
    const std::string& get_string_b() const { return m_string_b; }

    void set_max_size(uint64_t _max_size) { m_max_size = _max_size; }
    void set_string_a(const std::string& _str) { m_string_a = std::move(_str); }
    void set_string_b(const std::string& _str) { m_string_b = std::move(_str); }

    int32_t do_write(top::base::xstream_t& stream) 
    {

        int32_t _start = stream.size();
        stream << m_max_size;
        stream << m_string_a;
        stream << m_string_b;

        return (stream.size() - _start);
    }
    int32_t do_read(top::base::xstream_t& stream) 
    {
        int32_t _start = stream.size();
        stream >> m_max_size;
        stream >> m_string_a;
        stream >> m_string_b;

        return (stream.size() - _start);
    }

    int32_t serialize_from_new(xstream_t& stream)
    {
        return do_read(stream);
    }

    int32_t serialize_to_new(xstream_t& stream)
    {
        return do_write(stream);
    }

private:
    uint64_t     m_max_size { 0 };
    std::string  m_string_a {};
    std::string  m_string_b {};

};
