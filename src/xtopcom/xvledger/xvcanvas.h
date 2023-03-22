// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
 
#include <mutex>
#include "xbase/xvmethod.h"
#include "xstatistic/xstatistic.h"

namespace top
{
    namespace base
    {
        //xvcanvas_t is the execution context that recording and compile to bin-log
        //xvcanvas_t has own mutex so it is multiple-thread safe
        class xvcanvas_t : public xrefcount_t, public xstatistic::xstatistic_obj_face_t
        {
        public:
            enum enum_compile_optimization
            {
                enum_compile_optimization_none = 0x00,
                enum_compile_optimization_all  = 0x0F,
                enum_compile_optimization_mask = 0x0F,
            };
            enum {enum_max_binlog_size = 536870912}; //1 << 29 = 536870912 = 512MB
        public://compile & decompile then encode/decode to certain format,encode_options = enum_compile_optimization | enum_compress_optimization
            static const int  decode_from(xstream_t & input_bin,const uint32_t bin_size,std::deque<xvmethod_t> & output_records);
            static const int  decode_from(const std::string & input_bin,std::deque<xvmethod_t> & output_records);
            
        private://compile and decompile between records and bin-log
            static const int  encode_to(const std::deque<xvmethod_t> & input_records,const int compile_options,std::string & output_bin);
            static const int  encode_to(const std::deque<xvmethod_t> & input_records,const int compile_options,xstream_t & output_bin);
            static const int  compile(const std::deque<xvmethod_t> & input_records,const int compile_options,xstream_t & output_stream);
            static const int  decompile(xstream_t & input_stream,std::deque<xvmethod_t> & output_records);
            
        public:
            xvcanvas_t();
            xvcanvas_t(const xvcanvas_t & obj);
            xvcanvas_t(const std::string & bin_log);//de-compile bin-log to xvmethod_t instructions
        protected:
            virtual ~xvcanvas_t();
        private:
            xvcanvas_t(xvcanvas_t &&);
            xvcanvas_t & operator = (const xvcanvas_t & obj);
            
        public:
            bool       record(const xvmethod_t & op);//record instruction
            bool       rollback(size_t height);
            std::deque<xvmethod_t>  clone();
            
            const int  encode(xstream_t & output_bin,const int compile_options = xvcanvas_t::enum_compile_optimization_all);
            const int  encode(std::string & output_bin,const int compile_options = xvcanvas_t::enum_compile_optimization_all);
 
            size_t     get_op_records_size() const {return m_records.size();}

            virtual int32_t get_class_type() const override {return xstatistic::enum_statistic_vcanvas;}
        private:
            virtual size_t get_object_size_real() const override;
            
        public://debug purpose only
            void              log();
            void              print();
            const std::string dump();
            void              dump(std::string & full_content);
        protected:
            const std::deque<xvmethod_t> & get_op_records() const {return m_records;}
        private:
            std::recursive_mutex       m_lock;  // TODO(jimmy) no need lock?
            std::deque<xvmethod_t>     m_records;
        };
        
    }//end of namespace of base

}//end of namespace top
