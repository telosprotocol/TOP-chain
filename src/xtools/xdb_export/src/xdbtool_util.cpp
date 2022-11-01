
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

NS_END2