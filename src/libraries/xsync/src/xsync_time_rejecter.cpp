#include "xbase/xutl.h"
#include "xsync/xsync_time_rejecter.h"

NS_BEG2(top, sync)

bool xsync_time_rejecter_t::reject(){
    int64_t now = base::xtime_utl::gmttime_ms();

    if ((m_time_ms != 0) && (now - m_time_ms) < m_min_time_interval_ms){
        return true;
    }

    m_time_ms = now;

    return false;
}

NS_END2