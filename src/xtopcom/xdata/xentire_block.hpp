#pragma once

#include <string>
#include "xbasic/xserialize_face.h"
#include "xdata/xblock.h"
#include "xdata/xtransaction.h"
#include "xbase/xobject_ptr.h"
#include "xmetrics/xmetrics.h"
#include "xsync/xsync_util.h"

namespace top { namespace data {
using xdataobj_ptr_t = xobject_ptr_t<base::xdataobj_t>;
using namespace base;

class xentire_block_t : public top::basic::xserialize_face_t {
public:

    xentire_block_t() {
        XMETRICS_COUNTER_INCREMENT("sync_entire_blocks", 1);
    }

    virtual int32_t do_write(base::xstream_t & stream) {
        KEEP_SIZE();

        if (block_ptr != nullptr) {
            block_ptr->full_block_serialize_to(stream);
            if (carry_unit_blocks) {
                if (!block_ptr->is_tableblock()) {
                    xerror("xentire_block_t::do_write not table block,but carry_unit_blocks is true:%s", block_ptr->dump().c_str());
                    return -1;
                }

                uint32_t num = 0;
                std::vector<xblock_ptr_t> blocks;
                auto unit_infos_str = block_ptr->get_unit_infos();
                xdbg("xentire_block_t::do_write table block:%s, unit_infos:%s", block_ptr->dump().c_str(), unit_infos_str.c_str());
                if (!unit_infos_str.empty()) {
                    base::xtable_unit_infos_t unit_infos;
                    unit_infos.serialize_from_string(unit_infos_str);
                    auto &unit_infos_vec = unit_infos.get_unit_infos();
                    xdbg("xentire_block_t::do_write table block:%s, unit_infos_vec size:%u", block_ptr->dump().c_str(), unit_infos_vec.size());

                    for (auto & unit_info : unit_infos_vec) {
                        base::xvaccount_t account(unit_info.get_addr());
                        base::xauto_ptr<base::xvblock_t> unit_block = base::xvchain_t::instance().get_xblockstore()->load_block_object(account, unit_info.get_height(), unit_info.get_hash(), true);
                        if (unit_block != nullptr) {
                            unit_block->add_ref();
                            base::xvblock_t *vblock = unit_block.get();
                            data::xblock_t *block = (data::xblock_t*)vblock;
                            data::xblock_ptr_t unit_block_ptr = nullptr;
                            unit_block_ptr.attach(block);
                            blocks.push_back(unit_block_ptr);
                        } else {
                            xwarn("xentire_block_t::do_write table block:%s, unit block load fail account:%s,height:%llu,hash:%s,hash size:%u", block_ptr->dump().c_str(), unit_info.get_addr().c_str(), unit_info.get_height(), xstring_utl::to_hex(unit_info.get_hash()).c_str(), unit_info.get_hash().size());
                            return -1;
                        }
                    }
                }

                num = blocks.size();
                stream << num;
                xdbg("xentire_block_t::do_write table block:%s, unit num:%d", block_ptr->dump().c_str(), num);
                for (auto & block :blocks) {
                    block->full_block_serialize_to(stream);
                    xdbg("xentire_block_t::do_write succ table block:%s,unit block:%s", block_ptr->dump().c_str(), block->dump().c_str());
                }
            }
        }

        return CALC_LEN();
    }

    virtual int32_t do_read(base::xstream_t & stream) {
        KEEP_SIZE();

        xblock_t* _data_obj = dynamic_cast<xblock_t*>(xblock_t::full_block_read_from(stream));
        if (_data_obj != nullptr) {
            block_ptr.attach(_data_obj);
            block_ptr->reset_block_flags();

            if (block_ptr->is_tableblock()) {
                uint32_t num = 0;
                stream >> num;
                if (num > 0) {
                    carry_unit_blocks = true;
                    xdbg("xentire_block_t::do_read table block:%s, num:%d", block_ptr->dump().c_str(), num);
                    for (uint32_t i = 0; i < num; i++) {
                        xblock_t* _unit = dynamic_cast<xblock_t*>(xblock_t::full_block_read_from(stream));
                        xblock_ptr_t unit_ptr = nullptr;
                        unit_ptr.attach(_unit);
                        unit_ptr->reset_block_flags();
                        unit_blocks.push_back(unit_ptr);
                        xdbg("xentire_block_t::do_read table block:%s, unit block:%s", block_ptr->dump().c_str(), unit_ptr->dump().c_str());
                    }
                }
            }
        }
        return CALC_LEN();
    }

private:
    virtual ~xentire_block_t() {
        XMETRICS_COUNTER_INCREMENT("sync_entire_blocks", -1);
    }

public:
    xblock_ptr_t block_ptr{};
    std::vector<xblock_ptr_t> unit_blocks;
    bool carry_unit_blocks{false};
};

using xentire_block_ptr_t = xobject_ptr_t<xentire_block_t>;

}
}
