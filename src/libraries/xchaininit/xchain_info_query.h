#pragma once

#include <string>

#include "xchaininit/xchain_command.h"


namespace top {

class ChainInfo {
public:
    static ChainInfo* Instance();

    void SetChainCmd(const ChainCommandsPtr& chain_cmd);
    bool ProcessCommand(const std::string& cmdline, std::string& result);

private:
    ChainInfo();
    ~ChainInfo();

private:
   ChainCommandsPtr chain_cmd_ {nullptr}; 
};

}  //  namespace top
