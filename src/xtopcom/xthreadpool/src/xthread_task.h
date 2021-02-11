#pragma once

#include "xthread_apply.h"
#include "xbasic/xns_macro.h"

NS_BEG2(top, xthreadpool)

class xthread_task {
    using Content = xthread_context::Content;

public:
    xthread_task() {}
    virtual ~xthread_task() {}
    virtual void do_task() {}

    Content get_contract() { return contract_; }
    Content get_account() { return account_; }

    void set_account(const Content& account) {
        account_ = account;
    }

    void set_contract(const Content& contract) {
        contract_ = contract;
    }

protected:
    Content contract_;
    Content account_;
};

template<typename T, typename R, typename ...Args>
class contract_task_caller : public xthread_task
{
public:
    using func_t = R(T::*)(Args...);
    using type_t = std::tuple<typename std::decay<Args>::type...>;
    using callback_t = std::function<void(R)>;

    contract_task_caller() {}
    contract_task_caller(T& inst, func_t func, Args... args)
        : obj_(&inst), func_(func){
        set_params(args...);
    }
    ~contract_task_caller() {}

    void set_obj(const T& obj) {
        obj_ = &obj;
    }

    void set_func(func_t f) {
        func_ = f;
    }

    void set_callback(callback_t f) {
        callback_ = f;
    }

    void set_params(Args... args) {
        args_ = std::make_tuple(args...);
    }

    virtual void do_task() {
        if (func_ == nullptr)
            return;

        auto f = std::mem_fn(func_);
        ret_ = tuple_apply(f, *obj_, args_);

        if (callback_ != nullptr) {
            callback_(ret_);
        }
    }

    T* obj_;
    func_t func_;
    callback_t callback_;
    type_t args_;
    R ret_;
};

template<typename F>
class task_call_static {
public:
    task_call_static(F func)
        : func_(func)
    {}
    int call() {
        func_();
        return 0;
    }
private:
    F func_;
};

NS_END2
