
#include "../xdbtool_util.h"

NS_BEG2(top, db_export)

void xdbtool_util_t::generate_json_file(std::string const & filename, json const & j) {
    std::string name{filename};
    std::ofstream out_json(name);
    out_json << std::setw(4) << j;
    out_json.close();
    std::cout << "===> " << name << " generated json success!" << std::endl;
}

void xdbtool_util_t::generate_common_file(std::string const & filename, std::string const & data) {
    std::string name{filename};
    std::ofstream out_file(name);
    if (!data.empty()) {
        out_file << data;
    }
    out_file.close();
    std::cout << "===> " << name << " generated data success!" << std::endl;
}

std::map<common::xtable_address_t, uint64_t> xdbtool_util_t::parse_table_addr_height_list(std::string const & para) {
    std::vector<std::string> table_info;
    top::base::xstring_utl::split_string(para, ',', table_info);
    std::map<common::xtable_address_t, uint64_t> table_query_criteria;
    for (auto const & t : table_info) {
        std::vector<std::string> pair;
        top::base::xstring_utl::split_string(t, ':', pair);

        table_query_criteria.emplace(common::xtable_address_t::build_from(pair[0]), std::stoull(pair[1]));
    }
    return table_query_criteria;
}

NS_END2