#include "../xerror.h"

NS_BEG2(top, db_export)

std::error_code make_error_code(xerrc_t const ec) noexcept {
    return std::error_code{static_cast<int>(ec), db_export_category()};
}

static char const * err_msg(int const ec) {
    switch (static_cast<xerrc_t>(ec)) {
    case xerrc_t::ok:
        return "success";

    case xerrc_t::invalid_table_address:
        return "ivalid table address";

    case xerrc_t::table_block_not_found:
        return "table block not found";

    case xerrc_t::table_state_not_found:
        return "table state not found";

    case xerrc_t::account_data_not_found:
        return "account data not found";

    case xerrc_t::unknown_token:
        return "unknown token";

    default:
        return "unknown error";
    }
}

class xtop_db_export_category final : public std::error_category {
public:
    char const * name() const noexcept override {
        return "db_export";
    }

    std::string message(int const ec) const override {
        return err_msg(ec);
    }

    ~xtop_db_export_category() noexcept override = default;
};
using xdb_export_category_t = xtop_db_export_category;

std::error_category const & db_export_category() {
    static xdb_export_category_t c;
    return c;
}

NS_END2
