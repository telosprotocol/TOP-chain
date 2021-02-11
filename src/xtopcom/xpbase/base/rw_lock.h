//
//  rw_lock.h
//
//  Created by Charlie Xie on 02/03/2019.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//

#pragma once

#ifndef _WIN32
#include <pthread.h>
#endif
#include <mutex>

#include "xpbase/base/top_utils.h"

namespace top {

namespace base {

#ifndef _WIN32

class ReadWriteLock {
public:
    ReadWriteLock() {
        pthread_rwlock_init(&lock_, NULL);
    }

    ~ReadWriteLock() {
        pthread_rwlock_destroy(&lock_);
    }

    void WriteLock() {
        pthread_rwlock_wrlock(&lock_);
    }

    void ReadLock() {
        pthread_rwlock_rdlock(&lock_);
    }

    void ReadUnlock() {
        pthread_rwlock_unlock(&lock_);
    }

    void WriteUnlock() {
        pthread_rwlock_unlock(&lock_);
    }

private:
    pthread_rwlock_t lock_;

    DISALLOW_COPY_AND_ASSIGN(ReadWriteLock);
};

#else

class ReadWriteLock {
public:
    ReadWriteLock() : reader_count_(0), writer_used_(false), mutex_(), cond_var_() {}
    ~ReadWriteLock() {}

    void ReadLock() {
        std::unique_lock<std::mutex> lock(mutex_);
        while (writer_used_ == true) {
            cond_var_.wait(lock);
        }
        ++reader_count_;
    }

    void WriteLock() {
        std::unique_lock<std::mutex> lock(mutex_);
        while (reader_count_ != 0 || writer_used_ == true) {
            cond_var_.wait(lock);
        }
        writer_used_ = true;
    }

    void ReadUnlock() {
        std::unique_lock<std::mutex> lock(mutex_);
        --reader_count_;
        if (reader_count_ == 0) {
            cond_var_.notify_all();
        }
    }

    void WriteUnlock() {
        std::unique_lock<std::mutex> lock(mutex_);
        writer_used_ = false;
        cond_var_.notify_all();
    }

private:
    int reader_count_;
    bool writer_used_;
    std::mutex mutex_;
    std::condition_variable cond_var_;

    DISALLOW_COPY_AND_ASSIGN(ReadWriteLock);
};

#endif

class ReadLock {
public:
    explicit ReadLock(ReadWriteLock& lock) : lock_(lock) {
        lock_.ReadLock();
    }

    ~ReadLock() {
        lock_.ReadUnlock();
    }

private:
    ReadWriteLock& lock_;

    DISALLOW_COPY_AND_ASSIGN(ReadLock);
};

class WriteLock {
public:
    explicit WriteLock(ReadWriteLock& lock) : lock_(lock) {
        lock_.WriteLock();
    }

    ~WriteLock() {
        lock_.WriteUnlock();
    }

private:
    ReadWriteLock& lock_;

    DISALLOW_COPY_AND_ASSIGN(WriteLock);
};

}  // namespace base

}  // namespace top
