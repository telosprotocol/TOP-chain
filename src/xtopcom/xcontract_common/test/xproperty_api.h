#include "xcontract_common/xproperties/xproperty_access_control.h"

using namespace top::contract_common::properties;

class xtop_property_api: public xproperty_access_control_t {
public:
    xtop_property_api(top::observer_ptr<top::base::xvbstate_t> bstate, xproperty_access_control_data_t ac_data): xproperty_access_control_t(bstate, ac_data){}
    ~xtop_property_api() = default;

    bool read_permitted(top::common::xaccount_address_t const & reader, xproperty_identifier_t const & property_id)  const noexcept override final {
        return true;
    }

    bool write_permitted(top::common::xaccount_address_t const & writer, xproperty_identifier_t const & property_id) const noexcept override final {
        return true;
    }
};
using xproperty_api_t = xtop_property_api;