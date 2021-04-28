#include "xratelimit_cache.h"



NS_BEG2(top, xChainRPC)

static uint64_t GetSteadyClockMS() {
    auto now_time = std::chrono::steady_clock::now();
    uint64_t now = (std::chrono::duration_cast<
        std::chrono::milliseconds>(now_time.time_since_epoch())).count();
    return now;
}

bool RateLimitCheckUseBucket::Consume() {
    return bucket_.consume(request_token_,
        GetSteadyClockMS(), allow_borrow_size_) > 0;
}

void RateLimitCheckUseBucket::ResetBucket(uint32_t rate_per_second,
    uint32_t burst_size, int64_t timenow_ms, int32_t allow_borrow_size) {
    bucket_.reset(rate_per_second, burst_size, timenow_ms);
    allow_borrow_size_ = allow_borrow_size;
}

RatelimitCache::RatelimitCache(const RatelimitConfig* config)
    : config_(config)
    , thread_(std::bind(&RatelimitCache::ThreadFunc, this)) {
    clean_overtime_s_ = (config_->GetCacheTimeoutSecond() == 0) ?
        config_->GetCacheTimeoutSecond() : INT32_MAX;
}

RatelimitCache::~RatelimitCache() {
    break_out_ = true;
    cv_.notify_all();
    thread_.join();
    // ip_hashmap_.Foreach([](int, IpCheckContent * p) { delete p; });
    // account_hashmap_.Foreach([](std::string, AccountCheckContent * p) { delete p; });
}

bool RatelimitCache::CheckIpRelease(uint32_t ip) {
    auto content = GetIpContent(ip);
    if (content) {
        content->steady_time_ = GetSteadyClockMS();
        return content->Consume();
    }
    return false;
}

bool RatelimitCache::CheckAccountRelease(const std::string & account) {
    if (account.empty())
        return true;

    auto content = GetAccountContent(account);
    if (content) {
        content->steady_time_ = GetSteadyClockMS();
        return content->Consume();
    }
    return false;
}

bool RatelimitCache::CheckAssetRelease(const std::string & account) {
    if (account.empty())
        return true;

    auto content = GetAccountContent(account);
    if (content) {
        content->steady_time_ = GetSteadyClockMS();
        return content->Asset();
    }
    return false;
}

std::shared_ptr<IpCheckContent> RatelimitCache::GetIpContent(uint32_t ip) {
    std::shared_ptr<IpCheckContent> value;
    auto ret = ip_hashmap_.Find(ip, value);
    if (!ret) {
        value = std::make_shared<IpCheckContent>(config_->GetRatePerSecond(),
            config_->GetBurstSize(), GetSteadyClockMS());
        value->SetAllowBorrowSize(config_->GetAllowBorrowSize());
        value->SetRequestToken(config_->GetRequestToken());
        ret = ip_hashmap_.Insert(ip, value);
        if (!ret) {
            ip_hashmap_.Find(ip, value);
        }
    }
    return value;
}

std::shared_ptr<AccountCheckContent>
RatelimitCache::GetAccountContent(const std::string & account) {
    std::shared_ptr<AccountCheckContent> value;
    auto ret = account_hashmap_.Find(account, value);
    if (!ret) {
        value = std::make_shared<AccountCheckContent>(
            config_->GetRatePerSecond(),
            config_->GetBurstSize(),
            GetSteadyClockMS());
        value->SetAllowBorrowSize(config_->GetAllowBorrowSize());
        value->SetRequestToken(config_->GetRequestToken());
        ret = account_hashmap_.Insert(account, value);
        if (!ret) {
            account_hashmap_.Find(account, value);
        }
    }
    return value;
}

void RatelimitCache::ThreadFunc() {
    while (!break_out_) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait_for(lock, std::chrono::seconds(second_interval_));
        CleanIdle();
    }
}

void RatelimitCache::CleanIdle() {
    uint64_t now = GetSteadyClockMS();
    account_hashmap_.ForeachErase(
        [this, now](std::string,
            std::shared_ptr<AccountCheckContent> p)->bool {
        if (p != nullptr) {
            if (p->state_ == RateLimitCheck::State::Refused) {
                if (now < p->refused_time_)
                    return false;
            }
            return (now - p->steady_time_ > clean_overtime_s_ * 1000);
        } else {
            return true;
        }
        });
}

void RatelimitCache::ResetIpCheckBucket(uint32_t ip) {
    std::shared_ptr<IpCheckContent> content = GetIpContent(ip);
    content->ResetBucket(config_->GetRatePerSecond(),
        config_->GetBurstSize(), GetSteadyClockMS(),
        config_->GetAllowBorrowSize());
}

void RatelimitCache::ResetAccountCheckBucket(const std::string & account) {
    std::shared_ptr<AccountCheckContent> content = GetAccountContent(account);
    content->ResetBucket(config_->GetRatePerSecond(),
        config_->GetBurstSize(), GetSteadyClockMS(),
        config_->GetAllowBorrowSize());
}

NS_END2
