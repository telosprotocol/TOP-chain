#pragma once

#ifndef XCHAIN_TRANS_HTTP
#define XCHAIN_TRANS_HTTP

#include <memory>
#include <thread>
#include <string>

#include "trans_base.h"
#include "client_http.hpp"

namespace xChainSDK
{
	using HttpClient = SimpleWeb::Client<SimpleWeb::HTTP>;

	class trans_http : public trans_base
	{
	public:
		trans_http(const std::string& host);
		virtual ~trans_http();

		virtual int do_trans(const std::string& content);

		// void thread_func();

	private:
		HttpClient client;
		// std::thread thread_;
		// std::mutex mutex_;
		// std::condition_variable cv_;
		bool shut_down_{ false };
        // std::atomic_int notify_count_{ 0 };
	};
}

#endif // !XCHAIN_TRANS_HTTP
