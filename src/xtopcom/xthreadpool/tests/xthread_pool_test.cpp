#include "gtest/gtest.h"
#include "src/xthread_pool.h"
#include "src/xthread_task_container.h"
#include "src/xthread_pool_manager.h"
#include <iostream>
#include <vector>
#include <set>
#include <tuple>
#include "src/xthread_apply.h"

static bool is_check_contract = true;
static bool s_failed = false;
static std::mutex s_mutex;
static std::condition_variable s_cv;
static std::atomic_int result;

using namespace top::xthreadpool;

using TaskContent = xthread_task_container::TaskContent;

class contract_task_mock : public xthread_task {
public:
    using AccountMap = std::map<std::string, std::list<uint32_t>>;

    contract_task_mock() {}
    ~contract_task_mock() {}

    void set_account_name(const std::string& account) {
        account_name_ = account;
    }

    void set_contract_name(const std::string& contract) {
        contract_name_ = contract;
    }

    void do_task() {
        print_in();
        usleep(1000 * 200);
        print_out();

        result++;
        s_cv.notify_one();
    }

    void print() {
        // std::lock_guard<std::mutex> lock(mutex_);
        auto thread_id = std::this_thread::get_id();
        std::cout << contract_name_.c_str() << "-" << account_name_.c_str() 
            << ":id=" << idx_ << ", thread_id=" << thread_id << std::endl;
    }

    void print_in() {
        std::lock_guard<std::mutex> lock(mutex_);
        cur_running_++;
        if (max_running_ < cur_running_) max_running_ = cur_running_;
        insert_account_id(account_name_, idx_);

        if (accounts_.find(account_name_) != accounts_.end()) {
            s_failed = true;
            std::cout << "!!!!error!!!!" << std::endl;
        } else if (contracts_.find(contract_name_) != contracts_.end() &&
            is_check_contract) {
            s_failed = true;
            std::cout << "!!!!error!!!!" << std::endl;
        }

        auto thread_id = std::this_thread::get_id();
        std::cout << thread_id << ": [ IN]id=" << idx_ << " - " 
            << contract_name_.c_str() << ":" << account_name_.c_str() << std::endl;

        accounts_.insert(account_name_);
        contracts_.insert(contract_name_);
    }

    void print_out() {
        std::lock_guard<std::mutex> lock(mutex_);

        auto thread_id = std::this_thread::get_id();
        std::cout << thread_id << ": [OUT]id=" << idx_ << " - " 
            << contract_name_.c_str() << ":" << account_name_.c_str() << std::endl;

        auto it = accounts_.find(account_name_);
        if (it != accounts_.end()) accounts_.erase(it);
        auto it2 = contracts_.find(contract_name_);
        if (it2 != contracts_.end()) contracts_.erase(it2);

        cur_running_--;
    }

    void insert_account_id(const std::string & account, uint32_t id) {
        auto it = account_ids_.find(account);
        if (it != account_ids_.end()) {
            std::list<uint32_t>& l = it->second;
            l.push_back(id);
        } else {
            std::list<uint32_t> l;
            l.push_back(id);
            account_ids_.insert(AccountMap::value_type(account, l));
        }
    }

    static void print_account_ids() {
        for (auto it : account_ids_) {
            std::cout << it.first.c_str() << ": ";
            for (auto val : it.second) {
                std::cout << val << ", ";
            }
            std::cout << std::endl;
        }

        std::cout << "Max Running: " << max_running_ << std::endl;
    }

    std::mutex mutex_;
    uint32_t idx_;
    std::string account_name_;
    std::string contract_name_;

    static std::set<std::string> accounts_;
    static std::set<std::string> contracts_;
    static AccountMap account_ids_;
    static size_t cur_running_;
    static size_t max_running_;
};

std::set<std::string> contract_task_mock::accounts_;
std::set<std::string> contract_task_mock::contracts_;
contract_task_mock::AccountMap contract_task_mock::account_ids_;
size_t contract_task_mock::cur_running_ = 0;
size_t contract_task_mock::max_running_ = 0;



class test_thread_method {
public:
    test_thread_method() {}
    ~test_thread_method() {}

    void func1(int a, int b) {
        std::cout << "call func1(" << a << ", " << b << ");" << std::endl;
    }

    int func2(int a, int b) {
        std::cout << "call func2(" << a << ", " << b << ");" << std::endl;
        return a + b;
    }
};


class contract_caller {
public:
    void callback(int ret) {
        std::cout << "callback, ret=" << ret << std::endl;
        result = ret;
        s_cv.notify_one();
    }
};

static bool thread_callback(int param) {

    std::cout << "thread_callback" << std::endl;
    result = param;
    s_cv.notify_one();
    return true;
}

using type_f = std::function<bool(int)>;

TEST(xthread_pool, TestAddCall) {

    xthread_pool_manager manager;
    int param = 123;
    auto callback = std::bind(thread_callback, param);
    task_call_static<decltype(callback)> call(callback);
    uint64_t account = 0;
    manager.add_call(account, call);
    {
        std::unique_lock<std::mutex> lock(s_mutex);
        s_cv.wait_for(lock, std::chrono::milliseconds(500));
        EXPECT_EQ(result, param);
    }
}

TEST(xthread_pool, TestAddTask) {
    int param1 = 12;
    int param2 = 13;
    uint64_t account = 0;
    xthread_pool_manager manager;
    contract_caller caller;
    test_thread_method inst;
    auto task = std::make_shared<
        contract_task_caller<
        test_thread_method, int, int, int>>(
            inst, &test_thread_method::func2, param1, param2);
    task->set_account(account);
    task->set_callback(std::bind(&contract_caller::callback,
        &caller, std::placeholders::_1));
    manager.add_task(task);
    {
        std::unique_lock<std::mutex> lock(s_mutex);
        s_cv.wait_for(lock, std::chrono::milliseconds(500));
        EXPECT_EQ(result, inst.func2(param1, param2));
    }
}

TEST(xthread_pool, TestFeatures) {
    int contract_count = 10;
    int account_count = 10;
    std::vector<std::string> contracts;
    std::vector<std::string> accounts;
    char buf[32] = { 0 };
    uint32_t idx = 0;
    std::vector<TaskContent> tasks;
    for (auto i = 0; i < contract_count; ++i) {
        snprintf(buf, 32, "C_%03d", i);
        contracts.push_back(std::string(buf));
    }
    for (auto i = 0; i < account_count; ++i) {
        snprintf(buf, 32, "A_%03d", i);
        accounts.push_back(std::string(buf));
    }

    for (auto i = 0; i < contract_count; ++i) {
        for (auto j = 0; j < account_count; j++) {
            auto task = std::make_shared<contract_task_mock>();
            task->set_contract_name(contracts[j]);
            task->set_account_name(accounts[i]);
            task->set_contract(j);
            task->set_account(i);
            task->idx_ = idx++;
            tasks.push_back(task);
            task->print();
        }
    }
    std::cout << "======================" << std::endl;
    result = 0;
    xthread_pool_manager manager;
    is_check_contract = manager.get_thread_context()->is_check_contract();
    manager.add_tasks(tasks);
    
    {
        std::unique_lock<std::mutex> lock(s_mutex);
        if (!s_cv.wait_for(lock, std::chrono::milliseconds(1000 * 20),
            [contract_count, account_count] () {
                return result == contract_count * account_count; })) {
            std::cout << "time out. result = " << result << std::endl;
        }
    }

    contract_task_mock::print_account_ids();

    EXPECT_FALSE(s_failed);
}
