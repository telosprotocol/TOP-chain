#include "xsyncbase/xsync_policy.h"
#include "xvnetwork/xaddress.h"
#include "xdata/xblock.h"

#pragma once
NS_BEG2(top, sync)
class xchain_downloader_t;

enum xsync_command_execute_result {
    abort,
    finish,
    abort_overflow,
    wait_response,
};

template<class T>
class xsync_command_t {
    public:
        virtual xsync_command_execute_result execute(T t) = 0;
};

class xsync_on_blocks_response_command_t : public xsync_command_t<xchain_downloader_t *> {
    public:
        xsync_on_blocks_response_command_t(const std::vector<data::xblock_ptr_t>& blocks, const vnetwork::xvnode_address_t& self_addr,
            const vnetwork::xvnode_address_t& target_addr):
            m_blocks(blocks), m_self_addr(self_addr), m_target_addr(target_addr){
        };

        xsync_command_execute_result execute(xchain_downloader_t *downloader) override;
    private:
        std::vector<data::xblock_ptr_t> m_blocks;
        vnetwork::xvnode_address_t m_self_addr;
        vnetwork::xvnode_address_t m_target_addr;
};

class xsync_on_snapshot_response_command_t : public xsync_command_t<xchain_downloader_t *> {
    public:
        xsync_on_snapshot_response_command_t(
            const std::string & chain_snapshot, uint64_t height,
            const vnetwork::xvnode_address_t& self_addr,
            const vnetwork::xvnode_address_t& target_addr):
            m_height(height), m_snapshot(chain_snapshot),  m_self_addr(self_addr),
            m_target_addr(target_addr){
        };

        xsync_command_execute_result execute(xchain_downloader_t *downloader) override;

    private:
        uint64_t m_height;
        std::string m_snapshot;
        vnetwork::xvnode_address_t m_self_addr;
        vnetwork::xvnode_address_t m_target_addr;
};

class xsync_download_command_t : public xsync_command_t<xchain_downloader_t *> {
    public:
        xsync_download_command_t(
            const std::pair<uint64_t, uint64_t> height_interval, const enum_chain_sync_policy sync_policy,
            const vnetwork::xvnode_address_t& self_addr,
            const vnetwork::xvnode_address_t& target_addr):
            m_expect_height_interval(height_interval), m_sync_policy(sync_policy),
            m_self_addr(self_addr), m_target_addr(target_addr){
        };

        xsync_command_execute_result execute(xchain_downloader_t *downloader) override;

    private:
        std::pair<uint64_t, uint64_t> m_expect_height_interval;
        enum_chain_sync_policy m_sync_policy;
        vnetwork::xvnode_address_t m_self_addr;
        vnetwork::xvnode_address_t m_target_addr;
};

class xsync_task_t {
public:
    xsync_task_t(xchain_downloader_t *downloader): m_downloader(downloader){
    }

    // std::string id() {
    //     return m_self_addr.to_string() + "||" + m_target_addr.to_string();
    // }

    bool start();
    bool expired();
    xsync_command_execute_result execute(xsync_command_t<xchain_downloader_t *> &cmd);
    void stop();
    bool finished();
private:
    bool m_finished{true};
    const int64_t m_timeout_ms{10000};
    int64_t m_current_time_ms{0};
    xchain_downloader_t *m_downloader;
};

NS_END2
