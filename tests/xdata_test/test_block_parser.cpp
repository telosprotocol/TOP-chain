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
#include "xevm_common/rlp.h"
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