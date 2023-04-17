#pragma once

#include <string>
#include "xbasic/xserialize_face.h"
#include "xdata/xblock.h"
#include "xdata/xtransaction.h"
#include "xbase/xobject_ptr.h"
#include "xmetrics/xmetrics.h"

namespace top { namespace data {
using xdataobj_ptr_t = xobject_ptr_t<base::xdataobj_t>;
using namespace base;

class xentire_block_t : public top::basic::xserialize_face_t {
public:

    xentire_block_t() {
    }

    virtual int32_t do_write(base::xstream_t & stream) {
        KEEP_SIZE();

        if (block_ptr != nullptr) {
            block_ptr->full_block_serialize_to(stream);
        }

        return CALC_LEN();
    }

    virtual int32_t do_read(base::xstream_t & stream) {
        KEEP_SIZE();

        xblock_t* _data_obj = dynamic_cast<xblock_t*>(xblock_t::full_block_read_from(stream));
        if (_data_obj != nullptr) {
            block_ptr.attach(_data_obj);
            block_ptr->reset_block_flags();
        }
        return CALC_LEN();
    }

private:
    virtual ~xentire_block_t() {
    }

public:
    xblock_ptr_t block_ptr{};
};

using xentire_block_ptr_t = xobject_ptr_t<xentire_block_t>;

}
}
