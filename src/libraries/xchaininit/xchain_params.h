// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>

namespace top {

class xchain_params {
public:
    // add by smaug
    void initconfig_using_configcenter();

private:
	int  get_uuid(std::string& uuid);
    void load_user_config();

};
}
