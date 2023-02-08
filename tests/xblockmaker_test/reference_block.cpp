#include "gtest/gtest.h"

#include "test_common.hpp"
#include "xblockmaker/xtable_maker.h"
#include "xchain_fork/xutility.h"

using namespace top;
using namespace top::base;
using namespace top::mbus;
using namespace top::store;
using namespace top::data;
using namespace top::mock;
using namespace top::blockmaker;

class reference_block : public testing::Test {
protected:
    void SetUp() override {
        base::xvblock_fork_t::instance().init(chain_fork::xutility_t::is_block_forked);
    }

    void TearDown() override {
    }

    // [0, 21] unit hash for T00000LZbJfje6hW6MmG43h6EaTkme3ZqggGoUvF
    const std::vector<std::string> oUvF_unit{"12ee197d461f227f6e05dacb259d6431a96d53823367f7112de62848a409fca6"
                                            ,"1357d1c304ab55f26b843604df43c4e5e33f3e7196b6337470aff928104f8267"
                                            ,"2f53c7233b82f5eb039422820b77133afc0713ca8dd2341e557a7eec1f88d06b"
                                            ,"8171b37fc419c3f5f19d2a4e3e32109adc6eb4dd265724374b6ba3441e2a82d2"
                                            ,"b249add7e7558ed304bd2f7075303dae5e1296f2ff6886e129736a63e57a6b85"
                                            ,"5e6476693d2ea0ece4fc49214c5f99b965bd3823f42835d0e2dd3c17f0b70e8f"
                                            ,"b48798db3b5bc6a2d822f3fb2c2966c48c6186879056b474c0282a1e25a3741a"
                                            ,"f4cbdde184a8fdbf81cece0a46001b6bd959141aa1928c1c9d676d0748a16bc9"
                                            ,"3a2b53d053d59ea80ae50728dc9b06977674cbf8f38de3b8c1cd7215cd4ce7ed"
                                            ,"9159eb7552f02e2db1afdc7ca5c222eff7b99e8a087c8215d04f0e37e960a89a"
                                            ,"30203db131be542b84f64523f759454a297e1e4faa1279ad1b7dcd728953a2a4"
                                            ,"9c330ceeb47afe425f908ff9a1f53c588626fcf754fc85ab27996f9f9acdabb5"
                                            ,"f1ffa8ead5ee3f14c7dc38c6f36bb64f25990994ec57e99362510187daa76938"
                                            ,"7c484798ce746df512851014d5fdefb119db73c88778bca2a5b4e0f8b2bc3122"
                                            ,"17c24ad3a4a4037dfbcf8c78b13c210aae68d1f3cfe7f42c8cf5dc6a4dd77419"
                                            ,"57112a8d51d1652bfd828cea084a26d845cd96526671707f7e7680bdfebccbb8"
                                            ,"eb661f4f2179e86b302e538ec37f43602fdf8cfb718326a168f0b794a161ed3c"
                                            ,"fd5e004c54d5e17b48faac77e7fdfbbf8638838aedf079dce80c3fad3d95f401"
                                            ,"cf47e4f8fecd4fef977dc4ba56a37e64df7b545c729ef321dac4b9128385dfcf"
                                            ,"39bb5c631b65192fa3a9607813bfe99ea416d8e54894ac511fe6d668107c72f4"
                                            ,"502af97e7560ad92c5b4623043aac203e2b84e4f52d0dc23266e76f58d421344"
                                            ,"23ee11c5190f4e50a46b715126875124536cdd57259c7648e5d8cfbc29afcb76"};

    // [0, 21] table hash for Ta0000@1
    const std::vector<std::string>  table_1{""
                                            ,"7a4f3c214a2e06ead1cf03c2f161dbc1fd82194754655985860c94618b4cad80"
                                            ,"4d04285961be987f46ccad8ffc811e2b335185596c5f80969eb0e5b63f375cc0"
                                            ,"ac91f3312feb073afe45d21c7d1444321b9b86b2d7a5e19cf49f98664a931696"
                                            ,"49c5bb1c2a20a19bced9b7b688071616c4a0dffc04aae2261255e242e979af44"
                                            ,"de11057267e53d63bd6c7bdbe2334d3b6144c7d3c4bdd3b25d25f1563e340577"
                                            ,"a14262b630b55f3c21016bf76a76afa17be4a3a0694dc96db9ca99075cc1dcc9"
                                            ,"afcb624b9f8501f50e55c8a0b58edbf5e8cfc5d39f0417947288be666eb87ce7"
                                            ,"f299d28e697ac027c0a47ce2a1b6a83a29453a738ed4d8a123becbe359439a4f"
                                            ,"6f2aef90d51e21ce258c4e1bde8de76a19bccbd2918c1e804742e2b2d9c6549c"
                                            ,"c0af2aba110b096b13f407195353c7c0fa255cfceb031f4256a13aa7165ffa92"
                                            ,"01f0de472dcf05717ce5f40da7257852aa1402d68391bee79d664132d43d7806"
                                            ,"c5563c0bef309286134d0865758b5ccbc80abffe5974339eeb3530c15c5d2b49"
                                            ,"8252274852cd938348f2f5c2056c40fe6f212ec22635f52bfc6926b2369e79f0"
                                            ,"0f513ceff8ad4f942084fae9faa442707ee6a57224e72ac981013db7efdbe923"
                                            ,"65085c0f9c5dca1cb83e3a1ae5005d63697488d82770605ea916d2ec43be6435"
                                            ,"54fbd8f73d6535e4cf3705115a0aef826e1e22a3240796516a9573f10c8b4a12"
                                            ,"d5c5a89fda873274d4cc1f731a3f36d6a4911ae22beeeecd6fb335c2d148c222"
                                            ,"88179b33e1cc5997e2d3da97f7cd0d00f2900075187b498342ededd73817f6b4"
                                            ,"b17b1b2793943964a50d6309870fbe13b7036b119cef782c0f1e605a618d4e85"
                                            ,"8bab9d1ed1cfa21ad7b494a3705e293511599db991108f1d76a3dc82206b2641"
                                            ,"f332665cd73a2ec853dbe8332f9abae72eed2213bb331c10903def64035a3c09"
                                            ,"9ff33aa63b81d721037d8642db7ccc02d8ce8b666d5178efdf45b415ac3ff8a1"};

    // [0, 21] tx hash
    const std::vector<std::string>  tx_hash{"92037798c8a03d20ad67457ce8ade3c3f5e1beb8bcf000c0dcb443d3f119cd52"
                                            ,"b41e8bf832d035cce37d6dfd7e22c96db53160116831003720a20c213a27585e"
                                            ,"0731e152d8cac55d7bd62fcb1e53894ede82c3dfd785b6aec9c51fdfe33149fa"
                                            ,"4cd903ad50e68b7af7f86d737e193a4344859ac180d71b09a84866ff8eeb8859"
                                            ,"acbcd2e8c578cda1342c29b7111389a2aed3d854f03140d8b243e0d5c859ed26"
                                            ,"c5d13642ea532f216cf5d97a5024062b242479bcf66ae34c581a2bd66dbc6e66"
                                            ,"cc0a5e0a244943eeb484d8b988ec7dacadaf9b60e59d9fac5973cc9f22595b2b"
                                            ,"57a07d947f5b0d93c20eab2439facc1e064de33d1ca327dab5c9bd42eaa57edb"
                                            ,"7858ce583b544c55cd295b2bf5bdad135d13c43b7b033c2d2e9da66a6605545a"
                                            ,"4127341944ce22fa15938f58578c4b54cdcd45a62feb4a1ef8c98fac91348d31"
                                            ,"538e4999d36d6784ecaf2d2f7ec6f50c7069202d0a1c391555885c9cd4250e33"
                                            ,"5ba70eab06da96086292ab651f2d2cc1ed5a144df76b05102fb1124b4a970654"
                                            ,"bc487d8fde4452e4c39e855ff00e56883ac69c7efa2bde6cc02216482ea66e44"
                                            ,"45f4434cc88ea49f20e59c9b064bd2ad99551f1ce5cf39171d928470ed2866e6"
                                            ,"1e2d2a0f439096ab8ca2d3b1b982f28f96ab23faceb11a617637dd29d33ab008"
                                            ,"b17ec0c225bae9800f77c6c476b83b48a4798c6dbb64e9194db8326cd9d98998"
                                            ,"83a83533549aef94e8f22f218789dfc45a24c038bce5a3da901748f1478ef627"
                                            ,"bba64a366e412168dd1ff214fabaccc827b06cca5d8586549a92e6bac93091b2"
                                            ,"7eabd48f9371b2a9d20442bf08d9bc5776174704582a31efd30e2091ed34f62f"
                                            ,"c261956380e1328eb3a19b6db3b58df1570c314d86f2b269ed06f0e1a8c25b38"
                                            ,"577728decbbe2318f5e7fe73a1fa3f434c97f7efb71322c1968414a50255d113"};
        
};

#ifndef XCHAIN_FORKED_BY_DEFAULT
TEST_F(reference_block, creation) {
    xblockmaker_resources_ptr_t resources = std::make_shared<test_xblockmaker_resources_t>();

    mock::xdatamock_table mocktable(1, 2, true);
    std::string table_addr = mocktable.get_account();
    std::vector<std::string> unit_addrs = mocktable.get_unit_accounts();
    std::string from_addr = unit_addrs[0];
    std::string to_addr = unit_addrs[1];

    std::vector<xblock_ptr_t> all_gene_units = mocktable.get_all_genesis_units();
    for (auto & v : all_gene_units) {
        resources->get_blockstore()->store_block(base::xvaccount_t(v->get_account()), v.get());
        // std::cout << v->dump() << std::endl << std::endl;
        // std::cout << v->get_account() << " " << v->get_height() << " " << v->get_block_hash_hex_str() << std::endl;
        if (v->get_account() == "T00000LZbJfje6hW6MmG43h6EaTkme3ZqggGoUvF") {
            EXPECT_EQ(base::xstring_utl::to_hex(v->get_block_hash()), oUvF_unit[0]);
        }
    }

    uint64_t nonce = 0;
    uint64_t clock = 10;
    xtable_maker_ptr_t tablemaker = make_object_ptr<xtable_maker_t>(table_addr, resources);
    for (uint32_t i = 1; i <= 22; ++i) {
        auto tx = xdatamock_unit::make_fixed_transfer_tx(from_addr, to_addr, nonce);
        xcons_transaction_ptr_t constx = make_object_ptr<xcons_transaction_t>(tx.get());
        EXPECT_EQ(constx->get_digest_hex_str(), tx_hash[nonce]);
        // std::cout << "tx hash: " << constx->get_digest_hex_str() << std::endl << std::endl;
        std::vector<xcons_transaction_ptr_t> send_txs;
        send_txs.push_back(constx);

        xtablemaker_para_t table_para(mocktable.get_table_state(), mocktable.get_commit_table_state());
        table_para.set_origin_txs(send_txs);
        xblock_consensus_para_t proposal_para = mocktable.init_consensus_para(clock);

        xtablemaker_result_t table_result;
        xblock_ptr_t proposal_block = tablemaker->make_proposal(table_para, proposal_para, table_result);
        xassert(proposal_block != nullptr);
        xassert(proposal_block->get_height() == i);
        EXPECT_EQ(proposal_block->get_block_version(), 256);

        mocktable.do_multi_sign(proposal_block, true);
        mocktable.on_table_finish(proposal_block);
        resources->get_blockstore()->store_block(mocktable, proposal_block.get());

        // std::cout << proposal_block->dump() << std::endl << std::endl;
        // std::cout << proposal_block->get_account() << " " << proposal_block->get_height() << " " << proposal_block->get_block_hash_hex_str() << std::endl;
        EXPECT_EQ(base::xstring_utl::to_hex(proposal_block->get_block_hash()), table_1[i]);

        std::vector<xobject_ptr_t<base::xvblock_t>> units;
        proposal_block->extract_sub_blocks(units);        
        if (!proposal_block->is_fullblock()) {
            EXPECT_EQ(units.size(), 1);
            xobject_ptr_t<data::xblock_t> unit = dynamic_xobject_ptr_cast<data::xblock_t>(units[0]);
            // std::cout << unit->dump() << std::endl << std::endl;
            // std::cout << unit->get_account() << " " << unit->get_height() << " " << unit->get_block_hash_hex_str() << std::endl;
            EXPECT_EQ(base::xstring_utl::to_hex(unit->get_block_hash()), oUvF_unit[unit->get_height()]);
            nonce++;
        }
    }
}
#endif
