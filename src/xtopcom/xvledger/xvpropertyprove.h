// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>

#include "xbasic/xversion.h"
#include "xvledger/xdataobj_base.hpp"
#include "xvledger/xprovecert.h"
#include "xvledger/xmerkle.hpp"
#include "xvledger/xvproperty.h"
#include "xvledger/xtxreceipt.h"
#include "xvledger/xvstate.h"

namespace top
{
    namespace base
    {
        class xvproperty_holder_t : public xvbstate_t {
        public:
            xvproperty_holder_t(const std::string & address, uint64_t block_height);
            
            bool    add_property(xvproperty_t* property);
        };

        class xvproperty_prove_t : public xbase_dataunit_t<xvproperty_prove_t, xdata_type_property_prove> {
        public:
            xvproperty_prove_t();
            xvproperty_prove_t(xvproperty_t* property, const xtx_receipt_ptr_t & tx_receipt);
        protected:            
            virtual ~xvproperty_prove_t();
        private:
            xvproperty_prove_t(const xvproperty_prove_t &);
            xvproperty_prove_t & operator = (const xvproperty_prove_t &);

            int32_t do_write(base::xstream_t & stream) override;
            int32_t do_read(base::xstream_t & stream) override;

        protected:
            static std::string              merkle_path_to_string(const base::xmerkle_path_256_t & path);
        public:
            bool                            is_valid() const;
            xauto_ptr<xmapvar_t<std::string>>   load_string_map_var(const std::string & property_name);
            const xtx_receipt_ptr_t &               get_action_prove() const {return m_receipt;}
            const xobject_ptr_t<xvproperty_t> &     get_property() const {return m_property;}
        private:
            xobject_ptr_t<xvproperty_t>     m_property{nullptr};
            base::xtx_receipt_ptr_t         m_receipt{nullptr};
        };

        using xvproperty_prove_ptr_t = xobject_ptr_t<xvproperty_prove_t>;

        class xpropertyprove_build_t {
        public:
            static xvproperty_prove_ptr_t         create_property_prove(xvblock_t* commit_block, xvblock_t* cert_block, xvbstate_t* bstate, const std::string & propname);
        };

    }  // namespace base
}  // namespace top
