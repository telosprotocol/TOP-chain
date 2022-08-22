// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xdata.h"
#include "xvledger/xvblock_offdata.h"

namespace top
{
    namespace base
    {
        //---------------------------------xunitblock_t---------------------------------//




        //---------------------------------xvblock_out_offdata_t---------------------------------//
        xvblock_out_offdata_t::xvblock_out_offdata_t(const std::vector<xobject_ptr_t<xvblock_t>> & subblocks)
        : m_subblocks(subblocks) {
        }

        int32_t xvblock_out_offdata_t::serialize_to_string(std::string & str) const {
            base::xstream_t _stream(base::xcontext_t::instance());
            auto size = do_write(_stream);
            str.clear();
            str.assign((const char*)_stream.data(), _stream.size());
            return str.size();
        }

        int32_t xvblock_out_offdata_t::do_write(base::xstream_t & _stream) const {
            const int32_t begin_size = _stream.size();

            uint32_t num = m_subblocks.size();
            _stream.write_compact_var(num);
            for (auto & block :m_subblocks) {
                std::string block_object_str;
                block->serialize_to_string(block_object_str);
                _stream.write_compact_var(block_object_str);
                if (block->get_header()->get_block_class() != base::enum_xvblock_class_nil) {
                    _stream.write_compact_var(block->get_input()->get_resources_data());
                    _stream.write_compact_var(block->get_output()->get_resources_data());
                }
            }

            return (_stream.size() - begin_size);
        }

        int32_t xvblock_out_offdata_t::serialize_from_string(const std::string & _data) {
            base::xstream_t _stream(base::xcontext_t::instance(),(uint8_t*)_data.data(),(uint32_t)_data.size());
            const int result = do_read(_stream);
            return result;
        }

        int32_t xvblock_out_offdata_t::do_read(base::xstream_t & _stream) {
            const int32_t begin_size = _stream.size();
            uint32_t subblock_num = 0;
            _stream.read_compact_var(subblock_num);
            for (uint32_t i = 0; i < subblock_num; ++i) {
                std::string block_object_str;
                _stream.read_compact_var(block_object_str);
                base::xvblock_t* new_block = base::xvblock_t::create_block_object(block_object_str);
                if (nullptr == new_block) {
                    xassert(false);
                    return -1;
                }
                xobject_ptr_t<base::xvblock_t> block_ptr = nullptr;
                block_ptr.attach(new_block);

                if (block_ptr->get_block_class() != base::enum_xvblock_class_nil) {
                    std::string input_resource_str;
                    std::string output_resource_str;
                    _stream.read_compact_var(input_resource_str);
                    _stream.read_compact_var(output_resource_str);
                    if (false == block_ptr->set_input_resources(input_resource_str)) {
                        xassert(false);
                        return -1;
                    }
                    if (false == block_ptr->set_output_resources(output_resource_str)) {
                        xassert(false);
                        return -1;
                    }
                }
                m_subblocks.push_back(block_ptr);
            }
            return (begin_size - _stream.size());
        }

        void    xvblock_out_offdata_t::set_subblocks(std::vector<xobject_ptr_t<xvblock_t>> subblocks) {
            m_subblocks = subblocks;
        }

    }//end of namespace of base

}//end of namespace top
