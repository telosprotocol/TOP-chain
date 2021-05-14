#include "xsync/xsync_task.h"
#include "xbase/xutl.h"
#include "xsync/xchain_downloader.h"

NS_BEG2(top, sync)
xsync_command_execute_result xsync_on_blocks_response_command_t::execute(xchain_downloader_t* downloader) {
    return downloader->execute_next_download(m_blocks, m_self_addr, m_target_addr);
}

xsync_command_execute_result xsync_on_snapshot_response_command_t::execute(xchain_downloader_t* downloader) {
    return downloader->execute_next_download(m_snapshot, m_height, m_self_addr, m_target_addr);
}

xsync_command_execute_result xsync_download_command_t::execute(xchain_downloader_t* downloader) {
    return downloader->execute_download(m_expect_height_interval.first, m_expect_height_interval.second, 
            m_sync_policy, m_self_addr, m_target_addr, "time");
}

bool xsync_task_t::start(){
    if (m_finished) {
        m_finished = false;
        int64_t now = base::xtime_utl::gmttime_ms();
        return true;
    }

    return false;
}

bool xsync_task_t::expired(){
    int64_t now = base::xtime_utl::gmttime_ms();
    if ((now - m_current_time_ms) > m_timeout_ms) {
        m_finished = true;
        m_downloader->destroy();
        return true;
    }

    return false;
}

xsync_command_execute_result xsync_task_t::execute(xsync_command_t<xchain_downloader_t *> &cmd) {
    xsync_command_execute_result result = xsync_command_execute_result::abort;
    if (!m_finished){
        result = cmd.execute(m_downloader);
        if (wait_response != result) {
            m_finished = true;
            m_downloader->destroy();
            return result;
        }

        m_current_time_ms = base::xtime_utl::gmttime_ms();
    }

    return result;
}

void xsync_task_t::stop(){
    m_finished = true;
    return;
}

bool xsync_task_t::finished() {
    return m_finished;
}

NS_END2