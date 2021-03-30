/**
 * Merkle implementation. For odd nodes, don't copy last one, just hash it as next level node.
 * Very special case is the nodes' number is 2^N + 1, the last node will be odd N times
 */

#pragma once

#include <string>
#include <vector>
#include <list>
#include <cmath>
#include "xutility/xhash.h"
#include "xbase/xmem.h"
#include "xbase/xcontext.h"
#include "xbasic/xversion.h"
#include "xbasic/xserializable_based_on.h"

NS_BEG1(top)

/**
 * merkle path node. a path of merkle
 * is now in any array list.
 * NOTE : the version back-compatiable
 * of this data struct should be done
 * by caller
 */
template<typename _Tv>
struct xmerkle_path_node_t : public xserializable_based_on<void>
{

#define XMERKLE_PATH_NODE_SIZE() \
        (sizeof(uint32_t) + sizeof(uint32_t) + sizeof(_Tv))

    uint32_t level; // level of tree, 2 ^ 32 levels, allow any big block
    uint32_t pos;   // 0, odd; 1, left; 2, right
    _Tv signature;

    int32_t do_write(base::xstream_t & stream) const override {
        KEEP_SIZE();
        stream << level;
        stream << pos;
        stream << signature;
        return CALC_LEN();
    }

    int32_t do_read(base::xstream_t & stream) override {
        KEEP_SIZE();
        stream >> level;
        stream >> pos;
        stream >> signature;
        return CALC_LEN();
    }
};

template<typename _Th, typename _Tv>
class xmerkle_t
{
public:
    /**
     * Calculate merkle root from value (leaf) list.
     * @param IN values leafs
     * @return the root hash. if succ, return value is not empty, otherwise, empty string
     */
    std::string calc_root(const std::vector<std::string> &values)
    {
        if (values.empty()) {
            return std::string();
        }

        std::vector<std::string> list;
        if (!hash_leafs(values, list)) return std::string();

        while (list.size() > 1) {
            if (!hash_list(list)) return std::string();
        }

        return list[0];
    }

    /**
     * Calculate path from value (leaf) list
     * @param IN values leafs
     * @param IN index the index of value in values list
     * @param OUT hash_path hash path list,contains enough hash values which is used by validate_path()
     * NOTE : root is not included in the path, since it is unbelievable to validator
     * @return if succ, return true; otherwise false
     */
    bool calc_path(const std::vector<std::string> &values, int index, std::vector<xmerkle_path_node_t<_Tv>>& hash_path)
    {
        if (values.empty() || !(index >= 0 && index < values.size())) {
            return false;
        }

        int level = log2(values.size()) + 1;
        std::vector<std::string> list;
        if (!hash_leafs(values, list)) return false;

        while (list.size() > 1) {
            add_path_node(list, index, level, hash_path);
            index /= 2;
            level--;
            if (!hash_list(list)) {
                hash_path.clear();
                return false;
            }
        }

        return true;
    }

    /**
     * Validate hash path for special value.
     * @param IN value the special value, which is one of leaf to be validated
     * @param IN root the root of merkle tree
     * @param IN hash_path the hash path for value validation
     * @return true if valid; otherwise false
     */
    bool validate_path(const std::string &value, const std::string &root, const std::vector<xmerkle_path_node_t<_Tv>> &hash_path)
    {
        if (value.empty() || root.empty()) return false;

        std::string hs = hash(value);
        if (hs.empty()) return false;

        int level = -1;
        for(int i=0;i<hash_path.size();i++) {
            const xmerkle_path_node_t<_Tv>& node = hash_path[i];
            if(level == -1) {
                level = node.level;
                if (node.pos == 0) {
                    // additional check : must same to hs
                    if (hs != std::string((char*) node.signature.data(), node.signature.size())) {
                        // error data
                        return false;
                    }
                    continue; // same, then next
                }
            }

            if (node.level > level) {
                // data error
                return false;
            } else if (node.level < level) {
                hs = hash_multi_times(hs, level - node.level);
                if (hs.empty()) return false;

                level = node.level; // adjust level
            }

            if (node.pos == 1) { // left
                hs = hash(std::string((char*) node.signature.data(), node.signature.size()), hs);
                if (hs.empty()) return false;
                level--;
            } else if (node.pos == 2) { // right
                hs = hash(hs, std::string((char*) node.signature.data(), node.signature.size()));
                if (hs.empty()) return false;
                level--;
            } else {
                return false; // error, shouldn't be here
            }
        }

        return hs == root;
    }

private:
    void add_path_node(const std::vector<std::string> &hashes, int index, int level, std::vector<xmerkle_path_node_t<_Tv>> &hash_path)
    {
        if (hashes.empty()) {
            return;
        }

        xmerkle_path_node_t<_Tv> node;
        node.level = level;

        if (hashes.size() == 1) {
            node.pos = 0;
            memcpy(node.signature.data(), hashes[index].c_str(), hashes[index].size());
            hash_path.push_back(node);
        } else {
            if (index & 1) {
                node.pos = 1;
                memcpy(node.signature.data(), hashes[index-1].c_str(), hashes[index-1].size());
            } else {
                if (index == hashes.size() - 1) {
                    if (hash_path.empty()) { // only first odd node need added
                        node.pos = 0;
                        memcpy(node.signature.data(), hashes[index].c_str(), hashes[index].size());
                    } else {
                        return; // escape
                    }
                } else {
                    node.pos = 2;
                    memcpy(node.signature.data(), hashes[index+1].c_str(), hashes[index+1].size());
                }
            }
            hash_path.push_back(node);
        }
    }

    bool hash_leafs(const std::vector<std::string> &values, std::vector<std::string> &list)
    {
        for(auto &s : values) {
            const std::string& str = hash(s);
            if (str.empty()) return false;

            list.push_back(str);
        }
        return true;
    }

    std::string hash_multi_times(const std::string &value, int times)
    {
        _Th h;
        return hash_multi_times(h, value, times);
    }

    std::string hash_multi_times(_Th &h, const std::string &value, int times)
    {
        _Tv v;
        h.reset();
        h.update(value);
        if (!h.get_hash(v)) return std::string();

        for(int i=1;i<times;i++) {
            h.reset();
            h.update(v.data(), v.size());
            if (!h.get_hash(v)) return std::string();
        }

        return std::string((char*) v.data(), v.size());
    }

    std::string hash(const std::string &value)
    {
        _Th h;
        return hash(h, value);
    }

    std::string hash(_Th &h, const std::string &value)
    {
        h.reset();
        h.update(value);

        _Tv v;
        if (!h.get_hash(v)) return std::string();

        return std::string((char*) v.data(), v.size());
    }

    std::string hash(const std::string &left, const std::string &right)
    {
        _Th h;
        return hash(h, left, right);
    }

    std::string hash(_Th &h, const std::string &left, const std::string &right)
    {
        h.reset();
        h.update(left);
        h.update(right);

        _Tv v;
        if (!h.get_hash(v)) return std::string();

        return std::string((char*) v.data(), v.size());
    }

    bool hash_list(std::vector<std::string>& nodes)
    {
        size_t size = nodes.size() & (~1);
        std::list<std::string> list;
        _Th h;

        for(int i=0; i + 1 < size; i += 2) {
            const std::string& str = hash(h, nodes[i], nodes[i+1]);
            if (str.empty()) return false;

            list.push_back(str);
        }

        // last odd one
        if(nodes.size() & 1) {
            const std::string& str = hash(h, nodes.back());
            if (str.empty()) return false;

            list.push_back(str);
        }

        nodes.clear();
        for(auto &s : list) {
            nodes.push_back(s);
        }

        return true;
    }

};

template<typename _Tv>
class xmerkle_path_t : public xserializable_based_on<void> {
 public:
    int32_t do_write(base::xstream_t & stream) const override {
        KEEP_SIZE();
        xassert(m_levels.size() < 255);
        uint8_t count = (uint8_t)m_levels.size();
        stream << count;
        for (auto& iter : m_levels) {
            iter.serialize_to(stream);
        }
        return CALC_LEN();
    }

    int32_t do_read(base::xstream_t & stream) override {
        KEEP_SIZE();
        uint8_t count;
        stream >> count;
        for (uint8_t i = 0; i < count; i++) {
            xmerkle_path_node_t<_Tv> value;
            value.serialize_from(stream);
            m_levels.push_back(value);
        }
        return CALC_LEN();
    }

    size_t size() const {return m_levels.size();}
    size_t get_serialize_size() const {
        base::xstream_t stream(base::xcontext_t::instance());
        xassert(stream.size() == 0);
        this->serialize_to(stream);
        return stream.size();
    }

    std::vector<xmerkle_path_node_t<_Tv>> m_levels;
};

using xmerkle_path_256_t = xmerkle_path_t<uint256_t>;


NS_END1
