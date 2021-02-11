// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xunitblock.hpp"

namespace top
{
    namespace test
    {
        class xtestclocker_t
        {
        public:
            xtestclocker_t();
        protected:
            ~xtestclocker_t();
        private:
            xtestclocker_t(const xtestclocker_t &);
            xtestclocker_t & operator = (const xtestclocker_t &);
        public:
            base::xvblock_t*    on_clock_fire();//caller notify xtestclocker_t generate new clock block
        
            base::xauto_ptr<base::xvblock_t>    get_latest_clock();
        private:
            base::xvblockstore_t*  m_blockstore;
        private:
            std::string            m_clock_account;
        };
    };
};
