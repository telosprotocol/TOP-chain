#include "gtest/gtest.h"
#include "xbase/xmem.h"
#include "xconfig/xconfig_register.h"
#include "xcrypto/xckey.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xtransaction_v1.h"
#include "xdata/xtransaction_v2.h"
#include "xdata/xtransaction_v3.h"
#include "xdata/xethtransaction.h"
#include "xdata/xtx_factory.h"
#include "xbasic/xhex.h"
#include "xvledger/xvblock.h"
#include "xcommon/rlp.h"
#include "xdata/xblock.h"
#include "xdata/xblockdump.h"

using namespace top;
using namespace top::base;
using namespace top::data;

size_t GetFileSize(FILE* file)
{
    size_t file_size = 0;
    fseek(file, 0, SEEK_END);
    file_size = static_cast<size_t>(ftell(file));
    fseek(file, 0, SEEK_SET);
    return file_size;
}

class test_block_parser : public testing::Test {
protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

void block_parse_dump(base::xvblock_t* _block)
{
}

TEST_F(test_block_parser, block_file_read)
{
    FILE* file = fopen("big_block_Ta0004.log", "rb");
    if (file) {
        const size_t file_size = GetFileSize(file);
        char* const buffer = new char[file_size];
        std::cout << "  file_size " << file_size << '\n';
        size_t bytes_last_read = 0;
        size_t bytes_read = 0;

        do {
            bytes_last_read = fread(buffer + bytes_read, 1, file_size - bytes_read, file);
            bytes_read += bytes_last_read;
        } while (bytes_last_read > 0 && bytes_read < file_size);

        const std::string file_content(buffer, bytes_read);
        delete[] buffer;

        if (file_content.size() > 0) {
            // std::cout << " block_hex_content " << file_content << '\n';
            std::string block_content = base::xstring_utl::from_hex(file_content);
            base::xstream_t stream(base::xcontext_t::instance(), (uint8_t*)block_content.c_str(), (uint32_t)block_content.size());

            base::xvblock_t* _data_obj = xblock_t::full_block_read_from(stream);
            if (_data_obj != nullptr) {
                data::xblockdump_t::dump_tableblock(_data_obj);
            } else {
                std::cout << " block full_block_read_from failed" << '\n';
            }
        } else {
            std::cout << " no found block content from file.txt" << '\n';
        }
    } else {
        std::cout << " error read file.txt" << '\n';
    }
}

TEST_F(test_block_parser, block_valid_check_version_50000_BENCH)
{
    FILE* file = fopen("block_bin_ver50000.log", "rb");
    if (file == NULL) {
        std::cout << " error read file.txt" << std::endl;
        return;
    }

    const size_t file_size = GetFileSize(file);
    char* const buffer = new char[file_size];
    std::cout << "  file_size " << file_size << '\n';
    size_t bytes_last_read = 0;
    size_t bytes_read = 0;

    do {
        bytes_last_read = fread(buffer + bytes_read, 1, file_size - bytes_read, file);
        bytes_read += bytes_last_read;
    } while (bytes_last_read > 0 && bytes_read < file_size);

    const std::string file_content(buffer, bytes_read);
    delete[] buffer;

    if (file_content.size() == 0) {
        std::cout << " no found block content from file.txt" << std::endl;
        return;
    }

    // std::cout << " block_hex_content " << file_content << '\n';
    std::string block_content = base::xstring_utl::from_hex(file_content);
    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t*)block_content.c_str(), (uint32_t)block_content.size());

    base::xvblock_t* block = xblock_t::full_block_read_from(stream);
    if (block == nullptr) {
        std::cout << " block full_block_read_from failed" << std::endl;
        return;
    }

    std::cout << "block=" << block->dump() << std::endl;
    std::cout << "qcert=" << block->get_cert()->dump() << std::endl;
    std::cout << "header=" << block->get_header()->dump() << std::endl;
{    
    std::cout << "block_hash=" << base::xstring_utl::to_hex(block->get_block_hash()) << std::endl;
    std::cout << "calc_block_hash=" << base::xstring_utl::to_hex(block->build_block_hash()) << std::endl;
    xassert(block->get_block_hash() == block->build_block_hash());
}
{
    std::cout << "header_hash=" << base::xstring_utl::to_hex(block->get_header_hash()) << std::endl;
    std::cout << "calc_header_hash=" << base::xstring_utl::to_hex(block->build_header_hash()) << std::endl;
    xassert(block->get_header_hash() == block->build_header_hash());    
}

{
    std::string _bin = block->get_input_data();
    std::string input_hash = block->get_input_hash();
    std::string calc_bin_hash = block->get_cert()->hash(_bin);    
    std::error_code ec;
    auto input_object = block->load_input(ec);
    std::string resources_hash = input_object->get_resources_hash();
    std::string resource_data = input_object->get_resources_data();
    std::string calc_resource_hash = block->get_cert()->hash(resource_data);
    std::cout << "input_bin=" << _bin.size() << ",input_hash=" << base::xstring_utl::to_hex(input_hash) << std::endl;
    std::cout << "input_calc_bin_hash=" << base::xstring_utl::to_hex(calc_bin_hash) << std::endl;
    std::cout << "input_calc_resource_hash=" << base::xstring_utl::to_hex(calc_resource_hash) << std::endl;
    std::cout << "input_resources_hash=" << base::xstring_utl::to_hex(resources_hash) << std::endl;

    if (block->get_block_version() < base::enum_xvblock_fork_version_6_0_0) {
        xassert(block->get_input_hash().empty());  // not use header input_hash
        xassert(resources_hash == calc_resource_hash);// use input object resource hash
        xassert(_bin == resource_data);
    }
}
{
    std::string _bin = block->get_output_data();
    std::string output_hash = block->get_output_hash();
    std::string calc_bin_hash = block->get_cert()->hash(_bin);    
    std::error_code ec;
    auto output_object = block->load_output(ec);
    std::string resources_hash = output_object->get_resources_hash();
    std::string resource_data = output_object->get_resources_data();
    std::string calc_resource_hash = block->get_cert()->hash(resource_data);
    std::cout << "output_bin=" << _bin.size() << ",output_hash=" << base::xstring_utl::to_hex(output_hash) << std::endl;
    std::cout << "output_calc_bin_hash=" << base::xstring_utl::to_hex(calc_bin_hash) << std::endl;
    std::cout << "output_calc_resource_hash=" << base::xstring_utl::to_hex(calc_resource_hash) << std::endl;
    std::cout << "output_resources_hash=" << base::xstring_utl::to_hex(resources_hash) << std::endl;

    if (block->get_block_version() < base::enum_xvblock_fork_version_6_0_0) {
        xassert(block->get_output_hash().empty());  // not use header input_hash
        xassert(resources_hash == calc_resource_hash);// use input object resource hash
        xassert(_bin == resource_data);
    }
}

    xassert(block->is_valid());
    xassert(block->is_valid(true));
}