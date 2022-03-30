/**
 * Merkle implementation. For odd nodes, don't copy last one, just hash it as next level node.
 * Very special case is the nodes' number is 2^N + 1, the last node will be odd N times
 */

#pragma once

#include <string>
#include <vector>
#include <list>
#include <cmath>
#include "xbase/xmem.h"
#include "xbase/xcontext.h"
#include "xmetrics/xmetrics.h"

#ifdef __GNUC__
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wsign-compare"
#endif  // __GNUC__

namespace top
{
    namespace base
    {
        /**
         * merkle path node. a path of merkle
         * is now in any array list.
         * NOTE : the version back-compatiable
         * of this data struct should be done
         * by caller
         */
        template<typename _Tv>
        struct xmerkle_path_node_t
        {
        public:
            xmerkle_path_node_t()
            {
                level = 0;
                pos   = 0;
            }
            ~xmerkle_path_node_t(){};
            xmerkle_path_node_t(xmerkle_path_node_t const &)             = default;
            xmerkle_path_node_t(xmerkle_path_node_t &&)                  = default;
            xmerkle_path_node_t & operator=(xmerkle_path_node_t const &) = default;
            xmerkle_path_node_t & operator=(xmerkle_path_node_t &&)      = default;

        public:
        #define XMERKLE_PATH_NODE_SIZE() \
                (sizeof(uint32_t) + sizeof(uint32_t) + sizeof(_Tv))

            uint32_t level; // level of tree, 2 ^ 32 levels, allow any big block
            uint32_t pos;   // 0, odd; 1, left; 2, right
            _Tv signature;

        public:
            int32_t serialize_to(base::xstream_t & stream) const {
                const int32_t begin_size = stream.size();

                stream.write_compact_var(level);
                stream.write_compact_var(pos);
                stream << signature;

                return (stream.size() - begin_size);
            }

            int32_t serialize_from(base::xstream_t & stream) {
                const int32_t begin_size = stream.size();

                stream.read_compact_var(level);
                stream.read_compact_var(pos);
                stream >> signature;

                return (begin_size - stream.size());
            }

            bool operator == (const xmerkle_path_node_t & other) const
            {
                if( (other.level == level) && (other.pos == pos) && (other.signature == signature) )
                    return true;

                return false;
            }
        };

        template<typename _Th, typename _Tv>
        class xmerkle_t
        {
        public:
            xmerkle_t(){};
            xmerkle_t(const std::vector<std::string>  &leafs, bool leaf_change_hash = true)
            {
                calc_root_hash(leafs, leaf_change_hash);
            }

            /**
             * Calculate merkle root from value (leaf) list.
             * @param  values leafs
             * @return the root hash. if succ, return value is not empty, otherwise, empty string
             */
            static std::string calc_root(const std::vector<std::string> &values)
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
             * @param values leafs
             * @param index the index of value in values list
             * @param hash_path hash path list,contains enough hash values which is used by validate_path()
             * NOTE : root is not included in the path, since it is unbelievable to validator
             * @return if succ, return true; otherwise false
             */
            static bool calc_path(const std::vector<std::string> &values, int index, std::vector<xmerkle_path_node_t<_Tv>>& hash_path)
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
             * @param value the special value, which is one of leaf to be validated
             * @param root the root of merkle tree
             * @param hash_path the hash path for value validation
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

            /**
             * Calculate merkle root from value (leaf) list.
             * @param  values leafs
             * @param  leaf_change_hash decide leafs if change to hash
             * @return the root hash. if succ, return value is not empty, otherwise, empty string
             */
            std::string calc_root_hash(const std::vector<std::string> & values, bool leaf_change_hash = true)
            {
                if (!m_tree_root_hash.empty()) {
                    return m_tree_root_hash;
                }

                if (values.empty()) {
                    return std::string();
                }

                std::vector<std::string> leafs_hash;
                if (!hash_leafs(values, leafs_hash, leaf_change_hash)) {
                    return std::string();
                }

                m_tree_leafs = values;
                m_tree_level = ceil(log2(values.size()))+1; //fix tree level max
                m_node_level = log2(values.size())+1;       //keep node level max
                m_tree_level_node.emplace(m_tree_level_node.begin(), leafs_hash);
                
                uint64_t level = 1;   
                while (level < m_tree_level) {
                    std::vector<std::string> list = m_tree_level_node.front();
                    if (!hash_list(list)) {
                       return std::string();
                    }
                    m_tree_level_node.emplace(m_tree_level_node.begin(), list);
                    level++;
                }
                
                auto &root_hash_node = m_tree_level_node.at(0);
                m_tree_root_hash = root_hash_node.at(0);
                return m_tree_root_hash;
            }

            /**
             * Calculate path from value (leaf) list
             * @param leaf leaf
             * @param hash_path hash path list,contains enough hash values which is used by validate_path()
             * NOTE : root is not included in the path, since it is unbelievable to validator
             * @return if succ, return true; otherwise false
             */
            bool calc_path_hash(const std::string &leaf, std::vector<xmerkle_path_node_t<_Tv>>& hash_path)
            {
                //check merkle tree exist
                if (m_tree_root_hash.empty() || m_tree_leafs.empty()) {
                    xwarn(" merkle tree is null.");
                    return false;
                }

                auto iter = std::find(m_tree_leafs.begin(), m_tree_leafs.end(), leaf);
                if (iter == m_tree_leafs.end()) {
                    xassert(0);
                    return false;
                }

                int tree_level = (int)m_tree_level;
                int node_level = (int)m_node_level;
                int index = static_cast<int>(std::distance(m_tree_leafs.begin(), iter));;
                while (tree_level > 1) {
                    std::vector<std::string> &hash_list = m_tree_level_node.at(tree_level-1);
                    add_path_node(hash_list, index, node_level, hash_path);
                    index /= 2;
                    node_level--;
                    tree_level--;
                }

                return true;
            }

        private:
            static void add_path_node(const std::vector<std::string> &hashes, int index, int level, std::vector<xmerkle_path_node_t<_Tv>> &hash_path)
            {
                if (hashes.empty()) {
                    return;
                }

                xmerkle_path_node_t<_Tv> node;
                node.level = level;

                if (hashes.size() == 1) {
                    node.pos = 0;
                    node.signature = hashes[index];
                    // memcpy(node.signature.data(), hashes[index].c_str(), hashes[index].size());
                    hash_path.push_back(node);
                } else {
                    if (index & 1) {
                        node.pos = 1;
                        node.signature = hashes[index-1];
                        // memcpy(node.signature.data(), hashes[index-1].c_str(), hashes[index-1].size());
                    } else {
                        if (index == hashes.size() - 1) {
                            if (hash_path.empty()) { // only first odd node need added
                                node.pos = 0;
                                node.signature = hashes[index];
                                // memcpy(node.signature.data(), hashes[index].c_str(), hashes[index].size());
                            } else {
                                return; // escape
                            }
                        } else {
                            node.pos = 2;
                            node.signature = hashes[index+1];
                            // memcpy(node.signature.data(), hashes[index+1].c_str(), hashes[index+1].size());
                        }
                    }
                    hash_path.push_back(node);
                }
            }

            static bool hash_leafs(const std::vector<std::string> &values, std::vector<std::string> &list, bool leaf_change_hash=true)
            {               
                if (!leaf_change_hash) {
                   list = values;
                   return true;
                }

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
                if (!get_hash(h, v)) return std::string();

                for(int i=1;i<times;i++) {
                    h.reset();
                    h.update(v.data(), v.size());
                    if (!get_hash(h, v)) return std::string();
                }

                return std::string((char*) v.data(), v.size());
            }

            static std::string hash(const std::string &value)
            {
                _Th h;
                return hash(h, value);
            }

            static std::string hash(_Th &h, const std::string &value)
            {
                h.reset();
                h.update(value);

                _Tv v;
                if (!get_hash(h, v)) return std::string();

                return std::string((char*) v.data(), v.size());
            }

            static std::string hash(const std::string &left, const std::string &right)
            {
                _Th h;
                return hash(h, left, right);
            }

            static std::string hash(_Th &h, const std::string &left, const std::string &right)
            {
                h.reset();
                h.update(left);
                h.update(right);

                _Tv v;
                if (!get_hash(h, v)) return std::string();

                return std::string((char*) v.data(), v.size());
            }

            static bool get_hash(_Th &h, _Tv &v)
            {   
                XMETRICS_GAUGE(metrics::cpu_merkle_hash_calc, 1);
                return h.get_hash(v);
            }

            static bool hash_list(std::vector<std::string>& nodes)
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

            
        private:
            uint64_t  m_tree_level = 0;
            uint64_t  m_node_level = 0 ;
            std::string m_tree_root_hash {};
            std::vector<std::string> m_tree_leafs {};
            std::vector<std::vector<std::string>> m_tree_level_node {};
        };

        template<typename _Tv>
        class xmerkle_path_t
        {
        public:
            xmerkle_path_t(){};
            xmerkle_path_t(const std::vector<xmerkle_path_node_t<_Tv>> & paths)
            {
                m_levels = paths;
            }
            ~xmerkle_path_t(){};
        private:
            xmerkle_path_t(const xmerkle_path_t & obj);
            xmerkle_path_t(const xmerkle_path_t && obj);
            xmerkle_path_t & operator = (xmerkle_path_t && obj);
            xmerkle_path_t & operator = (const xmerkle_path_t & obj);

        public:
            int32_t   serialize_to(xstream_t & stream) const
            {
                return do_write(stream);
            }

            int32_t   serialize_from(xstream_t & stream)
            {
                return do_read(stream);
            }
            std::string serialize_to_string() {
                base::xstream_t _stream(base::xcontext_t::instance());
                serialize_to(_stream);
                std::string _path_bin = std::string((char *)_stream.data(), _stream.size());
                xassert(!_path_bin.empty());
                return _path_bin;
            }

         protected:
            int32_t do_write(base::xstream_t & stream) const {
                const int32_t begin_size = stream.size();
                xassert(m_levels.size() < 255);
                const uint8_t count = (uint8_t)m_levels.size();
                stream << count;
                for (auto& iter : m_levels) {
                    iter.serialize_to(stream);
                }
                return (stream.size() - begin_size);
            }

            int32_t do_read(base::xstream_t & stream) {
                const int32_t begin_size = stream.size();
                uint8_t count = 0;
                stream >> count;
                for (uint8_t i = 0; i < count; i++) {
                    xmerkle_path_node_t<_Tv> value;
                    value.serialize_from(stream);
                    m_levels.push_back(value);
                }
                return (begin_size - stream.size());
            }
        public:
            std::vector<xmerkle_path_node_t<_Tv>> &  get_levels_for_write() {return m_levels;}  // TODO(jimmy)
            const std::vector<xmerkle_path_node_t<_Tv>> &  get_levels() {return m_levels;}  // TODO(jimmy)
            size_t size() const {return m_levels.size();}
            size_t get_serialize_size() const {
                base::xstream_t stream(base::xcontext_t::instance());
                xassert(stream.size() == 0);
                this->serialize_to(stream);
                return stream.size();
            }
        private:
            std::vector<xmerkle_path_node_t<_Tv>> m_levels;
        };

        using xmerkle_path_256_t = xmerkle_path_t<uint256_t>;


    }//end of namespace of base

}//end of namespace top
#ifdef __GNUC__
#    pragma GCC diagnostic pop
#endif  // __GNUC__
