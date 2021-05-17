
#include "trans_base.h"

#include <utility>

namespace xChainSDK
{
	TransMode trans_base::s_defaule_mode = TransMode::HTTP;

	trans_base::CreateFuncMap trans_base::_create_funcs;

	trans_base::trans_base(const std::string& host)
		: _host(host)
		, _uri("/")
	{
	}
	trans_base::~trans_base()
	{
//		std::cout << "destory trans_base." << std::endl;
	}

	void trans_base::set_uri(const std::string& uri)
	{
		_uri = uri;
	}

	std::shared_ptr<trans_base> trans_base::create(TransMode mode, const std::string& host)
	{
		auto it = _create_funcs.find(mode);
		if (it != _create_funcs.end())
		{
			return it->second(host);
		}
		return nullptr;
	}

	void trans_base::regist_create_function(TransMode mode, CreateFunc func)
	{
		_create_funcs.insert(CreateFuncMap::value_type(mode, func));
	}
}