// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xmbus/xmessage_bus.h"
#include "xunit_service/xbatch_packer.h"
#include "xunit_service/xcons_face.h"

#include <mutex>
#include <string>
#include <vector>

NS_BEG2(top, xunit_service)

// task dispatch
class xworkpool_dispatcher
  : public xcons_dispatcher
  , public std::enable_shared_from_this<xworkpool_dispatcher> {
public:
    xworkpool_dispatcher(observer_ptr<mbus::xmessage_bus_face_t> const &mb,
            std::shared_ptr<xcons_service_para_face> const & p_para, std::shared_ptr<xblock_maker_face> const & block_maker);
    virtual ~xworkpool_dispatcher();

public:
    virtual bool dispatch(base::xworkerpool_t * pool, base::xcspdu_t * pdu, const xvip2_t & xip_from, const xvip2_t & xip_to);
    virtual bool subscribe(const std::vector<base::xtable_index_t> & tables, const xvip2_t & xip, const common::xlogic_time_t& start_time);

public:
    virtual bool start(const xvip2_t & xip, const common::xlogic_time_t& start_time);

    virtual bool fade(const xvip2_t & xip);

    virtual bool unreg(const xvip2_t & xip);

    virtual bool destroy(const xvip2_t & xip);

protected:
    std::string       account(base::xtable_index_t & tableid);
    base::xtable_index_t get_tableid(const std::string & account);
    int16_t           get_thread_index(base::xworkerpool_t * pool, base::xtable_index_t& tableid);
    base::xworker_t * get_worker(base::xworkerpool_t * pool, base::xtable_index_t& table_id);
    void              fire_clock(base::xvblock_t * block, base::xworker_t *, xbatch_packer_ptr_t packer);
    void              chain_timer(common::xlogic_time_t time);
    void              on_clock(base::xvblock_t * clock_block) override;

protected:
    observer_ptr<mbus::xmessage_bus_face_t>  m_mbus;
    std::mutex                               m_mutex;
    xbatch_paker_map                         m_packers;
    std::shared_ptr<xcons_service_para_face> m_para;
    std::shared_ptr<xblock_maker_face>       m_blockmaker;
};

NS_END2
