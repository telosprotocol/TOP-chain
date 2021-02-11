// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xdata/xblockbody.h"
#include "xdata/xdata_common.h"
#include "xutility/xhash.h"
#include "xmetrics/xmetrics.h"

NS_BEG2(top, data)

REG_CLS(xdummy_entity_t);
REG_CLS(xinput_t);
REG_CLS(xoutput_t);

xinput_t::xinput_t()
: base::xvinput_t((enum_xdata_type)object_type_value) {
    XMETRICS_XBASE_DATA_CATEGORY_NEW(object_type_value);
}
xinput_t::xinput_t(const std::vector<base::xventity_t*> & entitys, const std::string & resource_data)
: base::xvinput_t(entitys, resource_data, (enum_xdata_type)object_type_value) {
    XMETRICS_XBASE_DATA_CATEGORY_NEW(object_type_value);
}
xinput_t::xinput_t(const std::vector<base::xventity_t*> & entitys, base::xstrmap_t & resource_obj)
: base::xvinput_t(entitys, resource_obj, (enum_xdata_type)object_type_value) {
    XMETRICS_XBASE_DATA_CATEGORY_NEW(object_type_value);
}

xinput_t::~xinput_t() {
    XMETRICS_XBASE_DATA_CATEGORY_DELETE(object_type_value);
    // xdbg("xinput_t::~xinput_t this=%p", this);
}

std::vector<std::string> xinput_t::get_merkle_leafs() const {
    std::vector<std::string> leafs;
    const auto & entitys = get_entitys();
    if (entitys.empty()) {
        xassert(!entitys.empty());
        return {};
    }

    for (auto & v : entitys) {
        std::string leaf = v->query_value("merkle-tree-leaf");
        if (!leaf.empty()) {
            leafs.push_back(leaf);
        }
    }
    return std::move(leafs);
}

const std::string xinput_t::get_root_hash() {
    std::vector<std::string> leafs = get_merkle_leafs();
    if (leafs.empty()) {
        return {};
    }
    xmerkle_t<utl::xsha2_256_t, uint256_t> merkle;
    std::string root = merkle.calc_root(leafs);
    return root;
}

bool xinput_t::calc_merkle_path(const std::string & leaf, xmerkle_path_256_t& hash_path) const {
    std::vector<std::string> leafs = get_merkle_leafs();
    if (leafs.empty()) {
        xassert(0);
        return false;
    }
    auto iter = std::find(leafs.begin(), leafs.end(), leaf);
    if (iter == leafs.end()) {
        xassert(0);
        return false;
    }

    int index = static_cast<int>(std::distance(leafs.begin(), iter));
    xmerkle_t<utl::xsha2_256_t, uint256_t> merkle;
    return merkle.calc_path(leafs, index, hash_path.m_levels);
}

std::string xinput_t::get_binary_string() const {
    base::xstream_t stream1(base::xcontext_t::instance());
    const_cast<xinput_t*>(this)->serialize_to(stream1);
    std::string binary_bin = std::string((char*)stream1.data(), stream1.size());
    return binary_bin;
}

xoutput_t::xoutput_t()
: base::xvoutput_t((enum_xdata_type)object_type_value) {
    XMETRICS_XBASE_DATA_CATEGORY_NEW(object_type_value);
}
xoutput_t::xoutput_t(const std::vector<base::xventity_t*> & entitys, const std::string & resource_data)
: base::xvoutput_t(entitys, resource_data, (enum_xdata_type)object_type_value) {
    XMETRICS_XBASE_DATA_CATEGORY_NEW(object_type_value);
}
xoutput_t::xoutput_t(const std::vector<base::xventity_t*> & entitys, base::xstrmap_t & resource_obj)
: base::xvoutput_t(entitys, resource_obj, (enum_xdata_type)object_type_value) {
    XMETRICS_XBASE_DATA_CATEGORY_NEW(object_type_value);
}

xoutput_t::~xoutput_t() {
    XMETRICS_XBASE_DATA_CATEGORY_DELETE(object_type_value);
    // xdbg("xoutput_t::~xoutput_t this=%p", this);
}

std::vector<std::string> xoutput_t::get_merkle_leafs() const {
    std::vector<std::string> leafs;
    const auto & entitys = get_entitys();
    if (entitys.empty()) {
        xassert(!entitys.empty());
        return {};
    }

    for (auto & v : entitys) {
        std::string leaf = v->query_value("merkle-tree-leaf");
        if (!leaf.empty()) {
            leafs.push_back(leaf);
        }
    }
    return std::move(leafs);
}

const std::string xoutput_t::get_root_hash() {
    std::vector<std::string> leafs = get_merkle_leafs();
    if (leafs.empty()) {
        return {};
    }
    xmerkle_t<utl::xsha2_256_t, uint256_t> merkle;
    std::string root = merkle.calc_root(leafs);
    return root;
}

bool xoutput_t::calc_merkle_path(const std::string & leaf, xmerkle_path_256_t& hash_path) const {
    std::vector<std::string> leafs = get_merkle_leafs();
    if (leafs.empty()) {
        xassert(0);
        return false;
    }
    auto iter = std::find(leafs.begin(), leafs.end(), leaf);
    if (iter == leafs.end()) {
        xassert(0);
        return false;
    }

    int index = static_cast<int>(std::distance(leafs.begin(), iter));
    xmerkle_t<utl::xsha2_256_t, uint256_t> merkle;
    return merkle.calc_path(leafs, index, hash_path.m_levels);
}

std::string xoutput_t::get_binary_string() const{
    base::xstream_t stream1(base::xcontext_t::instance());
    const_cast<xoutput_t*>(this)->serialize_to(stream1);
    std::string binary_bin = std::string((char*)stream1.data(), stream1.size());
    return binary_bin;
}

void xblockbody_para_t::create_default_input_output() {
    if (m_input == nullptr) {
        xassert(!input_entitys.empty());
        m_input = make_object_ptr<xinput_t>(input_entitys, *(input_resource.get_resource().get()));
    }
    if (m_output == nullptr) {
        xassert(!output_entitys.empty());
        m_output = make_object_ptr<xoutput_t>(output_entitys, *(output_resource.get_resource().get()));
    }
}

NS_END2
