#pragma once
#ifndef XCHAIN_TRANS_WS
#define XCHAIN_TRANS_WS

#include <thread>
#include "trans_base.h"
#include "client_ws.hpp"
#include "thread_queue.h"

namespace xChainSDK
{
	using WsClient = SimpleWeb::SocketClient<SimpleWeb::WS>;


	class trans_ws : public std::enable_shared_from_this<trans_ws>, public trans_base
	{
	public:
		trans_ws(const std::string& host);
		virtual ~trans_ws();

		virtual int do_trans(const std::string& content);

		void thread_func();

		std::shared_ptr<trans_ws> get_ptr();

	private:
		WsClient client;
		std::thread thread_;
//		std::mutex mutex_;
//		std::condition_variable cv_;
//		bool shut_down_{ false };
	public:
		std::string content_;
		std::shared_ptr<WsClient::Connection> connection_{ nullptr };
	};
}
#endif // !XCHAIN_TRANS_WS
