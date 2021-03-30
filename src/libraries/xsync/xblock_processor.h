#pragma once

#include "xbasic/xns_macro.h"
#include "xsync/xsync_tableunit.h"
#include "xsync/xchain_info.h"

NS_BEG2(top, sync)

class xblock_processor_t {
public:
    xblock_processor_t(std::string vnode_id,
        const observer_ptr<mbus::xmessage_bus_face_t> &mbus);

public:
    void handle_block(const mbus::xevent_ptr_t& e);

private:
    observer_ptr<mbus::xmessage_bus_face_t> m_mbus{};
};

NS_END2
