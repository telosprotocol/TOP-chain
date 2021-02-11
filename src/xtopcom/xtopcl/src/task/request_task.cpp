#include "request_task.h"
#include "network/trans_http.h"
#include "global_definition.h"

extern void radis_push(const std::string& key, const std::string& value);

namespace xChainSDK
{
    std::string request_task::_req_content = "";
    request_task::request_task(TaskInfoPtr info)
        : _info(info)
    {
        _is_running = false;
    }

    request_task::~request_task()
    {
//        std::cout << "_trans.use_count: " << _trans.use_count() << std::endl;
//        std::cout << "_protocol.use_count: " << _protocol.use_count() << std::endl;
    }

    bool request_task::is_running()
    {
        return _is_running;
    }

    int request_task::do_task()
    {
        _is_running = true;

        _trans = trans_base::create(trans_base::s_defaule_mode, _info->host);
        _protocol = ProtocolPtr(protocol::create(_info->method));
        if (_info->use_transaction) {
            _protocol->set_transaction(_info->trans_action);
        }

        assert(_trans != nullptr && _protocol != nullptr);

        _req_content = "";
        _protocol->encode(_info->params, _req_content);

        return	_trans->do_trans(_req_content);
    }

    void request_task::repeat_task_with_new_server(const std::string& new_server){
        auto req_cli = trans_base::create(trans_base::s_defaule_mode, new_server);
        req_cli->do_trans(_req_content);
    }

    task_info* request_task::get_task_info() {
        return _info.get();
    }

    void request_task::handle_result(const std::string& result)
    {
        assert(_protocol != nullptr);
        _protocol->decode(result, _info.get());
    }
}
