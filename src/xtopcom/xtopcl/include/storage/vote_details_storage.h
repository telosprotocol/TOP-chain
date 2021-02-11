#pragma once

#include <memory>

#ifndef _WIN32
#include "hiredis/hiredis.h"
#endif
#include "topchain_type.h"

namespace xChainSDK
{
	struct Ticket
	{
		std::string from;
		std::string to;
		uint32_t        count;
	};

	class vote_details_storage
	{
	public:
		static vote_details_storage* get_instance();
		static void destory_instance();

#ifndef _WIN32
		void init(redisContext* redis_context);
#endif

		int add_details(const VoteDetailsResult& detail);

		int get_tickets(uint32_t turns, std::vector<Ticket>& tickets);

	private:
		vote_details_storage();
		~vote_details_storage();

		static vote_details_storage* s_instance;

#ifndef _WIN32
		std::shared_ptr<redisContext> redis_context_;
#endif
	};
}