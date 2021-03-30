#include "xelect_common/elect_option.h"
#include "xpbase/base/top_log.h"
#include <atomic>

namespace top {
namespace elect {

struct ElectOption::Impl {
    std::atomic<bool> is_enable_staticec{false};
};

ElectOption::ElectOption() {
    impl_ = std::make_shared<Impl>();
}

ElectOption* ElectOption::Instance() {
    static ElectOption ins;
    return &ins;
}

bool ElectOption::IsEnableStaticec() {
    return impl_->is_enable_staticec;
}

void ElectOption::EnableStaticec() {
    if (!impl_->is_enable_staticec) {
        impl_->is_enable_staticec = true;
        TOP_FATAL("enable staticec");
    } else {
        TOP_FATAL("staticec enabled before, ignore!");
    }
}

}
}
