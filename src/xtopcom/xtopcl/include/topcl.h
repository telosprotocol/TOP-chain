// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once

#include "api_method.h"
#include "task_dispatcher.h"
#include "trans_base.h"
#include "user_info.h"
#include "xbase/xns_macro.h"
#include "xcrypto/xckey.h"

//#include <readline/history.h>
//#include <readline/readline.h>
#include <stdio.h>
#include <stdlib.h>

#include <algorithm>
#include <csignal>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

NS_BEG2(top, xtopcl)

class xtopcl final {
public:
    xChainSDK::ApiMethod api;
};

NS_END2
