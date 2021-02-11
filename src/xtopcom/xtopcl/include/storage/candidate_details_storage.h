#pragma once

#include <memory>

#ifndef _WIN32
#include "hiredis/hiredis.h"
#endif
#include "topchain_type.h"

namespace xChainSDK
{

	struct CandidateDetails
	{
		std::string name;
		uint32_t        tickets;
		float       self_rate;
	};

	class candidate_details_storage
	{
	public:
		static candidate_details_storage* get_instance();
		static void destory_instance();

#ifndef _WIN32
		void init(redisContext* redis_context);
#endif

		int add_details(const CandidateDetailsResult& detail);

		int get_candidae(uint32_t turns,std::vector<CandidateDetails>& candidates);
	private:
		candidate_details_storage();
		~candidate_details_storage();

		static candidate_details_storage* s_instance;

#ifndef _WIN32
		std::shared_ptr<redisContext> redis_context_;
#endif
	};
}