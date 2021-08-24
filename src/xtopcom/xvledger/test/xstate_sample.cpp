// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "../xvaccount.h"
#include "../xvblock.h"
#include "../xvstate.h"

template<typename T>
class xvar_t
{
public:
    static T get(top::base::xvalue_t & var){
        return var.get_value<T>();
    }
};

top::base::xvbstate_t*  create_xvbstate(const std::string & account,const uint64_t block_height,const uint64_t block_viewid)
{
    return new top::base::xvbstate_t(account,block_height,block_viewid,std::string(),std::string(),0,0,0);
}

int test_xstate(bool is_stress_test)
{
    top::base::xvblock_t::register_object(top::base::xcontext_t::instance());
    
    
    {
        {
            std::vector<int8_t> vect;
            for(int i = 0; i < 9; ++i)
            {
                vect.push_back(i);
            }
            top::base::xvalue_t val(vect);
            printf("value=%s\n",val.dump().c_str());
        }
        
        {
            std::vector<int16_t> vect;
            for(int i = 0; i < 9; ++i)
            {
                vect.push_back(i);
            }
            top::base::xvalue_t val(vect);
            printf("value=%s\n",val.dump().c_str());
        }
        
        {
            std::vector<int32_t> vect;
            for(int i = 0; i < 9; ++i)
            {
                vect.push_back(i);
            }
            top::base::xvalue_t val(vect);
            printf("value=%s\n",val.dump().c_str());
        }
        
        
        {
            std::vector<int64_t> vect;
            for(int i = 0; i < 9; ++i)
            {
                vect.push_back(i);
            }
            top::base::xvalue_t val(vect);
            printf("value=%s\n",val.dump().c_str());
        }
        
        {
            std::vector<uint64_t> vect;
            for(int i = 0; i < 9; ++i)
            {
                vect.push_back(i);
            }
            top::base::xvalue_t val(vect);
            printf("value=%s\n",val.dump().c_str());
        }
        
        {
            std::vector<std::string> vect;
            for(int i = 0; i < 9; ++i)
            {
                vect.push_back(top::base::xstring_utl::tostring(i));
            }
            top::base::xvalue_t val(vect);
            printf("value=%s\n",val.dump().c_str());
        }
 
        top::base::xauto_ptr<top::base::xvbstate_t> block_state_from_topchain(create_xvbstate("Ta00013T7BKn5pP8Zi3K5z2Z5BQuSXTf5u37Se79x@0",32,1));
        
        const std::string binlog_hex_str_from_topchain("ac20dc03f133020000005b2e546130303031335437424b6e357050385a69334b357a325a354251755358546635753337536537397840302f33320905c1010f034054303f062b54323a00f51c38416f346a6a597472586f4e77667a62366764704432584e4270715576343670384240300603878b6500203300f012435177797a467862575a35396d4e6a6b7133655a3365483431743762356d69646d330028fd8a3300fe124b633957796e647571784a76583356435537586a4843523959794b75424c3166783300ff124e58623336476b6f6642554d717843415a71644552693633687444564338597a74330000f9165a6a764e4a6a524e4735694571564b7964707141716f654e6a4275466d4e626a40300618aacc00f5124e70527859434651784d4876656454785270676b623842376f48743233354e32576600141b3e01f200323f0c033131330300000103313136080022323808001333180022343710002035350800410432303421000009001039120030023231070040033232340800e00232390300000102343103000001");
        
        const std::string binlog_raw_str = top::base::xstring_utl::from_hex(binlog_hex_str_from_topchain);
        block_state_from_topchain->apply_changes_of_binlog(binlog_raw_str);
        
        std::string full_state_bin;
        block_state_from_topchain->take_snapshot(full_state_bin);
        
        
        top::base::xauto_ptr<top::base::xvbstate_t> full_block_state(create_xvbstate("Ta00013T7BKn5pP8Zi3K5z2Z5BQuSXTf5u37Se79x@0",32,1));
        full_block_state->apply_changes_of_binlog(full_state_bin);
        
        //xassert(full_state_bin == binlog_raw_str);
    }
    
    {
        top::base::xauto_ptr<top::base::xvbstate_t> block_state_from_topio(create_xvbstate("Ta00013T7BKn5pP8Zi3K5z2Z5BQuSXTf5u37Se79x@0",32,1));
        
        const std::string binlog_hex_str_from_topio( "d58ddc03f133020000005b2e546130303031335437424b6e357050385a69334b357a325a354251755358546635753337536537397840302f33320905c1010f034054303f062b54323a00f51c38416f346a6a597472586f4e77667a62366764704432584e4270715576343670384240300603878b6500203300f012435177797a467862575a35396d4e6a6b7133655a3365483431743762356d69646d330028fd8a3300fe124b633957796e647571784a76583356435537586a4843523959794b75424c3166783300ff124e58623336476b6f6642554d717843415a71644552693633687444564338597a74330000f9165a6a764e4a6a524e4735694571564b7964707141716f654e6a4275466d4e626a40300618aacc00f5124e70527859434651784d4876656454785270676b623842376f48743233354e32576600141b3e01f200323f0c033131330300000103313136080022323808001333180022343710002035350800410432303421000009001039120030023231070040033232340800220232160060343103000001");
        
        const std::string binlog_raw_str = top::base::xstring_utl::from_hex(binlog_hex_str_from_topio);
        
        block_state_from_topio->apply_changes_of_binlog(binlog_raw_str);
        
        std::string full_state_bin;
        block_state_from_topio->take_snapshot(full_state_bin);
        xassert(full_state_bin == binlog_raw_str);
    }
    
    
    //test system account
    {
        const std::string account_addr = top::base::xvaccount_t::make_account_address(top::base::enum_vaccount_addr_type_native_contract, top::base::enum_test_chain_id, top::base::enum_chain_zone_beacon_index, 127, 7, std::string("1234567890abcdef"));
        const xvid_t account_id = top::base::xvaccount_t::get_xid_from_account(account_addr);
        xassert(account_id != 0);
        xassert(top::base::xvaccount_t::get_addrtype_from_account(account_addr) == top::base::enum_vaccount_addr_type_native_contract);
        xassert(top::base::xvaccount_t::get_chainid_from_ledgerid(top::base::xvaccount_t::get_ledgerid_from_account(account_addr)) == top::base::enum_test_chain_id);
        xassert(top::base::xvaccount_t::get_zoneindex_from_ledgerid(top::base::xvaccount_t::get_ledgerid_from_account(account_addr)) == top::base::enum_chain_zone_beacon_index);
        //xassert(get_vledger_subaddr(account_id) == 1023);
    }
    //test regular account
    {
        const std::string account_addr = top::base::xvaccount_t::make_account_address(top::base::enum_vaccount_addr_type_secp256k1_user_account,top::base::xvaccount_t::make_ledger_id(top::base::enum_main_chain_id, top::base::enum_chain_zone_consensus_index),std::string("1234567890abcdef"));
        const xvid_t account_id = top::base::xvaccount_t::get_xid_from_account(account_addr);
        xassert(account_id != 0);
        xassert(top::base::xvaccount_t::get_addrtype_from_account(account_addr) == top::base::enum_vaccount_addr_type_secp256k1_user_account);
        xassert(top::base::xvaccount_t::get_chainid_from_ledgerid(top::base::xvaccount_t::get_ledgerid_from_account(account_addr)) == top::base::enum_main_chain_id);
        xassert(top::base::xvaccount_t::get_zoneindex_from_ledgerid(top::base::xvaccount_t::get_ledgerid_from_account(account_addr)) == top::base::enum_chain_zone_consensus_index);
    }
    
    
    //test varint and varstring first
    {
        //short string
        {
            top::base::xautostream_t<1024> _test_stream(top::base::xcontext_t::instance());
            
            const std::string test_string("1234567");
            const int32_t test_i32  = -32;
            const uint32_t test_ui32 = 32;
            const int64_t test_i64  = -64;
            const uint64_t test_ui64 = 64;
            _test_stream.write_compact_var(test_i32);
            _test_stream.write_compact_var(test_ui32);
            _test_stream.write_compact_var(test_i64);
            _test_stream.write_compact_var(test_ui64);
            _test_stream.write_compact_var(test_string);
            
            std::string compressed_bin_string_stage1;
            top::base::xstream_t::compress_to_string(_test_stream,_test_stream.size(),compressed_bin_string_stage1);
            std::string compressed_bin_string_stage2;
            top::base::xstream_t::compress_to_string(compressed_bin_string_stage1,compressed_bin_string_stage2);
            std::string decompressed_bin_string_stage2;
            top::base::xstream_t::decompress_from_string(compressed_bin_string_stage2,decompressed_bin_string_stage2);
            _test_stream.reset();
            top::base::xstream_t::decompress_from_string(decompressed_bin_string_stage2, _test_stream);
            

            std::string verify_string;
            int32_t verify_i32  = 0;
            uint32_t verify_ui32 = 0;
            int64_t verify_i64  = 0;
            uint64_t verify_ui64 = 0;
            _test_stream.read_compact_var(verify_i32);
            _test_stream.read_compact_var(verify_ui32);
            _test_stream.read_compact_var(verify_i64);
            _test_stream.read_compact_var(verify_ui64);
            _test_stream.read_compact_var(verify_string);
            
            xassert(test_i32 == verify_i32);
            xassert(test_ui32 == verify_ui32);
            xassert(test_i64 == verify_i64);
            xassert(test_ui64 == verify_ui64);
            xassert(test_string == verify_string);
            
            xassert(_test_stream.size() == 0);
        }
        //medium string(> 64K)
        {
            top::base::xautostream_t<1024> _test_stream(top::base::xcontext_t::instance());
            std::string test_string;
            std::string test_seed_string("0123456789");
            for(int i = 0; i < 6666; ++i) //over 65536 bytes
            {
                test_string += test_seed_string;
            }
            _test_stream.write_compact_var(test_string);
            
            std::string bin_string;
            top::base::xstream_t::compress_to_string(_test_stream,_test_stream.size(), bin_string);
            _test_stream.reset();
            top::base::xstream_t::decompress_from_string(bin_string, _test_stream);
            
            std::string verify_string;
            _test_stream.read_compact_var(verify_string);
            xassert(test_string == verify_string);
            xassert(_test_stream.size() == 0);
        }
        
        //large string(> 16M)
        if(0)
        {
            top::base::xautostream_t<1024> _test_stream(top::base::xcontext_t::instance());
            std::string test_string;
            std::string test_seed_string("0123456789");
            for(int i = 0; i < 1677722; ++i) //over 16M bytes
            {
                test_string += test_seed_string;
            }
            _test_stream.write_compact_var(test_string);
            std::string verify_string;
            _test_stream.read_compact_var(verify_string);
            xassert(test_string == verify_string);
            xassert(_test_stream.size() == 0);
        }
 
    }
    
    const std::string account_addr("abcdefg");
    //vm-rumtime modify state of block & record
    {

        top::base::xvalue_t var(std::string("test string"));
        const int32_t intv = xvar_t<int32_t>::get(var);
        const std::string strv = xvar_t<std::string>::get(var);
        
        top::base::xauto_ptr<top::base::xvcanvas_t> hq_block_canvas(new top::base::xvcanvas_t());
        top::base::xauto_ptr<top::base::xvbstate_t> hq_block_state(create_xvbstate(account_addr,1,1));
        if(hq_block_state) //build state of a hq block
        {
            
            top::base::xauto_ptr<top::base::xtokenvar_t> token_property(hq_block_state->new_token_var(std::string("@token"),hq_block_canvas()));
            if(token_property)
            {
                auto add_100 = token_property->deposit(top::base::vtoken_t(100),hq_block_canvas());
                auto sub_10 = token_property->withdraw(top::base::vtoken_t(10),hq_block_canvas());
                auto sub_100 = token_property->withdraw(top::base::vtoken_t(100),hq_block_canvas());
            }

            top::base::xauto_ptr<top::base::xmtokens_t> native_tokens(hq_block_state->new_multiple_tokens_var (std::string("@nativetokens"),hq_block_canvas()));
            if(native_tokens)
            {
                xassert(native_tokens->deposit("@vpntoken", 100,hq_block_canvas()));
                xassert(native_tokens->withdraw("@vpntoken", 10,hq_block_canvas()));
                xassert(native_tokens->withdraw("@vpntoken", 100,hq_block_canvas()));
                
                xassert(native_tokens->withdraw("@messagingtoken", 10,hq_block_canvas()));
                xassert(native_tokens->withdraw("@messagingtoken", 100,hq_block_canvas()));
                xassert(native_tokens->deposit("@messagingtoken", 100,hq_block_canvas()));
            }
            
            top::base::xauto_ptr<top::base::xnoncevar_t> nonce_property(hq_block_state->new_nonce_var(std::string("@nonce"),hq_block_canvas()));
            if(nonce_property){
                auto res = nonce_property->alloc_nonce(hq_block_canvas());
            }
            
            top::base::xauto_ptr<top::base::xcodevar_t> code_property(hq_block_state->new_code_var(std::string("@code"),hq_block_canvas()));
            if(code_property){
                code_property->deploy_code("@code",hq_block_canvas());
                auto result = code_property->deploy_code("try overwrite to value",hq_block_canvas());
                xassert(result == false);
            }
            
            top::base::xauto_ptr<top::base::xstringvar_t> string_property(hq_block_state->new_string_var(std::string("@string"),hq_block_canvas()));
            if(string_property){
                string_property->reset("test",hq_block_canvas());
                string_property->reset("@string",hq_block_canvas());
            }
            
            top::base::xauto_ptr<top::base::xmapvar_t<std::string>> map_property(hq_block_state->new_string_map_var(std::string("@stringmap"),hq_block_canvas()));
            if(map_property){
                auto res = map_property->insert("name", "@stringmap",hq_block_canvas());
            }
            
            top::base::xauto_ptr<top::base::xdequevar_t<std::string>> queue_property(hq_block_state->new_string_deque_var(std::string("@stringdeque"),hq_block_canvas()));
            if(map_property){
                auto res = queue_property->push_back("@stringdeque",hq_block_canvas());
            }
            
            top::base::xauto_ptr<top::base::xvintvar_t<int64_t>> int64_property(hq_block_state->new_int64_var(std::string("@5"),hq_block_canvas()));
            if(int64_property){
                auto res = int64_property->set(-5,hq_block_canvas());
            }
            
            top::base::xauto_ptr<top::base::xvintvar_t<uint64_t>> uint64_property(hq_block_state->new_uint64_var(std::string("@6"),hq_block_canvas()));
            if(uint64_property){
                auto res = uint64_property->set(6,hq_block_canvas());
            }
            
            {
                top::base::xauto_ptr<top::base::xdequevar_t<int8_t>> int8_deque_property(hq_block_state->new_int8_deque_var(std::string("@deque_int8"),hq_block_canvas()));
                if(int8_deque_property){
                    auto res = int8_deque_property->push_back(6,hq_block_canvas());
                    int8_deque_property->update(0,-8,hq_block_canvas());
                    xassert(int8_deque_property->query(0) == -8);
                }
                
                top::base::xauto_ptr<top::base::xdequevar_t<int16_t>> int16_deque_property(hq_block_state->new_int16_deque_var(std::string("@deque_int16"),hq_block_canvas()));
                if(int16_deque_property){
                    auto res = int16_deque_property->push_front(1,hq_block_canvas());
                    int16_deque_property->pop_front(hq_block_canvas());
                    int16_deque_property->push_back(2,hq_block_canvas());
                    int16_deque_property->update(0,-16,hq_block_canvas());
                    xassert(int16_deque_property->query(0) == -16);
                }
                
                top::base::xauto_ptr<top::base::xdequevar_t<int32_t>> int32_deque_property(hq_block_state->new_int32_deque_var(std::string("@deque_int32"),hq_block_canvas()));
                if(int32_deque_property){
                    int32_deque_property->push_back(1,hq_block_canvas());
                    int32_deque_property->pop_back(hq_block_canvas());
                    int32_deque_property->push_front(2,hq_block_canvas());
                    int32_deque_property->update(0,-32,hq_block_canvas());
                    xassert(int32_deque_property->query(0) == -32);
                }
                
                top::base::xauto_ptr<top::base::xdequevar_t<int64_t>> int64_deque_property(hq_block_state->new_int64_deque_var(std::string("@deque_int64"),hq_block_canvas()));
                if(int64_deque_property){
                    auto res = int64_deque_property->push_back(-64,hq_block_canvas());
                    xassert(int64_deque_property->query(0) == -64);
                }
                
                top::base::xauto_ptr<top::base::xdequevar_t<uint64_t>> uint64_deque_property(hq_block_state->new_uint64_deque_var(std::string("@deque_uint64"),hq_block_canvas()));
                if(uint64_deque_property){
                    auto res = uint64_deque_property->push_back(64,hq_block_canvas());
                    xassert(uint64_deque_property->query(0) == 64);
                }
            }
            
            {
                top::base::xauto_ptr<top::base::xmapvar_t<int8_t>> int8_map_property(hq_block_state->new_int8_map_var(std::string("@map_int8"),hq_block_canvas()));
                if(int8_map_property){
                    auto res = int8_map_property->insert("name", 1,hq_block_canvas());
                    int8_map_property->erase("name",hq_block_canvas());
                    int8_map_property->insert("name", -8,hq_block_canvas());
                    xassert(int8_map_property->query("name")  == -8);
                }
                
                top::base::xauto_ptr<top::base::xmapvar_t<int16_t>> int16_map_property(hq_block_state->new_int16_map_var(std::string("@map_int16"),hq_block_canvas()));
                if(int16_map_property){
                    auto res = int16_map_property->insert("name", 1,hq_block_canvas());
                    int8_map_property->clear(hq_block_canvas());
                    int8_map_property->insert("name", -8,hq_block_canvas());
                    int16_map_property->insert("name", -16,hq_block_canvas());
                    xassert(int16_map_property->query("name")  == -16);
                }
                
                top::base::xauto_ptr<top::base::xmapvar_t<int32_t>> int32map_property(hq_block_state->new_int32_map_var(std::string("@map_int32"),hq_block_canvas()));
                if(int32map_property){
                    auto res = int32map_property->insert("name", 1,hq_block_canvas());
                    std::map<std::string,int32_t> new_map;
                    new_map["name"] = -32;
                    int32map_property->reset(new_map,hq_block_canvas());
                    xassert(int32map_property->query("name")  == -32);
                }
                
                top::base::xauto_ptr<top::base::xmapvar_t<int64_t>> int64map_property(hq_block_state->new_int64_map_var(std::string("@map_int64"),hq_block_canvas()));
                if(int64map_property){
                    auto res = int64map_property->insert("name", -64,hq_block_canvas());
                    xassert(int64map_property->query("name")  == -64);
                }
                
                top::base::xauto_ptr<top::base::xmapvar_t<uint64_t>> uint64map_property(hq_block_state->new_uint64_map_var(std::string("@map_uint64"),hq_block_canvas()));
                if(uint64map_property){
                    auto res = uint64map_property->insert("name", 64,hq_block_canvas());
                    xassert(uint64map_property->query("name")  == 64);
                }
            }
        }
        
        //test rebuild state from bin-log

        //rebuild bin-log
        std::string recorded_bin_log;
        //hq_block_state->rebase_change_to_snapshot(); //convert to full state
        hq_block_canvas->encode(recorded_bin_log);
        hq_block_canvas->log();
        hq_block_canvas->print();
   
        top::base::xauto_ptr<top::base::xvbstate_t> copy_block_state(create_xvbstate(account_addr,1,1));
        copy_block_state->apply_changes_of_binlog(recorded_bin_log);
        
        
        auto canvas = copy_block_state->rebase_change_to_snapshot();
        canvas->encode(recorded_bin_log);
        
        std::string bstate_bin;
        copy_block_state->serialize_to_string(bstate_bin);
        top::base::xauto_ptr<top::base::xvbstate_t> confirm_block_state = top::base::xvblock_t::create_state_object(bstate_bin);
        confirm_block_state->apply_changes_of_binlog(recorded_bin_log);
        
        
        //reserialize block/state
        std::string state_log;
        confirm_block_state->serialize_to_string(state_log);
        top::base::xauto_ptr<top::base::xvbstate_t> reload_state(create_xvbstate(account_addr,1,1));
        reload_state->serialize_from_string(state_log);
        
        //test clone from existing state
        top::base::xauto_ptr<top::base::xvbstate_t> clone_block_state((top::base::xvbstate_t*)reload_state->clone());
        if(clone_block_state) //do final verify
        {
            //verify result
            auto token = clone_block_state->load_token_var(std::string("@token"));
            xassert(-10 == token->get_balance());
            
            auto native_tokens = clone_block_state->load_multiple_tokens_var (std::string("@nativetokens"));
            if(native_tokens)
            {
                xassert(-10 == native_tokens->get_balance("@vpntoken"));
                xassert(-10 == native_tokens->get_balance("@messagingtoken"));
            }
            
            auto nonce = clone_block_state->load_nonce_var(std::string("@nonce"));
            xassert(1 == nonce->get_nonce());
            
            auto code = clone_block_state->load_code_var(std::string("@code"));
            xassert(code->get_code() == "@code");
            
            auto str = clone_block_state->load_string_var(std::string("@string"));
            xassert(str->query() == "@string");
            
            auto stringmap = clone_block_state->load_string_map_var(std::string("@stringmap"));
            xassert(stringmap->query("name") == "@stringmap");
            
            auto stringqueue = clone_block_state->load_string_deque_var(std::string("@stringdeque"));
            xassert(stringqueue->query(0) == "@stringdeque");
            
            auto int64_val = clone_block_state->load_int64_var(std::string("@5"));
            xassert(int64_val->get() == -5);
            
            auto uint64_val = clone_block_state->load_uint64_var(std::string("@6"));
            xassert(uint64_val->get() == 6);
            
            //verify deque
            {
                auto deque_int8 = clone_block_state->load_int8_deque_var(std::string("@deque_int8"));
                xassert(deque_int8->query(0) == -8);
                
                auto deque_int16 = clone_block_state->load_int16_deque_var(std::string("@deque_int16"));
                xassert(deque_int16->query(0) == -16);
                
                auto deque_int32 = clone_block_state->load_int32_deque_var(std::string("@deque_int32"));
                xassert(deque_int32->query(0) == -32);
                
                auto deque_int64 = clone_block_state->load_int64_deque_var(std::string("@deque_int64"));
                xassert(deque_int64->query(0) == -64);
                
                auto deque_uint64 = clone_block_state->load_uint64_deque_var(std::string("@deque_uint64"));
                xassert(deque_uint64->query(0) == 64);
            }
            
            //verify map
            {
                auto map_int8 = clone_block_state->load_int8_map_var(std::string("@map_int8"));
                xassert(map_int8->query("name") == -8);
                
                auto map_int16 = clone_block_state->load_int16_map_var(std::string("@map_int16"));
                xassert(map_int16->query("name") == -16);
                
                auto map_int32 = clone_block_state->load_int32_map_var(std::string("@map_int32"));
                xassert(map_int32->query("name") == -32);
                
                auto map_int64 = clone_block_state->load_int64_map_var(std::string("@map_int64"));
                xassert(map_int64->query("name") == -64);
                
                auto map_uint64 = clone_block_state->load_uint64_map_var(std::string("@map_uint64"));
                xassert(map_uint64->query("name") == 64);
            }
        
            xassert(true);
        }
        
    
    }
 
    printf("/////////////////////////////// [test_xstate] finish ///////////////////////////////  \n");
    return 0;
}
