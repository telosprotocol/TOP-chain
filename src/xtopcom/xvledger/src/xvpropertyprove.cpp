// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>

#include "xbasic/xversion.h"
#include "xvledger/xdataobj_base.hpp"
#include "xvledger/xprovecert.h"
#include "xvledger/xvpropertyprove.h"
#include "xvledger/xtxreceipt.h"
#include "xutility/xhash.h"

namespace top
{
    namespace base
    {
        xvproperty_holder_t::xvproperty_holder_t(const std::string & address, uint64_t block_height)
        : base::xvbstate_t(address, block_height, (uint64_t)1, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0) {

        }

        bool xvproperty_holder_t::add_property(xvproperty_t* property) {
            return add_child_unit(property);
        }



        xvproperty_prove_t::xvproperty_prove_t() {

        }

        xvproperty_prove_t::xvproperty_prove_t(xvproperty_t* property, const xtx_receipt_ptr_t & tx_receipt) {
            property->add_ref();
            m_property.attach(property);
            m_receipt = tx_receipt;
        }

        xvproperty_prove_t::~xvproperty_prove_t() {
            
        }

        int32_t xvproperty_prove_t::do_write(base::xstream_t & stream) {
            const int32_t begin_size = stream.size();
            uint8_t has_property = m_property != nullptr;
            stream << has_property;
            if (m_property != nullptr) {
                // std::string value_bin;                
                // m_property->serialize_to_string(value_bin);
                // stream.write_compact_var(value_bin);
                m_property->serialize_to(stream);
            }
            uint8_t has_receipt = m_receipt != nullptr;
            stream << has_receipt;
            if (m_receipt != nullptr) {
                m_receipt->serialize_to(stream);
            }
            return (stream.size() - begin_size);
        }
        int32_t xvproperty_prove_t::do_read(base::xstream_t & stream) {
            const int32_t begin_size = stream.size();
            uint8_t has_property;
            stream >> has_property;
            if (has_property) {
                // std::string value_bin;
                // stream.read_compact_var(value_bin);
                // m_property = make_object_ptr<xvproperty_t>();
                // m_property->serialize_from_string(value_bin);
                base::xdataunit_t* _dataunit = (base::xvproperty_t*)base::xdataunit_t::read_from(stream);
                xassert(_dataunit != nullptr);
                base::xvproperty_t* _property = dynamic_cast<base::xvproperty_t*>(_dataunit);
                xassert(_property != nullptr);
                m_property.attach(_property);
            }
            uint8_t has_receipt;
            stream >> has_receipt;
            if (has_receipt) {
                base::xtx_receipt_t* _receipt = (base::xtx_receipt_t*)base::xdataunit_t::read_from(stream);
                xassert(_receipt != nullptr);
                m_receipt.attach(_receipt);
            }
            return (begin_size - stream.size());
        }

        bool xvproperty_prove_t::is_valid() const {
            if (m_receipt == nullptr || m_property == nullptr) {
                xerror("xvproperty_prove_t::is_valid prove cert or property is null");
                return false;
            }

            std::string _merkle_leaf;
            m_property->serialize_to_string(_merkle_leaf);
            uint256_t hash = utl::xsha2_256_t::digest(_merkle_leaf);
            XMETRICS_GAUGE(metrics::cpu_hash_256_xvproperty_prove_t_leafs_calc, 1);
            std::string prophash = std::string(reinterpret_cast<char*>(hash.data()), hash.size());
            std::string propname = m_property->get_name();

            std::string prophash2 = m_receipt->get_tx_result_property(propname);
            if (prophash != prophash2) {
                xerror("xvproperty_prove_t::is_valid property hash not match.name=%s", propname.c_str());
                return false;
            }

            if (!m_receipt->is_valid()) {
                xwarn("xvproperty_prove_t::is_valid receipt check fail");
                return false;
            }
            return true;
        }

        xauto_ptr<xmapvar_t<std::string>> xvproperty_prove_t::load_string_map_var(const std::string & property_name) {
            xmapvar_t<std::string>* var_obj = (xmapvar_t<std::string>*)m_property->query_interface(enum_xobject_type_vprop_string_map);
            xassert(var_obj != nullptr);
            if(var_obj != nullptr)
                var_obj->add_ref();//for returned xauto_ptr
            return var_obj;
        }



        //----------------------------------------xpropertyprove_build_t-------------------------------------//
        xvproperty_prove_ptr_t    xpropertyprove_build_t::create_property_prove(xvblock_t* commit_block, xvblock_t* cert_block, xvbstate_t* bstate, const std::string & propname) {
            xtx_receipt_ptr_t txreceipt = xtxreceipt_build_t::create_table_input_primary_action_receipt(commit_block, cert_block);
            if (txreceipt == nullptr) {
                xassert(false);
                return nullptr;
            }
            xassert(commit_block->get_height() == bstate->get_block_height());
            std::string prophash = txreceipt->get_tx_result_property(propname);
            if (prophash.empty()) {
                xwarn("xpropertyprove_build_t::create_property_prove has no property");
                return nullptr;                
            }
            auto _property = bstate->load_property(propname);
            if (_property == nullptr) {
                xassert(false);
                return nullptr;
            }
            #ifdef DEBUG
            std::string property_bin;
            _property->serialize_to_string(property_bin);
            uint256_t hash = utl::xsha2_256_t::digest(property_bin);
            XMETRICS_GAUGE(metrics::cpu_hash_256_xvproperty_property_bin_calc, 1);
            std::string prophash2 = std::string(reinterpret_cast<char*>(hash.data()), hash.size());
            if (prophash != prophash2) {
                xerror("xpropertyprove_build_t::create_property_prove hash1=%s,hash2=%s",
                    base::xstring_utl::to_hex(prophash).c_str(), base::xstring_utl::to_hex(prophash2).c_str());
                xassert(false);
                return nullptr;                
            }
            xinfo("xpropertyprove_build_t::create_property_prove succ hash1=%s,hash2=%s",
                base::xstring_utl::to_hex(prophash).c_str(), base::xstring_utl::to_hex(prophash2).c_str());            
            #endif
            xvproperty_prove_ptr_t _property_receipt = make_object_ptr<xvproperty_prove_t>(_property.get(), txreceipt);
            return _property_receipt;
        }

    }  // namespace base
}  // namespace top
