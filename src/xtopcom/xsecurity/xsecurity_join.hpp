#pragma once

#include "xpbase/base/top_timer.h"
#include "xpbase/base/manager_template.h"
#include "xkad/routing_table/node_info.h"

namespace top {
namespace security {
const int32_t kFrozenPeriod = 30 * 1000 * 1000;  // 30s
class XSecurityJoin : public ManagerTemplate<std::string,bool> {
public:
    XSecurityJoin(base::TimerManager* timer_manager) {
        timer_manager_ = timer_manager;
    }
    ~XSecurityJoin() {}
    void Frozen(const std::string& xid) {
        if(!AddData(xid,false)) {
            return;
        }

        timer_ = std::make_shared<base::Timer>(timer_manager_, "XSecurityJoin");
        timer_->CallAfter(kFrozenPeriod, std::bind(&XSecurityJoin::Enable,this,xid));
    }
    bool IsActive(const std::string& xid) {
        bool active;
        if(!FindData(xid,active)) {
            return true;
        }
        return active;
    }
private:
    void Enable(const std::string& xid) {
        DeleteKey(xid);
    }

    base::TimerManager* timer_manager_{nullptr};
    std::shared_ptr<base::Timer> timer_;
};

}
}
