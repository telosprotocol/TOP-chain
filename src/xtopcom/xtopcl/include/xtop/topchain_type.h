#pragma once

#include <string>
#include <vector>

namespace xChainSDK {
    struct ResultBase {
        int error{0};
        std::string errmsg{""};
        uint32_t sequence_id{0};
    };

	struct VoteDetailsResult : ResultBase {
		struct Vote
		{
			Vote(uint32_t turns,
				const std::string& from,
				const std::string& to,
				uint32_t tickets)
				:_turns(turns),
				_from(from),
				_to(to),
				_tickets(tickets) {}
			~Vote() {}

			uint32_t        _turns;
			std::string _from;
			std::string _to;
			uint32_t        _tickets;
		};
		std::vector<Vote> _votes;
	};

	struct CandidateDetailsResult : ResultBase {
		struct Candidate
		{
			Candidate(uint32_t turns,
				const std::string& name,
				uint32_t tickets,
				float self_rate)
				:_turns(turns),
				_name(name),
				_tickets(tickets),
				_self_rate(self_rate) {}
			~Candidate() {}

			uint32_t        _turns;
			std::string _name;
			uint32_t        _tickets;
			float       _self_rate;
		};
		std::vector<Candidate> _candidates;
	};

	struct DividendDetailsResult : ResultBase {
		struct Dividend
		{
			Dividend(const std::string& name,uint32_t total_share)
				:_name(name),_total_share(total_share) {}
			~Dividend() {}

			std::string _name;
			uint32_t        _total_share;
		};
		std::vector<Dividend> _dividends;
	};

    struct RequestTokenResult : ResultBase {
        std::string _token;
        std::string _secret_key;
        std::string _sign_method;
        std::string _sign_version;
    };

    struct AccountInfoResult : ResultBase {
        //uint32_t _balance;
		uint64_t _balance;
        uint32_t _nonce;
        std::string _account;
        std::string _last_hash;
        uint64_t _last_hash_xxhash64;
    };

    struct GetBlockResult : ResultBase {
        std::string _type;
    };

    struct TransferResult : ResultBase {
        std::string _info;
    };

    struct CreateAccountResult : ResultBase {
        std::string _account;
    };

    struct GetPropertyResult : ResultBase {
        std::string _type{ "" };
        std::string _data1{ "" };
        std::string _data2{ "" };
        std::vector<std::string> _values;
    };

    struct AccountTransactionResult : ResultBase
    {
        struct Action
        {
            std::string _account_addr;
            std::string _action_ext;
            std::string _action_authorization;
            uint32_t _action_hash;
            std::string _action_name;
            std::string _action_param;
            uint32_t _action_size;
            uint32_t _action_type;
        };
        std::string _authorization;
        uint32_t _expire_duration;
        uint32_t _fire_timestamp;
        uint32_t _from_network_id;
        uint32_t _last_trans_hash;
        uint32_t _last_trans_nonce;
        uint32_t _to_network_id;
        uint32_t _trans_random_nounce;
        uint32_t _premium_price;
        std::string _transaction_hash;
        uint32_t _transaction_len;
        uint32_t _transaction_type;
        Action _source_action;
        Action _target_action;
        std::string       challenge_proof;
        std::string       _ext;
    };

    struct CreateContractResult : ResultBase {
    };

    struct PublishContractResult : ResultBase {
    };

    struct CallContractResult : ResultBase {
    };

    struct CreateSubAccountResult : ResultBase {

    };

    struct LockTokenResult : ResultBase {

    };

    struct UnlockTokenResult : ResultBase {

    };

    struct GetVoteResult : ResultBase {

    };

    struct VoteResult : ResultBase {

    };

    struct AbolishVoteResult : ResultBase {

    };

    struct ReturnVoteResult : ResultBase {

    };

    struct NodeRegResult : ResultBase {

    };

    struct SetVoteResult : ResultBase {

    };

	struct ClaimRewardResult : ResultBase {

	};

	struct IssuanceResult : ResultBase {

	};

    struct AddProposalResult : ResultBase {

	};

    struct WithdrawProposalResult : ResultBase {

	};

    struct VoteProposalResult : ResultBase {

	};

    struct AddProposalResult2 : ResultBase {

	};

    struct CancelProposalResult : ResultBase {

	};

    struct ApproveProposalResult : ResultBase {

	};

    struct ChainInfoResult : ResultBase {
        std::string version;
        uint64_t first_timerblock_stamp;
        std::string first_timerblock_hash;
	};

    struct GeneralInfoResult : ResultBase {
    };

    struct NodeInfoResult : ResultBase {

	};

    struct ElectInfoResult : ResultBase {

	};

    struct NodeRewardResult : ResultBase {

	};

    struct VoteDistResult : ResultBase {

	};

    struct VoterRewardResult : ResultBase {

	};

    struct GetProposalResult : ResultBase {

	};

}
