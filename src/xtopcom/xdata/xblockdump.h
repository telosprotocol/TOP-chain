// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>

#include "xvledger/xvblock.h"

NS_BEG2(top, data)

class xblockdump_t {
 public:
   static void  dump_tableblock(base::xvblock_t* block);

 private:
   static void  dump_unitblock(base::xvblock_t* block);
   static void  dump_action(std::string const& prefix_str, base::xvaction_t const& action);
   static void  dump_input_entity(std::string const& prefix_str, base::xvinentity_t* entity);
   static void  dump_output_entity(std::string const& prefix_str, base::xvoutentity_t* entity);
   static void  dump_input_entitys(base::xvblock_t* block);
   static void  dump_output_entitys(base::xvblock_t* block);
   static void  dump_input_resources(base::xvblock_t* block);
   static void  dump_output_resources(base::xvblock_t* block);
   static void  dump_object(base::xvblock_t* block);
};


NS_END2
