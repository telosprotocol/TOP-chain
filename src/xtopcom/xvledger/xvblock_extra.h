// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xmem.h"
#include "xbasic/xserializable_based_on.h"

namespace top
{
    namespace base
    {
        class xtableheader_extra_t : public xserializable_based_on<void> {
        protected:
            enum xblockheader_extra_data_type : uint16_t {
                enum_extra_data_type_tgas_total_lock_amount_property_height = 0,
                enum_extra_data_type_tgas_second_level_gmtime               = 1,
                enum_extra_data_type_eth_header                             = 2,
                enum_extra_data_type_output_offdata_hash                    = 3,
                enum_extra_data_type_pledge_balance_change_tgas             = 4,
                enum_extra_data_type_burn_gas                               = 5,
            };
        protected:
            int32_t do_write(base::xstream_t & stream) const override;
            int32_t do_read(base::xstream_t & stream) override;
            inline std::string     get_value(xblockheader_extra_data_type type) const;
            inline void            set_value(xblockheader_extra_data_type type, std::string const& value);
        public:
            int32_t deserialize_from_string(const std::string & extra_data);
            int32_t serialize_to_string(std::string & extra_data);
            std::string dump() const;

        public:
            uint64_t get_tgas_total_lock_amount_property_height() const;
            void     set_tgas_total_lock_amount_property_height(uint64_t height);
            uint64_t get_second_level_gmtime() const;
            void     set_second_level_gmtime(uint64_t gmtime);
            std::string get_ethheader() const;
            void     set_ethheader(const std::string & value);
            std::string get_output_offdata_hash() const;
            void     set_output_offdata_hash(const std::string & value);
            std::string get_pledge_balance_change_tgas() const;
            void     set_pledge_balance_change_tgas(const std::string & value);        
            uint64_t get_total_burn_gas() const;
            void     set_total_burn_gas(uint64_t burn_gas);
        private:
            std::map<uint16_t, std::string>  m_paras;
        };

        // unit block header extra
        class xunit_header_extra_t {
        protected:
            enum extra_data_type : uint16_t {
                enum_extra_data_type_binlog         = 0,
                enum_extra_data_type_state_hash     = 1,
            };

        protected:
            int32_t do_write(base::xstream_t & stream) const;
            int32_t do_read(base::xstream_t & stream);
        public:
            int32_t deserialize_from_string(const std::string & extra_data);
            int32_t serialize_to_string(std::string & extra_data);
            std::string     get_value(uint16_t type) const;
            void            set_value(uint16_t type, std::string const& value);
            
            void        set_binlog(std::string const & value) {set_value(enum_extra_data_type_binlog, value);}
            std::string get_binlog() const {return get_value(enum_extra_data_type_binlog);}
            void        set_state_hash(std::string const & value) {set_value(enum_extra_data_type_state_hash, value);}
            std::string get_state_hash() const {return get_value(enum_extra_data_type_state_hash);}
        private:
            std::map<uint16_t, std::string>  m_paras;
        };

    }//end of namespace of base

}//end of namespace top
