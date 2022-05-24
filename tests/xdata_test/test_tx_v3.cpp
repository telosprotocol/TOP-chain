#include "gtest/gtest.h"
#include "xbase/xmem.h"
#include "xconfig/xconfig_register.h"
#include "xcrypto/xckey.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xtransaction_v1.h"
#include "xdata/xtransaction_v2.h"
#include "xdata/xtransaction_v3.h"
#include "xevm_common/rlp.h"

using namespace top;
using namespace top::base;
using namespace top::data;

class test_tx_v3 : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_tx_v3, serialize) {
    xtransaction_v3_ptr_t tx = make_object_ptr<xtransaction_v3_t>();

    xJson::Value jv;
    jv["id"] = "12231";
    jv["jsonrpc"] = "2.0";
    jv["method"] = "eth_sendRawTransaction";
    jv["params"][0] = "0x02f8708203ff80822710822710827b0c94b7762d8dbd7e5c023ff99402b78af7c13b01eec1881bc16d674ec8000080c001a0d336694faa98f9cd69792ee30f9979f906f997d7562a0cb86d109f3476643a65a0679a7f510b12d48f9d1637884245229b7037b4f4094d1fb30e0f4f5cc77388ec";
    tx->construct_from_json(jv);
    base::xstream_t stream(base::xcontext_t::instance());
    tx->do_write(stream);
    std::cout << "run contract tx v3 size: " << stream.size() << std::endl;

    xtransaction_v3_ptr_t tx_r = make_object_ptr<xtransaction_v3_t>();
    tx_r->do_read(stream);
    EXPECT_EQ(tx_r->get_source_addr(), "T6000483d85d169f750ad626dc10565043a802b5499a3f");
}


TEST_F(test_tx_v3, exception) {
    {
        xtransaction_v3_ptr_t tx = make_object_ptr<xtransaction_v3_t>();
        xJson::Value jv;
        jv["id"] = "12231";
        jv["jsonrpc"] = "2.0";
        jv["method"] = "eth_sendRawTransaction";
        tx->construct_from_json(jv);
        base::xstream_t stream(base::xcontext_t::instance());
        tx->do_write(stream);
        std::cout << "run contract tx v3 size: " << stream.size() << std::endl;

        xtransaction_v3_ptr_t tx_r = make_object_ptr<xtransaction_v3_t>();
        tx_r->do_read(stream);
    }
    {
        xtransaction_v3_ptr_t tx = make_object_ptr<xtransaction_v3_t>();
        xJson::Value jv;
        jv["id"] = "12231";
        jv["jsonrpc"] = "2.0";
        jv["method"] = "eth_sendRawTransaction";
        jv["params"] = "0x02f8708203ff80822710822710827b0c94b7762d8dbd7e5c023ff99402b78af7c13b01eec1881bc16d674ec8000080c001a0d336694faa98f9cd69792ee30f9979f906f997d7562a0cb86d109f3476643a65a0679a7f510b12d48f9d1637884245229b7037b4f4094d1fb30e0f4f5cc77388ec";
        tx->construct_from_json(jv);
        base::xstream_t stream(base::xcontext_t::instance());
        tx->do_write(stream);
        std::cout << "run contract tx v3 size: " << stream.size() << std::endl;

        xtransaction_v3_ptr_t tx_r = make_object_ptr<xtransaction_v3_t>();
        tx_r->do_read(stream);
    }

    {
        xtransaction_v3_ptr_t tx = make_object_ptr<xtransaction_v3_t>();
        xJson::Value jv;
        jv["id"] = "12231";
        jv["jsonrpc"] = "2.0";
        jv["method"] = "eth_sendRawTransaction";
        jv["params"][0] = 1;
        tx->construct_from_json(jv);
        base::xstream_t stream(base::xcontext_t::instance());
        tx->do_write(stream);
        std::cout << "run contract tx v3 size: " << stream.size() << std::endl;

        xtransaction_v3_ptr_t tx_r = make_object_ptr<xtransaction_v3_t>();
        tx_r->do_read(stream);
    }

    {
        xtransaction_v3_ptr_t tx = make_object_ptr<xtransaction_v3_t>();
        xJson::Value jv;
        jv["id"] = "12231";
        jv["jsonrpc"] = "2.0";
        jv["method"] = "eth_sendRawTransaction";
        jv["params"][0] = "0x0";
        tx->construct_from_json(jv);
        base::xstream_t stream(base::xcontext_t::instance());
        tx->do_write(stream);
        std::cout << "run contract tx v3 size: " << stream.size() << std::endl;

        xtransaction_v3_ptr_t tx_r = make_object_ptr<xtransaction_v3_t>();
        tx_r->do_read(stream);
    }

    //0xf8671185174876e80082754194d8ae0197425c0ea651264b06978580dcb62f3c918203e880820a94a01d3aac56ba977387ae9e8fb87b9baee0759c75c306ba8ef9983ff20d284f1c839ff9d7100bceeb191ddd4ca9dfd03b1be1717660572d77adeb16d514c0ed2634
    {
        xtransaction_v3_ptr_t tx = make_object_ptr<xtransaction_v3_t>();
        xJson::Value jv;
        jv["id"] = "12231";
        jv["jsonrpc"] = "2.0";
        jv["method"] = "eth_sendRawTransaction";
        jv["params"][0] = "0xf8671185174876e80082754194d8ae0197425c0ea651264b06978580dcb62f3c918203e880820a94a01d3aac56ba977387ae9e8fb87b9baee0759c75c306ba8ef9983ff20d284f1c839ff9d7100bceeb191ddd4ca9dfd03b1be1717660572d77adeb16d514c0ed2634";
        tx->construct_from_json(jv);
        base::xstream_t stream(base::xcontext_t::instance());
        tx->do_write(stream);
        std::cout << "run contract tx v3 size: " << stream.size() << std::endl;

        std::cout << "sign :" << top::evm_common::toHex(tx->get_authorization()) << std::endl;
    }

    {
        xtransaction_v3_ptr_t tx = make_object_ptr<xtransaction_v3_t>();
        xJson::Value jv;
        jv["id"] = "12231";
        jv["jsonrpc"] = "2.0";
        jv["method"] = "eth_sendRawTransaction";
        jv["params"][0] = "0xeef8708203ff80822710822710827b0c94b7762d8dbd7e5c023ff99402b78af7c13b01eec1881bc16d674ec8000080c001a0d336694faa98f9cd69792ee30f9979f906f997d7562a0cb86d109f3476643a65a0679a7f510b12d48f9d1637884245229b7037b4f4094d1fb30e0f4f5cc77388ec1111";
        tx->construct_from_json(jv);
        base::xstream_t stream(base::xcontext_t::instance());
        tx->do_write(stream);
        std::cout << "run contract tx v3 size: " << stream.size() << std::endl;

        std::cout << "sign :" << top::evm_common::toHex(tx->get_authorization()) << std::endl;
    }

    {
        //generate signed transaction
        xobject_ptr_t<top::data::eip_1559_tx> tx = make_object_ptr<top::data::eip_1559_tx>();
        tx->accesslist = "";
        tx->chainid = 1023;
        tx->data = "";
        tx->gas = rand() % 100000;
        tx->nonce = rand();
        top::xbyte_buffer_t randstr = random_bytes(20);
        tx->to.append((char*)randstr.data(), randstr.size());
        tx->value = rand() * 1000000000;
        tx->max_fee_per_gas = rand() % 100000;
        tx->max_priority_fee_per_gas = rand() % 100000;

        top::evm_common::bytes encodedtmp;
        top::evm_common::append(encodedtmp, top::evm_common::RLP::encode(tx->chainid));
        top::evm_common::append(encodedtmp, top::evm_common::RLP::encode(tx->nonce));
        top::evm_common::append(encodedtmp, top::evm_common::RLP::encode(tx->max_priority_fee_per_gas));
        top::evm_common::append(encodedtmp, top::evm_common::RLP::encode(tx->max_fee_per_gas));
        top::evm_common::append(encodedtmp, top::evm_common::RLP::encode(tx->gas));
        top::evm_common::append(encodedtmp, top::evm_common::RLP::encode(tx->to));
        top::evm_common::append(encodedtmp, top::evm_common::RLP::encode(tx->value));
        top::evm_common::append(encodedtmp, top::evm_common::RLP::encode(tx->data));
        top::evm_common::append(encodedtmp, top::evm_common::fromHex("c0"));

        top::evm_common::bytes encoded;
        top::evm_common::append(encoded, static_cast<uint8_t>(2));
        top::evm_common::append(encoded, top::evm_common::RLP::encodeList(encodedtmp));

        std::string strEncoded;
        strEncoded.append((char*)encoded.data(), encoded.size());
        top::uint256_t hash = top::utl::xkeccak256_t::digest(strEncoded);

        uint8_t prikey[32];

        top::evm_common::bytes vprikey = top::evm_common::fromHex("3963ed01bce983f8829174b13d3417716ff604bfb0fed07634288df8b146f20f");

        memcpy(prikey, vprikey.data(), vprikey.size());

        top::utl::xecprikey_t key(prikey);
        top::utl::xecdsasig_t sig = key.sign(hash);

        char szOutput[65] = {0};
        top::utl::xsecp256k1_t::get_publickey_from_signature(sig, hash, (uint8_t*)szOutput);

        tx->signV = sig.get_recover_id();
        top::evm_common::bytes r;
        std::string strSignature;
        strSignature.append((char*)sig.get_raw_signature(), 64);
        std::string strR = strSignature.substr(0, 32); 
        std::string strS = strSignature.substr(32);
        r.insert(r.begin(), strR.begin(), strR.end());
        tx->signR = top::evm_common::fromBigEndian<top::evm_common::u256>(r);
        top::evm_common::bytes s;
        s.insert(s.begin(), strS.begin(), strS.end());
        tx->signS = top::evm_common::fromBigEndian<top::evm_common::u256>(s);


        encodedtmp.clear();
        top::evm_common::append(encodedtmp, top::evm_common::RLP::encode(tx->chainid));
        top::evm_common::append(encodedtmp, top::evm_common::RLP::encode(tx->nonce));
        top::evm_common::append(encodedtmp, top::evm_common::RLP::encode(tx->max_priority_fee_per_gas));
        top::evm_common::append(encodedtmp, top::evm_common::RLP::encode(tx->max_fee_per_gas));
        top::evm_common::append(encodedtmp, top::evm_common::RLP::encode(tx->gas));
        top::evm_common::append(encodedtmp, top::evm_common::RLP::encode(tx->to));
        top::evm_common::append(encodedtmp, top::evm_common::RLP::encode(tx->value + 1));
        top::evm_common::append(encodedtmp, top::evm_common::RLP::encode(tx->data));
        top::evm_common::append(encodedtmp, top::evm_common::fromHex("c0"));
        top::evm_common::append(encodedtmp, top::evm_common::RLP::encode(tx->signV));
        top::evm_common::append(encodedtmp, top::evm_common::RLP::encode(strR));
        top::evm_common::append(encodedtmp, top::evm_common::RLP::encode(strS));

        encoded.clear();
        top::evm_common::append(encoded, static_cast<uint8_t>(2));
        top::evm_common::append(encoded, top::evm_common::RLP::encodeList(encodedtmp));

        xtransaction_v3_ptr_t tx_r = make_object_ptr<xtransaction_v3_t>();
        std::string strParams;
        strParams.append("0x");
        strParams.append(top::evm_common::toHex(std::string((char*)encoded.data(), encoded.size())));
        xJson::Value jv;
        jv["id"] = "12231";
        jv["jsonrpc"] = "2.0";
        jv["method"] = "eth_sendRawTransaction";
        jv["params"][0] = strParams;
        tx_r->construct_from_json(jv);

        EXPECT_EQ(tx_r->sign_check(), true);

        xpublic_key_t pub_key(std::string(szOutput, 65));
        EXPECT_EQ(tx_r->pub_key_sign_check(pub_key), false);
    }
}


TEST_F(test_tx_v3, serialize_by_base) {
    xtransaction_v3_ptr_t tx = make_object_ptr<xtransaction_v3_t>();

    xJson::Value jv;
    jv["id"] = "12231";
    jv["jsonrpc"] = "2.0";
    jv["method"] = "eth_sendRawTransaction";
    jv["params"][0] = "0x02f8708203ff80822710822710827b0c94b7762d8dbd7e5c023ff99402b78af7c13b01eec1881bc16d674ec8000080c001a0d336694faa98f9cd69792ee30f9979f906f997d7562a0cb86d109f3476643a65a0679a7f510b12d48f9d1637884245229b7037b4f4094d1fb30e0f4f5cc77388ec";
    tx->construct_from_json(jv);
    base::xstream_t stream(base::xcontext_t::instance());
    tx->do_write(stream);
    std::cout << "run contract tx v3 size: " << stream.size() << std::endl;

    base::xstream_t stream1(xcontext_t::instance());
    tx->serialize_to(stream1);

    xtransaction_ptr_t tx_r;
    data::xtransaction_t::set_tx_by_serialized_data(tx_r, std::string((char*)stream1.data(), stream1.size()));
    EXPECT_EQ(tx_r->get_source_addr(), "T6000483d85d169f750ad626dc10565043a802b5499a3f");
}

TEST_F(test_tx_v3, rlp_serialize) {
    uint64_t sum = 0;
    for (int i = 0; i < 1000; i++)
    {
        uint64_t num1 = rand();
        uint64_t num2 = rand();
        uint64_t num3 = rand();
        uint64_t num4 = rand();
        uint64_t num5 = rand();

        std::string str1;
        std::string str2;
        std::string str3;
        std::string str4;
        std::string str5;
        top::xbyte_buffer_t randstr = random_bytes(32);
        str1.append((char*)randstr.data(), randstr.size());
        randstr = random_bytes(32);
        str2.append((char*)randstr.data(), randstr.size());
        randstr = random_bytes(32);
        str3.append((char*)randstr.data(), randstr.size());
        randstr = random_bytes(32);
        str4.append((char*)randstr.data(), randstr.size());
        randstr = random_bytes(32);
        str5.append((char*)randstr.data(), randstr.size());

        top::evm_common::bytes encoded;
        top::evm_common::append(encoded, top::evm_common::RLP::encode(num1));
        top::evm_common::append(encoded, top::evm_common::RLP::encode(num2));
        top::evm_common::append(encoded, top::evm_common::RLP::encode(num3));
        top::evm_common::append(encoded, top::evm_common::RLP::encode(num4));
        top::evm_common::append(encoded, top::evm_common::RLP::encode(num5));
        top::evm_common::append(encoded, top::evm_common::RLP::encode(str1));
        top::evm_common::append(encoded, top::evm_common::RLP::encode(str2));
        top::evm_common::append(encoded, top::evm_common::RLP::encode(str3));
        top::evm_common::append(encoded, top::evm_common::RLP::encode(str4));
        top::evm_common::append(encoded, top::evm_common::RLP::encode(str5));
        encoded = top::evm_common::RLP::encodeList(encoded);

        sum += encoded.size();
        top::evm_common::RLP::DecodedItem decoded = top::evm_common::RLP::decode(encoded);

        std::vector<std::string> vecData;
        for (int i = 0; i < (int)decoded.decoded.size(); i++) {
            std::string str(decoded.decoded[i].begin(), decoded.decoded[i].end());
            vecData.push_back(str);
        }

        EXPECT_EQ(top::evm_common::to_uint64(vecData[0]), num1);
        EXPECT_EQ(top::evm_common::to_uint64(vecData[1]), num2);
        EXPECT_EQ(top::evm_common::to_uint64(vecData[2]), num3);
        EXPECT_EQ(top::evm_common::to_uint64(vecData[3]), num4);
        EXPECT_EQ(top::evm_common::to_uint64(vecData[4]), num5);

        EXPECT_EQ(vecData[5], str1);
        EXPECT_EQ(vecData[6], str2);
        EXPECT_EQ(vecData[7], str3);
        EXPECT_EQ(vecData[8], str4);
        EXPECT_EQ(vecData[9], str5);
    }
    std::cout << "only rlp serialize:" << sum / 1000.0 << std::endl;
}

TEST_F(test_tx_v3, xtream_serialize) {
    uint64_t sum = 0;
    for (int i = 0; i < 1000; i++)
    {
        uint64_t num1 = rand();
        uint64_t num2 = rand();
        uint64_t num3 = rand();
        uint64_t num4 = rand();
        uint64_t num5 = rand();

        std::string str1;
        std::string str2;
        std::string str3;
        std::string str4;
        std::string str5;
        top::xbyte_buffer_t randstr = random_bytes(32);
        str1.append((char*)randstr.data(), randstr.size());
        randstr = random_bytes(32);
        str2.append((char*)randstr.data(), randstr.size());
        randstr = random_bytes(32);
        str3.append((char*)randstr.data(), randstr.size());
        randstr = random_bytes(32);
        str4.append((char*)randstr.data(), randstr.size());
        randstr = random_bytes(32);
        str5.append((char*)randstr.data(), randstr.size());

        top::base::xstream_t stream(top::base::xcontext_t::instance());
        stream.write_compact_var(num1);
        stream.write_compact_var(num2);
        stream.write_compact_var(num3);
        stream.write_compact_var(num4);
        stream.write_compact_var(num5);
        stream.write_compact_var(str1);
        stream.write_compact_var(str2);
        stream.write_compact_var(str3);
        stream.write_compact_var(str4);
        stream.write_compact_var(str5);

        sum += stream.size();
        uint64_t tmpnum1;
        uint64_t tmpnum2;
        uint64_t tmpnum3;
        uint64_t tmpnum4;
        uint64_t tmpnum5;

        std::string tmpstr1;
        std::string tmpstr2;
        std::string tmpstr3;
        std::string tmpstr4;
        std::string tmpstr5;

        stream.read_compact_var(tmpnum1);
        stream.read_compact_var(tmpnum2);
        stream.read_compact_var(tmpnum3);
        stream.read_compact_var(tmpnum4);
        stream.read_compact_var(tmpnum5);

        stream.read_compact_var(tmpstr1);
        stream.read_compact_var(tmpstr2);
        stream.read_compact_var(tmpstr3);
        stream.read_compact_var(tmpstr4);
        stream.read_compact_var(tmpstr5);


        EXPECT_EQ(tmpnum1, num1);
        EXPECT_EQ(tmpnum2, num2);
        EXPECT_EQ(tmpnum3, num3);
        EXPECT_EQ(tmpnum4, num4);
        EXPECT_EQ(tmpnum5, num5);

        EXPECT_EQ(tmpstr1, str1);
        EXPECT_EQ(tmpstr2, str2);
        EXPECT_EQ(tmpstr3, str3);
        EXPECT_EQ(tmpstr4, str4);
        EXPECT_EQ(tmpstr5, str5);
    }
    std::cout << "only xtream  serialize:" << sum / 1000.0 << std::endl;
}

TEST_F(test_tx_v3, v3_serialize) {
    uint64_t sum = 0;
    for (int i = 0; i < 1000; i++)
    {
        uint64_t num1 = rand();
        uint64_t num2 = rand();
        uint64_t num3 = rand();
        uint64_t num4 = rand();
        uint64_t num5 = rand();

        std::string str1;
        std::string str2;
        std::string str3;
        std::string str4;
        std::string str5;
        top::xbyte_buffer_t randstr = random_bytes(32);
        str1.append((char*)randstr.data(), randstr.size());
        randstr = random_bytes(32);
        str2.append((char*)randstr.data(), randstr.size());
        randstr = random_bytes(32);
        str3.append((char*)randstr.data(), randstr.size());
        randstr = random_bytes(32);
        str4.append((char*)randstr.data(), randstr.size());
        randstr = random_bytes(32);
        str5.append((char*)randstr.data(), randstr.size());

        top::evm_common::bytes encoded;
        top::evm_common::append(encoded, top::evm_common::RLP::encode(num1));
        top::evm_common::append(encoded, top::evm_common::RLP::encode(num2));
        top::evm_common::append(encoded, top::evm_common::RLP::encode(num3));
        top::evm_common::append(encoded, top::evm_common::RLP::encode(num4));
        top::evm_common::append(encoded, top::evm_common::RLP::encode(num5));
        top::evm_common::append(encoded, top::evm_common::RLP::encode(str1));
        top::evm_common::append(encoded, top::evm_common::RLP::encode(str2));
        top::evm_common::append(encoded, top::evm_common::RLP::encode(str3));
        top::evm_common::append(encoded, top::evm_common::RLP::encode(str4));
        top::evm_common::append(encoded, top::evm_common::RLP::encode(str5));
        encoded = top::evm_common::RLP::encodeList(encoded);

        uint8_t nEipVersion = 1;
        top::base::xstream_t stream(top::base::xcontext_t::instance());
        stream.write_compact_var(nEipVersion);
        stream.write_compact_var(std::string((char*)encoded.data(), encoded.size()));

        sum += stream.size();
        std::string strTop;
        strTop.append((char *)stream.data(), stream.size());

        std::string strEth;
        stream.read_compact_var(nEipVersion);
        stream.read_compact_var(strEth);

        EXPECT_EQ(nEipVersion, 1);

        top::evm_common::RLP::DecodedItem decoded = top::evm_common::RLP::decode(top::evm_common::data(strEth));
        std::vector<std::string> vecData;
        for (int i = 0; i < (int)decoded.decoded.size(); i++) {
            std::string str(decoded.decoded[i].begin(), decoded.decoded[i].end());
            vecData.push_back(str);
        }

        EXPECT_EQ(top::evm_common::to_uint64(vecData[0]), num1);
        EXPECT_EQ(top::evm_common::to_uint64(vecData[1]), num2);
        EXPECT_EQ(top::evm_common::to_uint64(vecData[2]), num3);
        EXPECT_EQ(top::evm_common::to_uint64(vecData[3]), num4);
        EXPECT_EQ(top::evm_common::to_uint64(vecData[4]), num5);

        EXPECT_EQ(vecData[5], str1);
        EXPECT_EQ(vecData[6], str2);
        EXPECT_EQ(vecData[7], str3);
        EXPECT_EQ(vecData[8], str4);
        EXPECT_EQ(vecData[9], str5);
    }
    std::cout << "v3 serialize:" << sum / 1000.0 << std::endl;
}


TEST_F(test_tx_v3, v3_performance) {
    for (int i = 0; i < 1000; i++)
    {
        //generate signed transaction
        xobject_ptr_t<top::data::eip_1559_tx> tx = make_object_ptr<top::data::eip_1559_tx>();
        tx->accesslist = "";
        tx->chainid = 1023;
        tx->data = "";
        tx->gas = rand() % 100000;
        tx->nonce = rand();
        top::xbyte_buffer_t randstr = random_bytes(20);
        tx->to.append((char*)randstr.data(), randstr.size());
        tx->value = rand() * 1000000000;
        tx->max_fee_per_gas = rand() % 100000;
        tx->max_priority_fee_per_gas = rand() % 100000;

        top::evm_common::bytes encodedtmp;
        top::evm_common::append(encodedtmp, top::evm_common::RLP::encode(tx->chainid));
        top::evm_common::append(encodedtmp, top::evm_common::RLP::encode(tx->nonce));
        top::evm_common::append(encodedtmp, top::evm_common::RLP::encode(tx->max_priority_fee_per_gas));
        top::evm_common::append(encodedtmp, top::evm_common::RLP::encode(tx->max_fee_per_gas));
        top::evm_common::append(encodedtmp, top::evm_common::RLP::encode(tx->gas));
        top::evm_common::append(encodedtmp, top::evm_common::RLP::encode(tx->to));
        top::evm_common::append(encodedtmp, top::evm_common::RLP::encode(tx->value));
        top::evm_common::append(encodedtmp, top::evm_common::RLP::encode(tx->data));
        top::evm_common::append(encodedtmp, top::evm_common::fromHex("c0"));

        top::evm_common::bytes encoded;
        top::evm_common::append(encoded, static_cast<uint8_t>(2));
        top::evm_common::append(encoded, top::evm_common::RLP::encodeList(encodedtmp));

        std::string strEncoded;
        strEncoded.append((char*)encoded.data(), encoded.size());
        top::uint256_t hash = top::utl::xkeccak256_t::digest(strEncoded);
        uint8_t prikey[32];

        top::evm_common::bytes vprikey = top::evm_common::fromHex("3963ed01bce983f8829174b13d3417716ff604bfb0fed07634288df8b146f20f");

        memcpy(prikey, vprikey.data(), vprikey.size());

        top::utl::xecprikey_t key(prikey);
        top::utl::xecdsasig_t sig = key.sign(hash);
        tx->signV = sig.get_recover_id();
        top::evm_common::bytes r;
        std::string strSignature;
        strSignature.append((char*)sig.get_raw_signature(), 64);
        std::string strR = strSignature.substr(0, 32); 
        std::string strS = strSignature.substr(32);
        r.insert(r.begin(), strR.begin(), strR.end());
        tx->signR = top::evm_common::fromBigEndian<top::evm_common::u256>(r);
        top::evm_common::bytes s;
        s.insert(s.begin(), strS.begin(), strS.end());
        tx->signS = top::evm_common::fromBigEndian<top::evm_common::u256>(s);


        encodedtmp.clear();
        top::evm_common::append(encodedtmp, top::evm_common::RLP::encode(tx->chainid));
        top::evm_common::append(encodedtmp, top::evm_common::RLP::encode(tx->nonce));
        top::evm_common::append(encodedtmp, top::evm_common::RLP::encode(tx->max_priority_fee_per_gas));
        top::evm_common::append(encodedtmp, top::evm_common::RLP::encode(tx->max_fee_per_gas));
        top::evm_common::append(encodedtmp, top::evm_common::RLP::encode(tx->gas));
        top::evm_common::append(encodedtmp, top::evm_common::RLP::encode(tx->to));
        top::evm_common::append(encodedtmp, top::evm_common::RLP::encode(tx->value));
        top::evm_common::append(encodedtmp, top::evm_common::RLP::encode(tx->data));
        top::evm_common::append(encodedtmp, top::evm_common::fromHex("c0"));
        top::evm_common::append(encodedtmp, top::evm_common::RLP::encode(tx->signV));
        top::evm_common::append(encodedtmp, top::evm_common::RLP::encode(strR));
        top::evm_common::append(encodedtmp, top::evm_common::RLP::encode(strS));

        encoded.clear();
        top::evm_common::append(encoded, static_cast<uint8_t>(2));
        top::evm_common::append(encoded, top::evm_common::RLP::encodeList(encodedtmp));

        xtransaction_v3_ptr_t tx_r = make_object_ptr<xtransaction_v3_t>();
        std::string strParams;
        strParams.append("0x");
        strParams.append(top::evm_common::toHex(std::string((char*)encoded.data(), encoded.size())));
        xJson::Value jv;
        jv["id"] = "12231";
        jv["jsonrpc"] = "2.0";
        jv["method"] = "eth_sendRawTransaction";
        jv["params"][0] = strParams;
        tx_r->construct_from_json(jv);

        EXPECT_EQ(tx_r->get_source_addr(), "T600045b576c4064306de62bf628e0a764ca0b37b3594a");
    }
}

TEST_F(test_tx_v3, v3_performance_only) {
    for (int i = 0; i < 1000; i++)
    {
        xtransaction_v3_ptr_t tx = make_object_ptr<xtransaction_v3_t>();

        xJson::Value jv;
        jv["id"] = "12231";
        jv["jsonrpc"] = "2.0";
        jv["method"] = "eth_sendRawTransaction";
        jv["params"][0] = "0x02f8708203ff80822710822710827b0c94b7762d8dbd7e5c023ff99402b78af7c13b01eec1881bc16d674ec8000080c001a0d336694faa98f9cd69792ee30f9979f906f997d7562a0cb86d109f3476643a65a0679a7f510b12d48f9d1637884245229b7037b4f4094d1fb30e0f4f5cc77388ec";
        tx->construct_from_json(jv);
        base::xstream_t stream(base::xcontext_t::instance());
        tx->serialize_to(stream);
        xtransaction_ptr_t tx_r = make_object_ptr<xtransaction_v3_t>();
        tx_r->serialize_from(stream);
        EXPECT_EQ(tx_r->get_source_addr(), tx->get_source_addr());
    }
}

TEST_F(test_tx_v3, v2_performance) {
    for (int i = 0; i < 1000; i++)
    {
        xtransaction_ptr_t tx = make_object_ptr<xtransaction_v2_t>();
        data::xproperty_asset asset_out{0};
        tx->make_tx_transfer(asset_out);
        std::string source_addr = "T8000037d4fbc08bf4513a68a287ed218b0adbd497ef30";
        std::string target_addr = "T80000077ae60e9d17e4f59fd614a09eae3d1312b2041a";
        tx->set_different_source_target_address(source_addr, target_addr);
        tx->set_fire_and_expire_time(600);
        const uint32_t min_tx_deposit = 100000;
        tx->set_deposit(min_tx_deposit);

        tx->set_last_nonce(0);
        std::string last_trans_hash = "0xce27bac30e9d5dd1";
        tx->set_last_hash(data::hex_to_uint64(last_trans_hash));
        tx->set_digest();
        std::string authorization = "0105c2ba9cd7d9a9b6c27b1c503ffc045846698cdff1492e4b";
        tx->set_authorization(authorization);
        auto tx_hash = tx->get_digest_hex_str();

        base::xstream_t stream(base::xcontext_t::instance());
        tx->serialize_to(stream);

        xtransaction_ptr_t tx_r = make_object_ptr<xtransaction_v2_t>();
        tx_r->serialize_from(stream);
        EXPECT_EQ(tx_r->get_source_addr(), tx->get_source_addr());
    }
}

TEST_F(test_tx_v3, serialize_compare) {
    uint64_t num1 = rand();
    uint64_t num2 = rand();
    uint64_t num3 = rand();
    uint64_t num4 = rand();
    uint64_t num5 = rand();

    std::string str1;
    std::string str2;
    std::string str3;
    std::string str4;
    std::string str5;
    top::xbyte_buffer_t randstr = random_bytes(32);
    str1.append((char*)randstr.data(), randstr.size());
    randstr = random_bytes(32);
    str2.append((char*)randstr.data(), randstr.size());
    randstr = random_bytes(32);
    str3.append((char*)randstr.data(), randstr.size());
    randstr = random_bytes(32);
    str4.append((char*)randstr.data(), randstr.size());
    randstr = random_bytes(32);
    str5.append((char*)randstr.data(), randstr.size());

    top::evm_common::bytes encoded;
    top::evm_common::append(encoded, top::evm_common::RLP::encode(num1));
    top::evm_common::append(encoded, top::evm_common::RLP::encode(num2));
    top::evm_common::append(encoded, top::evm_common::RLP::encode(num3));
    top::evm_common::append(encoded, top::evm_common::RLP::encode(num4));
    top::evm_common::append(encoded, top::evm_common::RLP::encode(num5));
    top::evm_common::append(encoded, top::evm_common::RLP::encode(str1));
    top::evm_common::append(encoded, top::evm_common::RLP::encode(str2));
    top::evm_common::append(encoded, top::evm_common::RLP::encode(str3));
    top::evm_common::append(encoded, top::evm_common::RLP::encode(str4));
    top::evm_common::append(encoded, top::evm_common::RLP::encode(str5));
    encoded = top::evm_common::RLP::encodeList(encoded);

    std::cout << "only rlp serialize:" << encoded.size() << std::endl;

    top::base::xstream_t stream(top::base::xcontext_t::instance());
    stream.write_compact_var(std::string((char*)encoded.data(), encoded.size()));

    std::cout << "v3 serialize:" << stream.size() << std::endl;

    std::string strEth;
    stream.read_compact_var(strEth);

    top::evm_common::RLP::DecodedItem decoded = top::evm_common::RLP::decode(encoded);

    std::vector<std::string> vecData;
    for (int i = 0; i < (int)decoded.decoded.size(); i++) {
        std::string str(decoded.decoded[i].begin(), decoded.decoded[i].end());
        vecData.push_back(str);
    }

    EXPECT_EQ(top::evm_common::to_uint64(vecData[0]), num1);
    EXPECT_EQ(top::evm_common::to_uint64(vecData[1]), num2);
    EXPECT_EQ(top::evm_common::to_uint64(vecData[2]), num3);
    EXPECT_EQ(top::evm_common::to_uint64(vecData[3]), num4);
    EXPECT_EQ(top::evm_common::to_uint64(vecData[4]), num5);

    EXPECT_EQ(vecData[5], str1);
    EXPECT_EQ(vecData[6], str2);
    EXPECT_EQ(vecData[7], str3);
    EXPECT_EQ(vecData[8], str4);
    EXPECT_EQ(vecData[9], str5);


    stream.reset();
    stream.write_compact_var(num1);
    stream.write_compact_var(num2);
    stream.write_compact_var(num3);
    stream.write_compact_var(num4);
    stream.write_compact_var(num5);
    stream.write_compact_var(str1);
    stream.write_compact_var(str2);
    stream.write_compact_var(str3);
    stream.write_compact_var(str4);
    stream.write_compact_var(str5);

    std::cout << "only xtream  serialize:" << stream.size() << std::endl;

    uint64_t tmpnum1;
    uint64_t tmpnum2;
    uint64_t tmpnum3;
    uint64_t tmpnum4;
    uint64_t tmpnum5;

    std::string tmpstr1;
    std::string tmpstr2;
    std::string tmpstr3;
    std::string tmpstr4;
    std::string tmpstr5;

    stream.read_compact_var(tmpnum1);
    stream.read_compact_var(tmpnum2);
    stream.read_compact_var(tmpnum3);
    stream.read_compact_var(tmpnum4);
    stream.read_compact_var(tmpnum5);

    stream.read_compact_var(tmpstr1);
    stream.read_compact_var(tmpstr2);
    stream.read_compact_var(tmpstr3);
    stream.read_compact_var(tmpstr4);
    stream.read_compact_var(tmpstr5);


    EXPECT_EQ(tmpnum1, num1);
    EXPECT_EQ(tmpnum2, num2);
    EXPECT_EQ(tmpnum3, num3);
    EXPECT_EQ(tmpnum4, num4);
    EXPECT_EQ(tmpnum5, num5);

    EXPECT_EQ(tmpstr1, str1);
    EXPECT_EQ(tmpstr2, str2);
    EXPECT_EQ(tmpstr3, str3);
    EXPECT_EQ(tmpstr4, str4);
    EXPECT_EQ(tmpstr5, str5);
}
