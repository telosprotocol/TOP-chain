#pragma once

#include <string>
#include <set>
#include <algorithm>
#include "xbasic/xns_macro.h"


NS_BEG2(top, xthreadpool)

class xthread_context final {
public:
    using Content = uint64_t;

private:
    template<typename T>
    class checker {
        using ContentContainer = std::set<T>;
    
    public:
        explicit checker(bool enable = true) : enable_(enable) {}
        ~checker() {
            content_.clear();
        }

        bool check_access(const T& cont) {
            if (!enable_)
                return true;

            auto it = find(cont);
            return it == std::end(content_);
        }

        void add_check(const T& cont) {
            if (!enable_)
                return;

            insert(cont);
        }

        void release_check(const T& cont) {
            if (!enable_)
                return;

            auto it = find(cont);
            if (it != std::end(content_)) {
                content_.erase(it);
            }
        }

        void clear_check() {
            content_.clear();
        }

    private:
        typename ContentContainer::iterator find(const T& cont) {
            return content_.find(cont);
        }

        void insert(const T& cont) {
            content_.insert(cont);
        }

    private:
        ContentContainer content_;
        bool enable_{ true };
    };

public:
    explicit xthread_context(bool check_contract = false)
        : account_(true)
        , contract_(check_contract)
        , passed_(check_contract)
        , check_contract_(check_contract)
    {}
    ~xthread_context() {}

    bool check_access(const Content& contract, const Content& account) {
        if (contract_.check_access(contract) &&
            account_.check_access(account)) {
            if (passed_.check_access(account))
                return true;
            else
                return false;
        } else {
            passed_.add_check(account);
            return false;
        }
    }

    void add_check(const Content& contract, const Content& account) {
        contract_.add_check(contract);
        account_.add_check(account);
    }

    void release_check(const Content& contract, const Content& account) {
        contract_.release_check(contract);
        account_.release_check(account);
    }

    void check_begin() {
        passed_.clear_check();
    }

    bool is_check_contract() {
        return check_contract_;
    }

private:
    checker<Content> account_;
    checker<Content> contract_;
    checker<Content> passed_;
    bool check_contract_;
};

NS_END2
