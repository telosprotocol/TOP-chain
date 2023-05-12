// Copyright (c) 2018-Present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xbase/xobject.h"
#include "xvledger/xvaccount.h"

namespace top
{
    namespace base
    {
        class xvaccountobj_t;
        
        enum enum_xvaccount_plugin_type //max 8 plugins
        {
            enum_xvaccount_plugin_start      = 0x00,
            enum_xvaccount_plugin_blockmgr   = 0x00, //manage blocks
            enum_xvaccount_plugin_statemgr   = 0x01, //manage states
            // enum_xvaccount_plugin_indexmgr   = 0x03, //manage indexs
            // enum_xvaccount_plugin_executemgr = 0x04, //manage contract & function execute
            
            //note:update max once add new plugin at above
            enum_xvaccount_plugin_end        = enum_xvaccount_plugin_statemgr,
            enum_xvaccount_plugin_max        = enum_xvaccount_plugin_end + 2,
        };
        
        class xvactplugin_t : public xobject_t
        {
            friend class xvaccountobj_t;
            friend class xvtable_t;
        protected:
            xvactplugin_t(xvaccountobj_t & parent_obj,const uint64_t idle_timeout_ms,enum_xvaccount_plugin_type type);
            virtual ~xvactplugin_t();
        private:
            xvactplugin_t();
            xvactplugin_t(xvactplugin_t &&);
            xvactplugin_t(const xvactplugin_t &);
            xvactplugin_t & operator = (const xvactplugin_t &);
            
        public:
            enum_xvaccount_plugin_type get_plugin_type()   const {return m_plugin_type;}
            const xvid_t            get_xvid()    const;
            const std::string &     get_account_address()  const;
            inline const std::string&   get_address() const {return get_account_address();}
            inline const std::string&   get_account() const {return get_account_address();}

            xvaccountobj_t*         get_account_obj()      const {return m_account_obj;}
            inline const uint64_t   get_idle_duration()    const {return m_idle_timeout_ms;}
            inline const uint64_t   get_last_access_time() const {return m_last_access_time_ms;} //UTC ms
            void                    set_last_access_time(const uint64_t last_access_time);
            
            //test whether has been idle status
            inline bool             is_closing() const {return (m_is_closing != 0);}
            virtual bool            is_live(const uint64_t timenow_ms) override;
            
        protected: //for generel purpose we place all kind of APIs ,but each plugin may just has 0 or 1 ptr
            virtual const xblockmeta_t*     get_block_meta() const {return NULL;}
            virtual const xstatemeta_t*     get_state_meta() const {return NULL;}
            virtual const xsyncmeta_t*      get_sync_meta()  const {return NULL;}
            
            virtual bool                    init_meta(const xvactmeta_t & meta) {return false;} //give chance to init plugin
            virtual bool                    save_meta() {return false;} //peristen meta
            virtual bool                    update_meta(){return false;}//update meta into cache
            //note:save_data must be protected by table' lock before call save_data
            virtual bool                    save_data()  {return false;}; //give plugin a chance to save blocks/tx to db
        protected:
            void                    stop(); //mark idle flag
            virtual bool            close(bool force_async = true) override;
            
        private:
            xvaccountobj_t * m_account_obj;
            uint64_t         m_last_access_time_ms; //UTC ms
            uint64_t         m_idle_timeout_ms;     //how long(ms) it will change to idle status
            enum_xvaccount_plugin_type m_plugin_type;
            uint8_t          m_is_closing; //atomic indicate whether is beeing idle status, 1 = true, 0 = false
        };
    
    }//end of namespace of base
}//end of namespace top
