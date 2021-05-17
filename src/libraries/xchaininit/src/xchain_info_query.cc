#include "xchaininit/xchain_info_query.h"

#include <cassert>
#include <memory>

#include "xpbase/base/top_log.h"


namespace top {

ChainInfo::ChainInfo() {
}

ChainInfo::~ChainInfo() {
    chain_cmd_ = nullptr;
}

ChainInfo* ChainInfo::Instance() {
    static ChainInfo ins;
    return &ins;
}

void ChainInfo::SetChainCmd(const ChainCommandsPtr& chain_cmd) {
    assert(!chain_cmd_);
    chain_cmd_ = chain_cmd;
}

bool ChainInfo::ProcessCommand(const std::string& cmdline, std::string& result) {
    if (!chain_cmd_) {
        TOP_FATAL("chain_cmd invalid");
        result = "topio not ready";
        return false;
    }
    return chain_cmd_->ProcessCommand(cmdline, result);
}

}  //  namespace top
