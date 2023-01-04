// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xvcanvas.h"

namespace top
{
    namespace base
    {
        //execute object present a logic container where execute script code
        class xvexeunit_t : public xdataunit_t
        {
            friend class xvexegroup_t;
        protected:
            typedef std::function< const xvalue_t (const xvmethod_t & op,xvcanvas_t * canvas) > xvstdfunc_t;
        public:
            static  const std::string   name(){ return std::string("xvexeunit");}
            virtual std::string         get_obj_name() const override {return name();}
            enum{enum_obj_type = enum_xobject_type_exe_unit};//allow xbase create xvproperty_t object from xdataobj_t::read_from()
            
        protected://not open for public
            xvexeunit_t(enum_xdata_type type);
            xvexeunit_t(xvexeunit_t * parent_unit,const std::string & unit_name,enum_xdata_type type);
            xvexeunit_t(const xvexeunit_t & obj);
            virtual ~xvexeunit_t();
            
        private://not implement those contruction
            xvexeunit_t();
            xvexeunit_t(xvexeunit_t &&);
            xvexeunit_t & operator = (const xvexeunit_t & obj);
            
        public:
            virtual void*               query_interface(const int32_t _enum_xobject_type_) override;//caller cast (void*) to related ptr
            virtual bool                close(bool force_async = false) override;
            virtual xvexeunit_t*        clone() = 0;

        public:
            //call instruction(operator) with related arguments
            virtual const xvalue_t      execute(const xvmethod_t & op,xvcanvas_t * canvas);//might throw exception for error
            inline  const xvalue_t      nil_impl(const xvmethod_t & op) const {return xvalue_t();}
            const xvalue_t              not_impl(const xvmethod_t & op) const;
            
        public:
            inline  const std::string&  get_unit_name()     const {return m_unit_name;}
            inline  const std::string&  get_execute_uri()   const {return m_execute_uri;}
            inline  xvexeunit_t*        get_parent_unit()   const {return m_parent_unit;}
            int32_t                     get_ex_alloc_size() const;
             
        protected:
            bool                 register_method(const uint8_t method_type,const uint8_t method_id,const xvstdfunc_t & api_function);
            bool                 register_method(const uint8_t method_type,const std::string & method_name,const xvstdfunc_t & api_function);
            virtual void         set_unit_name(const std::string & name);
            virtual void         set_parent_unit(xvexeunit_t * parent_ptr);
            
        private:
            std::string           m_unit_name;   //name for this execution unit
            std::string           m_execute_uri; //uniform execution resource identify
            xvexeunit_t*          m_parent_unit; //chain-mode to execute operateion
            std::map<int, xvstdfunc_t>          m_id_methods; //for static & predefined methods
            std::map<std::string, xvstdfunc_t>  m_name_methods; //for dynamic & runtime methods
        };
        
        //group container that hold child exeunits,like vblockstate hold many properties
        class xvexegroup_t : public xvexeunit_t
        {
        protected:
            xvexegroup_t(enum_xdata_type type);
            xvexegroup_t(const xvexegroup_t & obj);
            xvexegroup_t(xvexeunit_t * parent_unit,const std::string unit_name,enum_xdata_type type);
            virtual ~xvexegroup_t();
            
        private://not implement those contruction
            xvexegroup_t();
            xvexegroup_t(xvexegroup_t &&);
            xvexegroup_t & operator = (const xvexegroup_t & obj);
            
        public:
            //call instruction(operator) with related arguments
            virtual const xvalue_t  execute(const xvmethod_t & op,xvcanvas_t * canvas) override;//might throw exception for error
            virtual bool            close(bool force_async = false) override;
            //virtual xvexeunit_t*    clone() override;
            std::recursive_mutex&   get_mutex() const {return m_lock;}

            int32_t                 get_ex_alloc_size() const;
            
        protected:
            bool                clone_units_from(const xvexegroup_t & source);
            bool                add_child_unit(xvexeunit_t * child);
            bool                remove_child_unit(const std::string & unit_name);
            bool                remove_all_child_unit();
            xvexeunit_t *       find_child_unit(const std::string & unit_name) const;
            const int           get_childs_count() const {return (int)m_child_units.size();}
            const std::map<std::string,xvexeunit_t*> & get_child_units() const {return m_child_units;}
            virtual void        set_parent_unit(xvexeunit_t * parent_ptr) override;
        protected:
            virtual int32_t     do_write(xstream_t & stream) override; //allow subclass extend behavior
            virtual int32_t     do_read(xstream_t & stream)  override; //allow subclass extend behavior
        private:
            mutable std::recursive_mutex m_lock;
            std::map<std::string,xvexeunit_t*> m_child_units;
        };
    
        //convenient macro to register vfunction/api by method id
        #define BEGIN_DECLARE_XVIFUNC_ID_API(_category) template<typename _T, enum_xvinstruct_class category=_category > void register_xvfunct_id_api_internal##_category(_T * pThis){
        #define IMPL_XVIFUNCE_ID_API(methodid,entry) register_method((const uint8_t)category,(const uint8_t)methodid,std::bind(&_T::entry,pThis,std::placeholders::_1,std::placeholders::_2));
        #define END_DECLARE_XVIFUNC_ID_API(_category) }
        #define REGISTER_XVIFUNC_ID_API(_category) register_xvfunct_id_api_internal##_category(this)
            
            //convenient macro to register vfunction/api by method name
        #define BEGIN_DECLARE_XVIFUNC_NAME_API(_category) template<typename _T,enum_xvinstruct_class category=_category> void register_xvifunction_name_api_internal##_category(_T * pThis){
        #define IMPL_XVIFUNC_NAME_API(funcname,entry) register_method((const uint8_t)category,funcname, std::bind(&_T::entry,pThis,std::placeholders::_1,std::placeholders::_2));
        #define END_DECLARE_XVIFUNC_NAME_API(_category) }
        #define REGISTER_XVIFUNC_NAME_API(_category) register_xvifunction_name_api_internal##_category(this)

    }
}
