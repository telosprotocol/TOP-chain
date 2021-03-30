#pragma once

#include <memory>

#ifndef _WIN32
#include "hiredis/hiredis.h"
#endif
#include "topchain_type.h"

namespace xChainSDK
{

	class dividend_details_storage
	{
	public:
		static dividend_details_storage* get_instance();
		static void destory_instance();

#ifndef _WIN32
		void init(redisContext* redis_context);
#endif

		int add_details(const DividendDetailsResult& detail);

		int get_total_share(uint32_t& total_share, const std::string& name);
	private:
		dividend_details_storage();
		~dividend_details_storage();

		static dividend_details_storage* s_instance;

#ifndef _WIN32
		std::shared_ptr<redisContext> redis_context_;
#endif
	};
}