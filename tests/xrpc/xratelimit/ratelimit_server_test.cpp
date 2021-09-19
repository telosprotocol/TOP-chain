#include "gtest/gtest.h"
#include "xrpc/xratelimit/xratelimit_server.h"



using namespace top::xChainRPC;

TEST(RatelimitServerCase, TestServer) {
    uint req_count{ 10 };
    uint resp_count{ 10 };
    RatelimitConfig config;
    RatelimitServer server(config);
    RatelimitServerStat* stat = server.GetStat();
    usleep(1000);
    EXPECT_EQ(stat->request_worker_idles_, config.GetMaxThreads());
    EXPECT_EQ(stat->response_worker_idles_, config.GetMaxThreads());

    std::vector<RatelimitData*> request_datas;
    std::vector<RatelimitData*> response_datas;
    for (uint i{ 0 }; i < req_count; ++i) {
        char num[16] = { 0 };
        snprintf(num, 16, "req-%d", i);
        auto data = new RatelimitData();
        data->type_ = RatelimitData::TypeInOut::kIn;
        data->ip_ = i;
        data->account_ = num;
        data->err_ = 0;
        request_datas.push_back(data);
    }

    for (uint i{ 0 }; i < resp_count; ++i) {
        char num[16] = { 0 };
        snprintf(num, 16, "resp-%d", i);
        auto data = new RatelimitData();
        data->type_ = RatelimitData::TypeInOut::kOut;
        data->ip_ = i;
        data->account_ = num;
        data->err_ = 0;
        response_datas.push_back(data);
    }

    for (auto& d : request_datas) {  // it must be reference
        std::cout << "request in: " << d->ip_ <<
            " " << d->account_ << std::endl;
        server.RequestIn(d);
    }

    for (auto& d : request_datas) {
        std::cout << "responst in: " << d->ip_ <<
            " " << d->account_ << std::endl;
        server.ResponseIn(d);
    }

    usleep(500);

    EXPECT_EQ(stat->inqueue_push_requests_, req_count);
    EXPECT_EQ(stat->inqueue_pop_requests_, req_count);
    EXPECT_EQ(stat->outqueue_push_requests_, req_count);
    EXPECT_EQ(stat->outqueue_pop_requests_, 0);

    EXPECT_EQ(stat->inqueue_push_responses_, resp_count);
    EXPECT_EQ(stat->inqueue_pop_responses_, resp_count);
    EXPECT_EQ(stat->outqueue_push_responses_, resp_count);
    EXPECT_EQ(stat->outqueue_pop_responses_, 0);

    for (std::vector<RatelimitData*>::iterator it = request_datas.begin();
        it != request_datas.end(); ++it) {
        delete (*it);
    }
    request_datas.clear();

    for (std::vector<RatelimitData*>::iterator it = response_datas.begin();
        it != response_datas.end(); ++it) {
        delete (*it);
    }
    response_datas.clear();
}

TEST(RatelimitServerCase, TestServerHelper) {
    std::string ret{ "" };
    std::string param_name{ "target_account_addr=" };
    const size_t buf_size = 512;
    std::string str;
    char buffer[buf_size] = { 0 };
    std::string account = "T-123456789012345678901234567890123";
    snprintf(buffer, buf_size, "target_account_addr=%s& \
        identity_token=d9b13d6e-6c77-4069-a9bd-a14093c93322 \
        &method=getTransaction",
        account.c_str());
    str = buffer;
    memset(buffer, 0, buf_size);
    ret = RatelimitServerHelper::GetPostParam(str, param_name);
    EXPECT_EQ(ret, account);

    snprintf(buffer, buf_size, "method=getTransaction&target_account_addr=%s&identity_token=d9b13d6e-6c77-4069-a9bd-a14093c93322", account.c_str());
    str = buffer;
    memset(buffer, 0, buf_size);
    ret = RatelimitServerHelper::GetPostParam(str, param_name);
    EXPECT_EQ(ret, account);

    snprintf(buffer, buf_size, "method_account_address=getTransaction&target_account_addr=%s&identity_token=d9b13d6e-6c77-4069-a9bd-a14093c93322", account.c_str());
    str = buffer;
    memset(buffer, 0, buf_size);
    ret = RatelimitServerHelper::GetPostParam(str, param_name);
    EXPECT_EQ(ret, account);

    snprintf(buffer, buf_size, "identity_token=d9b13d6e-6c77-4069-a9bd-a14093c93322&method=getTransaction&target_account_addr=%s", account.c_str());
    str = buffer;
    memset(buffer, 0, buf_size);
    ret = RatelimitServerHelper::GetPostParam(str, param_name);
    EXPECT_EQ(ret, account);
}
