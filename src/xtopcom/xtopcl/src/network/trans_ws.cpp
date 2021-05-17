#include "trans_ws.h"

#include <stdint.h>
#include <string>
#include <iostream>
#include <functional>
#include <system_error>

#include "task/task_dispatcher.h"
#include "api_method.h"
#include "client_http.hpp"


namespace xChainSDK
{
	trans_ws::trans_ws(const std::string& host)
		: trans_base(host)
		, client(_host)
		, thread_(&trans_ws::thread_func, this)
	{
		client.on_message = [this](std::shared_ptr<WsClient::Connection> connection, std::shared_ptr<WsClient::InMessage> in_message) {
			ResponseContent* cont = new ResponseContent();
			cont->content = in_message->string();
			task_dispatcher::get_instance()->post_message(msgResponse, (uint32_t*)cont, 0);
//			task_dispatcher::get_instance()->handle_response(cont);
//			connection->send_close(1000);

			connection_ = connection;
		};

		client.on_open = [this](std::shared_ptr<WsClient::Connection> connection) {
			std::cout << "Client: Opened connection" << std::endl;
			connection->send(content_);
		};

		client.on_close = [this](std::shared_ptr<WsClient::Connection> /*connection*/, int status, const std::string& /*reason*/) {
			std::cout << "Client: Closed connection with status code " << status << std::endl;
			connection_ = nullptr;
		};

		client.on_error = [](std::shared_ptr<WsClient::Connection> /*connection*/, const SimpleWeb::error_code & ec) {
			std::cout << "Client: Error: " << ec << ", error message: " << ec.message() << std::endl;
		};
	}

	trans_ws::~trans_ws()
	{
		if (connection_ != nullptr)
			connection_->send_close(1000);
		thread_.join();
		std::cout << "delete trans_ws" << std::endl;
	}

	int trans_ws::do_trans(const std::string& content)
	{
		content_ = content;

		if (connection_ != nullptr)
		{
			connection_->send(content_);
		}
		return 0;
	}
	void trans_ws::thread_func()
	{
		client.start();
	}
	std::shared_ptr<trans_ws> trans_ws::get_ptr()
	{
		return shared_from_this();
	}
}