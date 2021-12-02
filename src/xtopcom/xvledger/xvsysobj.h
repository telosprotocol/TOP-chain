// Copyright (c) 2018-Present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xobject.h"
#include "xbase/xlock.h"
#include "xbase/xtimer.h"

namespace top
{
    namespace base
    {
        //configuration for boot/init...
        class xvconfig_t : public xobject_t
        {
        public:
            xvconfig_t(){};
        protected:
            virtual ~xvconfig_t(){};
        private:
            xvconfig_t(xvconfig_t &&);
            xvconfig_t(const xvconfig_t &);
            xvconfig_t & operator = (const xvconfig_t &);
        public:
            //key_path must be formated as dir path like /xx/yyy/zzz... or dot path like xx.yy.zzz
            const std::string  get_config(const std::string & key_path) const;
            bool               set_config(const std::string & key_path,const std::string & key_value);
        private:
            xspinlock_t    m_lock;
            std::map<std::string,std::string> m_keys_map;
        };
    
        class xsysobject_t : public xobject_t
        {
        public:
            enum enum_sys_object_type : int16_t
            {
                enum_sys_object_type_undef   = enum_xobject_type_base,
                //below are pre-defined object and be reserved
                enum_sys_object_type_bootstrap      = enum_xobject_type_min + 1,//init & boot
                enum_sys_object_type_module         = enum_xobject_type_min + 2,//function set
                enum_sys_object_type_daemon         = enum_xobject_type_min + 3,//service & daemon
                enum_sys_object_type_driver         = enum_xobject_type_min + 4,//net,disk etc
                enum_sys_object_type_plugin         = enum_xobject_type_min + 5,//plugin etc
                enum_sys_object_type_filter         = enum_xobject_type_min + 6,//filter etc
            };
            //version defintion = [8:Features][8:MAJOR][8:MINOR][8:PATCH]
            static const std::string version_to_string(const uint32_t ver_int);
            static const uint32_t    string_to_version(const std::string & ver_str);
        protected:
            xsysobject_t();
            xsysobject_t(const int16_t sys_object_type); //_enum_xobject_type_ refer enum_xobject_type
            virtual ~xsysobject_t();
        private:
            xsysobject_t(xsysobject_t &&);
            xsysobject_t(const xsysobject_t &);
            xsysobject_t & operator = (const xsysobject_t &);
            
        public:
            virtual bool  is_valid(const uint32_t obj_ver); //check version
            virtual int   init(const xvconfig_t & config);//init entry
            virtual bool  start(const int32_t at_thread_id = 0); //on-demand start thread if at_thread_id !=0
            virtual std::string get_obj_name() const override {return m_obj_key;}

            inline const int    get_object_type() const { return get_obj_type();}
            const std::string&  get_object_key()  const { return m_obj_key;}
            const uint32_t      get_object_version() const {return m_obj_version;}

            //only allow for the uninited object
            bool  set_object_type(const int16_t sys_object_type);
            bool  set_object_key(const char* ptr_obj_key);
            bool  set_object_key(const std::string & obj_key);
            bool  set_object_version(const uint32_t obj_ver);
            
        public: //forward to xiobject  if exist
            const uint64_t       get_time_now();
            const int32_t        get_thread_id() const; //0 means no-bind to any thread yet
            xcontext_t*          get_context() const;
            const xvconfig_t*    get_config()  const {return m_config_ptr;}
        protected://process
            virtual bool  run(const int32_t cur_thread_id,const uint64_t timenow_ms){return true;}
            xiobject_t*          get_iobject() const {return m_raw_iobject;}
        private:
            std::string     m_obj_key;
            xvconfig_t*     m_config_ptr;  //reference the config object and may use later
            xiobject_t*     m_raw_iobject; //bind iobject to given function related thread
            //The definition of version defintion = [8:Features][8:MAJOR][8:MINOR][8:PATCH]
            //Features: added new featurs that need most node agree then make it effect,refer BIP8 spec
                //MAJOR： version when make incompatible
                //MINOR： version when add functionality in a backwards compatible manner
                //PATCH： version when make backwards compatible bug fixes.
            uint32_t        m_obj_version;
        };
    
        typedef xsysobject_t* (*xnew_sysobj_function_t)(const uint32_t obj_ver);
        //static object ->register auto
        class xvsyslibrary
        {
        public:
            static xvsyslibrary & instance()
            {
                static xvsyslibrary _static_global_inst;
                return _static_global_inst;
            }
        private:
            xvsyslibrary(){};
            ~xvsyslibrary(){};
            xvsyslibrary(xvsyslibrary &&);
            xvsyslibrary(const xvsyslibrary &);
            xvsyslibrary & operator = (const xvsyslibrary &);
            
        public:
            bool  register_object(const char * object_key,xnew_sysobj_function_t _func_ptr);
            bool  register_object(const int   object_type,xnew_sysobj_function_t _func_ptr);
            
            xsysobject_t*  create_object(const int obj_type,const uint32_t obj_ver = 0);
            xsysobject_t*  create_object(const char* obj_key,const uint32_t obj_ver = 0);
            xsysobject_t*  create_object(const std::string & obj_key,const uint32_t obj_ver = 0);
        private:
            xspinlock_t    m_lock;
            std::map<int,xnew_sysobj_function_t>         m_type_to_obj;
            std::map<std::string,xnew_sysobj_function_t> m_key_to_obj;
        };
    
        template<typename T>
        class xsyscreator
        {
        protected:
            xsyscreator()
            {
                _auto_registor.do_nothing();
            }
            ~xsyscreator()
            {
                _auto_registor.do_nothing();
            }
        private:
            static xsysobject_t * new_object(const uint32_t obj_ver)
            {
                xsysobject_t* obj_ptr = new T;
                obj_ptr->set_object_type(T::get_register_type());
                obj_ptr->set_object_key(T::get_register_key());
                
                if(false == obj_ptr->is_valid(obj_ver))
                {
                    obj_ptr->release_ref();
                    obj_ptr = nullptr;
                }
                return obj_ptr;
            }
            class xsys_registor
            {
            public:
                xsys_registor()
                {
                    if(T::get_register_key() != nullptr)
                    {
                        //printf("xsysobject::registor,obj_key=%s,name=%s \n",obj_key,typeid(T).name());
                        xvsyslibrary::instance().register_object(T::get_register_key(), new_object);
                    }
                    if(T::get_register_type() != xsysobject_t::enum_sys_object_type_undef)
                    {
                        //printf("xsysobject::registor,obj_type=%d,name=%s \n",obj_type,typeid(T).name());
                        xvsyslibrary::instance().register_object(T::get_register_type(), new_object);
                    }
                }
                inline void do_nothing() const {};
            };
            static xsys_registor _auto_registor;
        };
    
        template<typename T>
        typename xsyscreator<T>::xsys_registor xsyscreator<T>::_auto_registor;
    
        //bootstrap for system and chain process
        class xvbootstrap_t : public xsysobject_t
        {
        public:
            //just register by object_key,instead of object_type,//not register type
            static const int16_t get_register_type() {return xsysobject_t::enum_sys_object_type_undef;}
        protected:
            xvbootstrap_t();
            virtual ~xvbootstrap_t();
        private:
            xvbootstrap_t(xvbootstrap_t &&);
            xvbootstrap_t(const xvbootstrap_t &);
            xvbootstrap_t & operator = (const xvbootstrap_t &);
        public:
            //caller respond to cast (void*) to related  interface ptr
            virtual void*  query_interface(const int32_t _enum_xobject_type_) override;
        protected:
            virtual bool  run(const int32_t cur_thread_id,const uint64_t timenow_ms) override;
        };

        //logic wrap of logic and functions
        class xvmodule_t : public xsysobject_t
        {
        public:
            //just register by object_key,instead of object_type//not register type
            static const int16_t get_register_type(){return xsysobject_t::enum_sys_object_type_undef;}
        protected:
            xvmodule_t();
            virtual ~xvmodule_t();
        private:
            xvmodule_t(xvmodule_t &&);
            xvmodule_t(const xvmodule_t &);
            xvmodule_t & operator = (const xvmodule_t &);
        public:
            //caller respond to cast (void*) to related  interface ptr
            virtual void*  query_interface(const int32_t _enum_xobject_type_) override;
        protected:
            virtual bool  run(const int32_t cur_thread_id,const uint64_t timenow_ms) override;
        };
    
        //virutal daemone running background
        class xvdaemon_t : public xsysobject_t
        {
        public:
            //just register by object_key,instead of object_type,//not register type
            static const int16_t get_register_type(){return xsysobject_t::enum_sys_object_type_undef;}
        protected:
            xvdaemon_t();
            virtual ~xvdaemon_t();
        private:
            xvdaemon_t(xvdaemon_t &&);
            xvdaemon_t(const xvdaemon_t &);
            xvdaemon_t & operator = (const xvdaemon_t &);
        public:
            //caller respond to cast (void*) to related  interface ptr
            virtual void*  query_interface(const int32_t _enum_xobject_type_) override;
        protected:
            virtual bool  run(const int32_t cur_thread_id,const uint64_t timenow_ms) override;
        };
    
        //virtual network,disk and log driver
        class xvdriver_t : public xsysobject_t
        {
        public:
            //just register by object_key,instead of object_type//not register type
            static const int16_t get_register_type(){return xsysobject_t::enum_sys_object_type_undef;}
        protected:
            xvdriver_t();
            virtual ~xvdriver_t();
        private:
            xvdriver_t(xvdriver_t &&);
            xvdriver_t(const xvdriver_t &);
            xvdriver_t & operator = (const xvdriver_t &);
        public:
            //caller respond to cast (void*) to related  interface ptr
            virtual void*  query_interface(const int32_t _enum_xobject_type_) override;
        protected:
            virtual bool  run(const int32_t cur_thread_id,const uint64_t timenow_ms) override;
        };
    
        //bootstrap object that load whole chain system
        class xvsysinit_t : public xvbootstrap_t,public xtimersink_t,public xsyscreator<xvsysinit_t>
        {
            friend class xsyscreator<xvsysinit_t>;
        public:
            static const char* get_register_key()
            {
                return "/init";
            }
            static const uint32_t get_register_version()
            {
                return xsysobject_t::string_to_version("0.0.0.1");
            }
            enum { enum_monitor_boot_interval_ms = 2000}; //every 2 seconds
        protected:
            xvsysinit_t();
            virtual ~xvsysinit_t();
        private:
            xvsysinit_t(xvsysinit_t &&);
            xvsysinit_t(const xvsysinit_t &);
            xvsysinit_t & operator = (const xvsysinit_t &);
            
        public:
            virtual int   init(const xvconfig_t & config_obj) override;
            virtual bool  start(const int32_t at_thread_id = 0) override;
            virtual bool  close(bool force_async = true) override; //must call close before release
        protected:
            virtual bool  run(const int32_t cur_thread_id,const uint64_t timenow_ms) override;
            
            //interface xtimersink_t
            virtual bool            on_timer_start(const int32_t errorcode,const int32_t thread_id,const int64_t timer_id,const int64_t cur_time_ms,const int32_t timeout_ms,const int32_t timer_repeat_ms) override;   //attached into io-thread
            virtual bool            on_timer_stop(const int32_t errorcode,const int32_t thread_id,const int64_t timer_id,const int64_t cur_time_ms,const int32_t timeout_ms,const int32_t timer_repeat_ms) override;   //detach means it detach
            virtual bool            on_timer_fire(const int32_t thread_id,const int64_t timer_id,const int64_t current_time_ms,const int32_t start_timeout_ms,int32_t & in_out_cur_interval_ms) override;
        private:
            xtimer_t*    m_monitor_timer;
            std::vector<xsysobject_t*>  m_boot_objects;
        };
    
    }//end of namespace of base
}//end of namespace top
