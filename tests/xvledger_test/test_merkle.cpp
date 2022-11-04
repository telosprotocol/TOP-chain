#include "gtest/gtest.h"
#include "xvledger/xvaccount.h"
#include "xvledger/xmerkle.hpp"
#include "xutility/xhash.h"
#include "xmetrics/xmetrics.h"
// #include "tests/mock/xvchain_creator.hpp"

using namespace top;
using namespace top::base;

class test_merkle : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};


int64_t hash_count_calc(int64_t n){
    int64_t count = n;
    if(0 == n){
      return 0;
    }
    if (1 == n) {
      return 1;
    }
    if (0 == (n%2)) {
       return (count + hash_count_calc((n/2)));
    } else {
       return (count + hash_count_calc((n/2) +1));
    }
}

class test_hash_mock_t {
 public:
    virtual bool     reset()/// restart
    {
        m_raw_value.clear();
        return true;
    } 
    virtual int                     update(const std::string & text) {
        m_raw_value += text;
        return (int)text.size();
    }
    virtual int                     update(const void* data, size_t numBytes) {
        std::string input = std::string((const char*)data, numBytes);
        m_raw_value += input;
        return (int)numBytes;
    }
    virtual bool                    get_hash(std::vector<uint8_t> & raw_bytes) {
        raw_bytes.assign(m_raw_value.begin(), m_raw_value.end());
        return true;
    }
    virtual bool     get_hash(std::string & hash) {
        hash = m_raw_value;
        return true;
    }
  private:
    std::string     m_raw_value;
};


TEST_F(test_merkle, merkle_root_1) {
    xmerkle_t<test_hash_mock_t, std::string> merkle;
    std::vector<std::string> leafs;
    leafs.push_back("1");

    std::string root = merkle.calc_root(leafs);
    std::cout << "root=" << root << std::endl;
    ASSERT_EQ(root, "1");
}

TEST_F(test_merkle, merkle_root_2) {
    xmerkle_t<test_hash_mock_t, std::string> merkle;
    std::vector<std::string> leafs;
    leafs.push_back("1");
    leafs.push_back("2");

    std::string root = merkle.calc_root(leafs);
    std::cout << "root=" << root << std::endl;
    ASSERT_EQ(root, "12");
    std::string root2 = merkle.calc_root(leafs);
    ASSERT_EQ(root2, "12");    
}
TEST_F(test_merkle, merkle_root_3) {
    xmerkle_t<test_hash_mock_t, std::string> merkle;
    std::vector<std::string> leafs;
    leafs.push_back("1");
    leafs.push_back("2");
    leafs.push_back("3");

    std::string root = merkle.calc_root(leafs);
    std::cout << "root=" << root << std::endl;
    ASSERT_EQ(root, "123");
}
TEST_F(test_merkle, merkle_root_4) {
    xmerkle_t<test_hash_mock_t, std::string> merkle;
    std::vector<std::string> leafs;
    leafs.push_back("1");
    leafs.push_back("2");
    leafs.push_back("3");
    leafs.push_back("4");

    std::string root = merkle.calc_root(leafs);
    std::cout << "root=" << root << std::endl;
    ASSERT_EQ(root, "1234");
}

TEST_F(test_merkle, merkle_path_1) {
    xmerkle_t<test_hash_mock_t, std::string> merkle;
    std::vector<std::string> leafs;
    leafs.push_back("1");

    {
        xmerkle_path_t<std::string> hash_path;
        int index = 0;
        bool ret = merkle.calc_path(leafs, index, hash_path.get_levels_for_write());
        ASSERT_EQ(ret, true);
        const std::vector<xmerkle_path_node_t<std::string>> & levels = hash_path.get_levels();
        ASSERT_EQ(levels.size(), 0);
    }
}
TEST_F(test_merkle, merkle_path_2) {
    xmerkle_t<test_hash_mock_t, std::string> merkle;
    std::vector<std::string> leafs;
    leafs.push_back("1");
    leafs.push_back("2");

    {
        xmerkle_path_t<std::string> hash_path;
        int index = 0;
        bool ret = merkle.calc_path(leafs, index, hash_path.get_levels_for_write());
        ASSERT_EQ(ret, true);
        const std::vector<xmerkle_path_node_t<std::string>> & levels = hash_path.get_levels();
        ASSERT_EQ(levels.size(), 1);
        std::cout << "merkle path for index " << index << std::endl;
        /*for (size_t i = 0; i < levels.size(); i++) {
            std::cout << "i = " << i << std::endl;
            std::cout << "level=" << levels[i].level << std::endl;
            std::cout << "pos=" << levels[i].pos << std::endl;
            std::cout << "signature=" << levels[i].signature << std::endl;
        }*/
    }
    {
        xmerkle_path_t<std::string> hash_path;
        int index = 1;
        bool ret = merkle.calc_path(leafs, index, hash_path.get_levels_for_write());
        ASSERT_EQ(ret, true);
        const std::vector<xmerkle_path_node_t<std::string>> & levels = hash_path.get_levels();
        ASSERT_EQ(levels.size(), 1);
        /*std::cout << "merkle path for index " << index << std::endl;
        for (size_t i = 0; i < levels.size(); i++) {
            std::cout << "i = " << i << std::endl;
            std::cout << "level=" << levels[i].level << std::endl;
            std::cout << "pos=" << levels[i].pos << std::endl;
            std::cout << "signature=" << levels[i].signature << std::endl;
        }*/
    }    
}
TEST_F(test_merkle, merkle_path_3) {
    xmerkle_t<test_hash_mock_t, std::string> merkle;
    std::vector<std::string> leafs;
    leafs.push_back("1");
    leafs.push_back("2");
    leafs.push_back("3");

    {
        xmerkle_path_t<std::string> hash_path;
        int index = 0;
        bool ret = merkle.calc_path(leafs, index, hash_path.get_levels_for_write());
        ASSERT_EQ(ret, true);
        const std::vector<xmerkle_path_node_t<std::string>> & levels = hash_path.get_levels();
        ASSERT_EQ(levels.size(), 2);
        /*std::cout << "merkle path for index " << index << std::endl;
        for (size_t i = 0; i < levels.size(); i++) {
            std::cout << "i = " << i << std::endl;
            std::cout << "level=" << levels[i].level << std::endl;
            std::cout << "pos=" << levels[i].pos << std::endl;
            std::cout << "signature=" << levels[i].signature << std::endl;
        }*/
    }
    {
        xmerkle_path_t<std::string> hash_path;
        int index = 1;
        bool ret = merkle.calc_path(leafs, index, hash_path.get_levels_for_write());
        ASSERT_EQ(ret, true);
        const std::vector<xmerkle_path_node_t<std::string>> & levels = hash_path.get_levels();
        ASSERT_EQ(levels.size(), 2);
        /*std::cout << "merkle path for index " << index << std::endl;
        for (size_t i = 0; i < levels.size(); i++) {
            std::cout << "i = " << i << std::endl;
            std::cout << "level=" << levels[i].level << std::endl;
            std::cout << "pos=" << levels[i].pos << std::endl;
            std::cout << "signature=" << levels[i].signature << std::endl;
        }*/
    }    
}
TEST_F(test_merkle, merkle_path_4) {
    xmerkle_t<test_hash_mock_t, std::string> merkle;
    std::vector<std::string> leafs;
    leafs.push_back("1");
    leafs.push_back("2");
    leafs.push_back("3");
    leafs.push_back("4");

    {
        xmerkle_path_t<std::string> hash_path;
        int index = 0;
        bool ret = merkle.calc_path(leafs, index, hash_path.get_levels_for_write());
        ASSERT_EQ(ret, true);
        const std::vector<xmerkle_path_node_t<std::string>> & levels = hash_path.get_levels();
        ASSERT_EQ(levels.size(), 2);
        /*std::cout << "merkle path for index " << index << std::endl;
        for (size_t i = 0; i < levels.size(); i++) {
            std::cout << "i = " << i << std::endl;
            std::cout << "level=" << levels[i].level << std::endl;
            std::cout << "pos=" << levels[i].pos << std::endl;
            std::cout << "signature=" << levels[i].signature << std::endl;
        }*/

        
    }   
    {
        xmerkle_path_t<std::string> hash_path;
        int index = 1;
        bool ret = merkle.calc_path(leafs, index, hash_path.get_levels_for_write());
        ASSERT_EQ(ret, true);
        const std::vector<xmerkle_path_node_t<std::string>> & levels = hash_path.get_levels();
        ASSERT_EQ(levels.size(), 2);
        /*std::cout << "merkle path for index " << index << std::endl;
        for (size_t i = 0; i < levels.size(); i++) {
            std::cout << "i = " << i << std::endl;
            std::cout << "level=" << levels[i].level << std::endl;
            std::cout << "pos=" << levels[i].pos << std::endl;
            std::cout << "signature=" << levels[i].signature << std::endl;
        }*/
    }    
}
TEST_F(test_merkle, merkle_valid_path) {   
    int leafs_num = 100;
     int count = 0;
    for (count = 0; count < leafs_num; count++) {
       
        std::vector<std::string> leafs;

        //std::cout << "merkle leasf number  " << count << " leafs is : ";
        for (int i = 0; i < count; i++) {
            std::string v = base::xstring_utl::tostring(i+1);
            leafs.push_back(v);
            //std::cout <<  v << " ";
        }
        //  std::cout << std::endl;
  
        for (int index = 0; index < count; index++) {
            xmerkle_t<test_hash_mock_t, std::string> merkle;
            xmerkle_path_t<std::string> hash_path; 
            const std::string mpt_root = merkle.calc_root(leafs);
            //std::cout << "merkle path mpt_root " << mpt_root << std::endl;
            bool ret = merkle.calc_path(leafs, index, hash_path.get_levels_for_write());
            ASSERT_EQ(ret, true);
            const std::vector<xmerkle_path_node_t<std::string>> & levels = hash_path.get_levels();
            //ASSERT_EQ(levels.size(), 2);
            /*std::cout << "merkle path for index " << index << std::endl;
            for (size_t i = 0; i < levels.size(); i++) {
                std::cout << "i = " << i << std::endl;
                std::cout << "level=" << levels[i].level << std::endl;
                std::cout << "pos=" << levels[i].pos << std::endl;
                std::cout << "signature=" << levels[i].signature << std::endl;
            }*/
            ret = merkle.validate_path(leafs[index], mpt_root, hash_path.get_levels());
            //std::cout << "ret= " << ret  << std::endl;
            ASSERT_EQ(ret, true);   
        }
    }

}

TEST_F(test_merkle, merkle_invalid_path_level_signature) {   
    int leafs_num = 10;
    int count = 0;
    for (count = 0; count < leafs_num; count++) {   
        std::vector<std::string> leafs;

        //std::cout << "merkle leasf number  " << count << " leafs is : ";
        for (int i = 0; i < count; i++) {
            std::string v = base::xstring_utl::tostring(i+1);
            leafs.push_back(v);
            //std::cout <<  v << " ";
        }
        //  std::cout << std::endl;
  
        for (int index = 0; index < count; index++) {
            xmerkle_t<test_hash_mock_t, std::string> merkle;
            xmerkle_path_t<std::string> hash_path; 
            const std::string mpt_root = merkle.calc_root(leafs);
            //std::cout << "merkle path mpt_root " << mpt_root << std::endl;
            bool ret = merkle.calc_path(leafs, index, hash_path.get_levels_for_write());
            ASSERT_EQ(ret, true);
            const std::vector<xmerkle_path_node_t<std::string>> & levels = hash_path.get_levels();
            //ASSERT_EQ(levels.size(), 2);
            //std::cout << "merkle path for index " << index << std::endl;

            //add err path 
            std::vector<xmerkle_path_node_t<std::string>> & err_levels = hash_path.get_levels_for_write();
            if (levels.size() > 1) {
                
                ASSERT_EQ(merkle.validate_path(leafs[index], mpt_root, hash_path.get_levels()), 1);
                //test error level, path is still right 
                err_levels[0].level += 1000; 
                ASSERT_EQ(merkle.validate_path(leafs[index], mpt_root, hash_path.get_levels()),1);
                //test error signature, path is error 
                err_levels[0].signature += 1; 
                ASSERT_EQ(merkle.validate_path(leafs[index], mpt_root, hash_path.get_levels()), 0);
            }
        }
    }
}
TEST_F(test_merkle, merkle_hash_compare) {

    int test_count = 100;
    for (int cur_count = 0; cur_count < test_count; ++cur_count) {
        std::vector<std::string> mpt_values;
        for(int i = 0; i < cur_count; ++i)  {
            std::string v = base::xstring_utl::tostring(i);
            mpt_values.push_back(v);
        }

        //old  
        top::base::xmerkle_t<top::utl::xsha2_256_t, uint256_t> merkle_obj_old;
        const std::string mpt_root_old = merkle_obj_old.calc_root(mpt_values);

        //new 
        top::base::xmerkle_t<top::utl::xsha2_256_t, uint256_t> merkle_obj_new(mpt_values);
        const std::string mpt_root_new = merkle_obj_new.calc_root_hash(mpt_values);
       
        //compare root hash
        xassert(mpt_root_old == mpt_root_new);
        //compare every hash path
        for (int j = 0; j < cur_count; ++j) {
            //old
            xmerkle_path_256_t hash_path_old;
            xassert(merkle_obj_old.calc_path(mpt_values, j, hash_path_old.get_levels_for_write()));
            xassert(merkle_obj_old.validate_path(mpt_values[j], mpt_root_old, hash_path_old.get_levels_for_write()));
            const std::vector<xmerkle_path_node_t<uint256_t>> & levels_old = hash_path_old.get_levels();
            base::xstream_t _stream_old(base::xcontext_t::instance());
            hash_path_old.serialize_to(_stream_old);
            std::string _path_bin_old = std::string((char *)_stream_old.data(), _stream_old.size());
            xassert(!_path_bin_old.empty());

            //new
            xmerkle_path_256_t hash_path_new;
            xassert(merkle_obj_new.calc_path_hash(mpt_values[j], hash_path_new.get_levels_for_write()));
            const std::vector<xmerkle_path_node_t<uint256_t>> & levels_ndw = hash_path_new.get_levels();
            xassert(merkle_obj_new.validate_path(mpt_values[j], mpt_root_new, hash_path_new.get_levels_for_write()));

            base::xstream_t _stream_new(base::xcontext_t::instance());
            hash_path_new.serialize_to(_stream_new);
            std::string _path_bin_new = std::string((char *)_stream_new.data(), _stream_new.size());
            xassert(!_path_bin_new.empty());

            xassert(_path_bin_old == _path_bin_new);
        }      
    }
}

TEST_F(test_merkle, merkle_hash_count_compare) {
#ifdef ENABLE_METRICS
    int test_count = 101;
    for (int cur_count = 0; cur_count < test_count; ++cur_count) {
        int64_t hash_path_total_old = 0;
        int64_t hash_path_total_new = 0;

        std::vector<std::string> mpt_values;
        for(int i = 0; i < cur_count; ++i)  {
            std::string v = base::xstring_utl::tostring(i);
            mpt_values.push_back(v);
        }     
        //old  
        int64_t hash_count_start = XMETRICS_GAUGE_GET_VALUE(metrics::cpu_hash_256_calc);
        top::base::xmerkle_t<top::utl::xsha2_256_t, uint256_t> merkle_obj_old;
        const std::string mpt_root_old = merkle_obj_old.calc_root(mpt_values);
        int64_t root_hash_count_old =  (XMETRICS_GAUGE_GET_VALUE(metrics::cpu_hash_256_calc) - hash_count_start);
        xassert(root_hash_count_old == hash_count_calc(cur_count));
        hash_path_total_old = root_hash_count_old;
    
        //new 
        hash_count_start =  XMETRICS_GAUGE_GET_VALUE(metrics::cpu_hash_256_calc);
        top::base::xmerkle_t<top::utl::xsha2_256_t, uint256_t> merkle_obj_new(mpt_values);
        const std::string mpt_root_new = merkle_obj_new.calc_root_hash(mpt_values);
        int64_t root_hash_count_new =  (XMETRICS_GAUGE_GET_VALUE(metrics::cpu_hash_256_calc) - hash_count_start);
        xassert(root_hash_count_new ==  hash_count_calc(cur_count));
        hash_path_total_new = root_hash_count_new;

        //compare root hash
        xassert(mpt_root_old == mpt_root_new);
        xassert(root_hash_count_old == root_hash_count_new);

        //compare every hash path
        for (int j = 0; j < cur_count; ++j) {
            //old
            xmerkle_path_256_t hash_path_old;
            hash_count_start =  XMETRICS_GAUGE_GET_VALUE(metrics::cpu_hash_256_calc);
            xassert(merkle_obj_old.calc_path(mpt_values, j, hash_path_old.get_levels_for_write()));
            hash_path_total_old += (XMETRICS_GAUGE_GET_VALUE(metrics::cpu_hash_256_calc) - hash_count_start);
            xassert(merkle_obj_old.validate_path(mpt_values[j], mpt_root_old, hash_path_old.get_levels_for_write()));
            const std::vector<xmerkle_path_node_t<uint256_t>> & levels_old = hash_path_old.get_levels();
            base::xstream_t _stream_old(base::xcontext_t::instance());
            hash_path_old.serialize_to(_stream_old);
            std::string _path_bin_old = std::string((char *)_stream_old.data(), _stream_old.size());
            xassert(!_path_bin_old.empty());

            //new
            xmerkle_path_256_t hash_path_new;
            hash_count_start = XMETRICS_GAUGE_GET_VALUE(metrics::cpu_hash_256_calc);
            xassert(merkle_obj_new.calc_path_hash(mpt_values[j], hash_path_new.get_levels_for_write()));
            hash_path_total_new += (XMETRICS_GAUGE_GET_VALUE(metrics::cpu_hash_256_calc) - hash_count_start);
            const std::vector<xmerkle_path_node_t<uint256_t>> & levels_ndw = hash_path_new.get_levels();
            xassert(merkle_obj_new.validate_path(mpt_values[j], mpt_root_new, hash_path_new.get_levels_for_write()));
            base::xstream_t _stream_new(base::xcontext_t::instance());
            hash_path_new.serialize_to(_stream_new);
            std::string _path_bin_new = std::string((char *)_stream_new.data(), _stream_new.size());
            xassert(!_path_bin_new.empty());

            xassert(_path_bin_old == _path_bin_new);
        }

        int test_count = cur_count; 
        //std::cout << "leafs num " << test_count << ":" << std::endl;
        /*std::cout << "old root:" << root_hash_count_old << " hash ,test " << test_count << " hash_path "\
        << hash_path_total_old << " hash" << std::endl;
        std::cout << "new root:" << root_hash_count_new << " hash ,test " << test_count << " hash_path "\
        << hash_path_total_new << " hash" << std::endl;*/
        xassert(hash_path_total_old >= hash_path_total_old);
        xassert(hash_path_total_old == (hash_count_calc(cur_count)*(cur_count+1)));
        xassert(hash_path_total_new == hash_count_calc(cur_count));
    }
#endif
}
