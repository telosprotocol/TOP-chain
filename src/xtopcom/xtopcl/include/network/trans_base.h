#pragma once


#ifndef XCHAIN_TRANS_BASE
#define XCHAIN_TRANS_BASE

#include "protocol.h"


namespace xChainSDK
{

    enum class TransMode { HTTP, HTTPS, WS, WSS };

    class trans_base
    {
    public:
        using CreateFunc = std::function<std::shared_ptr<trans_base>(const std::string&)>;
        using CreateFuncMap = std::map<TransMode, CreateFunc>;

        trans_base(const std::string& host);
        virtual ~trans_base();

        virtual int do_trans(const std::string& content) = 0;
        void set_uri(const std::string& uri);

        static std::shared_ptr<trans_base> create(TransMode mode, const std::string& host);
        static void regist_create_function(TransMode mode, CreateFunc func);
        static CreateFuncMap _create_funcs;

        static TransMode s_defaule_mode;
    protected:
        std::string _host;
        std::string _uri;
    };
}


#endif // !XCHAIN_TRANS_BASE
