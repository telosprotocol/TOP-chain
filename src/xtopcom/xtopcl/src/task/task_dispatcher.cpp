
#include "task_dispatcher.h"

#include "network/trans_http.h"
#include "network/trans_ws.h"
#include "protocol.h"
#include "user_info.h"
#include "xrpc/xuint_format.h"

#include <assert.h>

extern xChainSDK::user_info g_userinfo;

namespace xChainSDK {
using namespace top::xrpc;
using namespace top::data;
using std::cout;
using std::endl;
task_dispatcher * task_dispatcher::s_instance = nullptr;
std::vector<std::string> task_dispatcher::seeds;
std::uint16_t task_dispatcher::index = 0;
std::once_flag task_dispatcher::m_once_init_flag;

task_dispatcher::task_dispatcher() : seed_client(g_edge_domain) {
    seeds.push_back(g_server_host_port);
    auto rtn = seed_client.GetEdgeSeeds(seeds);
#ifdef DEBUG
    if (!rtn) {
        std::cout << "[debug]get seeds from " << SEED_URL << " failed" << std::endl;
    } else {
        std::cout << "[debug]get " << seeds.size() << " seeds from " << SEED_URL << " successed" << std::endl;
    }
#endif
}

task_dispatcher::~task_dispatcher() {
    task_queue.clear();
}

task_dispatcher * task_dispatcher::get_instance() {
    std::call_once(m_once_init_flag, [&]() {
        s_instance = new task_dispatcher();
        s_instance->init_thread();
    });

    if (trans_base::_create_funcs.size() == 0) {
#ifdef DEBUG
        std::cout << "[debug]task_dispatcher init" << std::endl;
#endif
        s_instance->init();
    }

    return s_instance;
}
void task_dispatcher::destory_instance() {
    delete s_instance;
}

void task_dispatcher::init() {
    regist_transmode();
}

void task_dispatcher::init_thread() {
    thread_ = std::thread(&task_dispatcher::thread_func, this);
}

void task_dispatcher::regist_transmode() {
    trans_base::regist_create_function(TransMode::HTTP, [](const std::string & host) { return std::make_shared<trans_http>(host); });

    trans_base::regist_create_function(TransMode::WS, [this](const std::string & host) { return get_ws_trans(host); });
}

int task_dispatcher::add_task(request_task::TaskInfoPtr info) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        uint32_t id = info->task_id;
        task_queue[id] = RequestTaskPtr(new request_task(info));
    }
    dispatch_task();
    return 0;
}

void task_dispatcher::dispatch_task() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = task_queue.begin();
    for (; it != task_queue.end(); ++it) {
        RequestTaskPtr task = it->second;
        if (!task->is_running()) {
            task->do_task();
            break;
        }
    }
}

void task_dispatcher::handle_response(const std::string & result) {
    std::lock_guard<std::mutex> lock(mutex_);
    uint32_t task_id = protocol::decode_task_id(result);

    if (result == SERVER_ERROR && (++index < seeds.size())) {
        std::cout << g_server_host_port << ": " << result << std::endl;
        request_task task(nullptr);
        g_server_host_port = seeds[index];
        std::cout << "Try another one: " << g_server_host_port << std::endl;
        task.repeat_task_with_new_server(seeds[index]);
        return;
    }

    if (static_cast<int>(task_id) != 0) {
        auto it = task_queue.find(task_id);
        if (it != task_queue.end()) {
            RequestTaskPtr task = it->second;
            assert(task != nullptr && task->is_running());
            push_result_queue(result, task);
            task_queue.erase(it);
            // task->handle_result(result);
        }
    } else {
        result_queue_.push(result);
        // std::cout << "NO ID ERROR: " << result.c_str() << std::endl;
    }
}

void task_dispatcher::handle_vote_details_result(VoteDetailsResultPtr result) {
}

void task_dispatcher::handle_candidate_details_result(CandidateDetailsResultPtr result) {
}

void task_dispatcher::handle_dividend_details_result(DividendDetailsResultPtr result) {
}

void task_dispatcher::handle_request_token_result(RequestTokenResultPtr result) {
    stat_.response_count_++;
    if (result->error == static_cast<int>(ErrorCode::Success)) {
        stat_.response_succ_count_++;
        g_userinfo.identity_token = result->_token;
        g_userinfo.secret_key = result->_secret_key;
        g_userinfo.sign_method = result->_sign_method;
        g_userinfo.sign_version = result->_sign_version;
    } else {
        stat_.response_fail_count_++;
    }
    stat_.check_over();
}
void task_dispatcher::handle_create_account_result(CreateAccountResultPtr result) {
    if (result->error == static_cast<int>(ErrorCode::Success)) {
    }
}

void task_dispatcher::handle_account_info_result(AccountInfoResultPtr result) {
    if (result->error == static_cast<int>(ErrorCode::Success)) {
        if (g_userinfo.account == result->_account) {
#ifdef DEBUG
// std::cout << "seq: " << result->sequence_id << " success:" << result->error << std::endl;
#endif
            g_userinfo.account = result->_account;
            g_userinfo.last_hash = result->_last_hash;
            g_userinfo.balance = result->_balance;
            g_userinfo.nonce = result->_nonce;
            g_userinfo.last_hash_xxhash64 = result->_last_hash_xxhash64;
        }
    } else {
#ifdef DEBUG
// std::cout << "seq: " << result->sequence_id << " errno: " << result->errmsg.c_str() << "................................................................." << std::endl;
#endif
    }
}

void task_dispatcher::handle_account_transaction_result(AccountTransactionResultPtr result) {
}
void task_dispatcher::handle_transfer_result(TransferResultPtr result) {
    if (result->error == static_cast<int>(ErrorCode::Success)) {
        // std::cout << "seq: " << result->sequence_id << " success:" << result->error << std::endl;
    } else if (result->error == static_cast<int>(ErrorCode::SignError)) {
        // std::cout << "seq: " << result->sequence_id << " errno: " << result->errmsg.c_str() << "................................................................." << std::endl;
    } else {
        // std::cout << "seq: " << result->sequence_id << " errno: " << result->error << std::endl;
    }
}

void task_dispatcher::handle_get_property_result(GetPropertyResultPtr result) {
    if (result->_type == "list") {
        if (result->_data1 == "@18") {
            for (auto val : result->_values) {
                xproperty_vote v;
                top::base::xstream_t stream(top::base::xcontext_t::instance(), (uint8_t *)val.data(), val.size());
                v.serialize_read(stream);
                std::string hash_str = uint_to_str(v.m_lock_hash.data(), v.m_lock_hash.size());
                std::cout << "m_lock_hash: " << hash_str.c_str() << ", m_amount: " << v.m_amount << ", m_available: " << v.m_available << ", m_expiration: " << v.m_expiration
                          << std::endl;
            }
        } else if (result->_data1 == "@19") {
            for (auto val : result->_values) {
                xproperty_vote_out v;
                top::base::xstream_t stream(top::base::xcontext_t::instance(), (uint8_t *)val.data(), val.size());
                v.serialize_read(stream);
                std::string hash_str = uint_to_str(v.m_lock_hash.data(), v.m_lock_hash.size());
                std::cout << "m_address:" << v.m_address.c_str() << ", m_lock_hash: " << hash_str.c_str() << ", m_amount: " << v.m_amount << ", m_expiration: " << v.m_expiration
                          << std::endl;
            }
        } else if (result->_data1 == "@20") {
            for (auto val : result->_values) {
                xvote_info v;
                top::base::xstream_t stream(top::base::xcontext_t::instance(), (uint8_t *)val.data(), val.size());
                v.serialize_read(stream);
                std::string hash_str = uint_to_str(v.m_lock_hash.data(), v.m_lock_hash.size());
                std::cout << "m_addr_from:" << v.m_addr_from.c_str() << ", m_addr_to: " << v.m_addr_to.c_str() << ", m_lock_hash: " << hash_str.c_str()
                          << ", m_amount: " << v.m_amount << ", m_expiration: " << v.m_expiration << std::endl;
            }
        }
    }
}

std::shared_ptr<trans_ws> task_dispatcher::get_ws_trans(const std::string & host) {
    if (ws_trans_ == nullptr) {
        ws_trans_ = std::make_shared<trans_ws>(host);
    }
    return ws_trans_->get_ptr();
}

void task_dispatcher::thread_func() {
    Msg msg;
    while (msg_queue_.pluck(msg)) {
        on_message(msg.id, msg.param1, msg.param2);
    }
    return;
}
void task_dispatcher::post_message(uint32_t id, uint32_t * param1, uint32_t * param2) {
    Msg msg{id, param1, param2};
    msg_queue_.push(msg);
}

void task_dispatcher::on_message(uint32_t id, uint32_t * param1, uint32_t * param2) {
    switch (id) {
    case msgResponse: {
        ResponseContent * cont = (ResponseContent *)param1;
        handle_response(cont->content);
        delete cont;
    } break;

    case msgAddTask: {
        task_info * info = (task_info *)param1;
        add_task(request_task::TaskInfoPtr(info));
        stat_.request_count_++;
    } break;

    default:
        break;
    }
}

std::string task_dispatcher::get_result() {
    std::string result;
    while (result_queue_.pluck(result)) {
        if (result == SERVER_ERROR) {
            index = (index + 1) % seeds.size();
#ifdef DEBUG
            std::cout << "[debug]changing server, old " << g_server_host_port << ", new " << seeds[index] << " , please resend your request" << std::endl;
#endif
            g_server_host_port = seeds[index];
        }
        return result;
    }
    return "empty result";
}

void task_dispatcher::push_result_queue(std::string const & result, RequestTaskPtr & task) {
    xJson::Reader reader;
    xJson::Value root;
    try {
        reader.parse(result, root);
    } catch (...) {
        root[ERROR_MSG] = "Json Parse Error";
    }

    auto styled_result = root.toStyledString();

    task->handle_result(styled_result);
    result_queue_.push(styled_result);
}
}  // namespace xChainSDK
