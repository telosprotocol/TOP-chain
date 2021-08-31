// #include "tests/xcontract_runtime/xuser_contract_runtime_fixture.h"

// #include "xbase/xmem.h"
// #include "xbase/xcontext.h"
// #include "xdata/xtransaction.h"

// NS_BEG3(top, tests, contract_runtime)

// TEST_F(contract_runtime_fixture, run_contract_normal) {
//     auto codevar = bstate_->new_code_var("#src_code", nullptr);
//     codevar->deploy_code(R"T(
// function init()
//     create_map('balances')
//     -- 创建map嵌套map的属性，这个代码感觉不管怎么写，都比较变扭。要么就是提供一个LUA的table类型数据序列化到字符串，然后将字符串存储为属性的API（和一个反向操作API）
//     -- create_map('allowed')
// end

// function name()
//     return get_string('name')
// end

// function symbol()
//     return get_string('symbol')
// end

// function decimals()
//     return get_integer('decimals')
// end

// function totalSupply()
//     return get_integer('totalSupply')
// end

// function balanceOf(address)
//     return map_prop_query('balances', address)
// end

// function transfer(to_address, amount)
//     sender_balance = get_value('balances', sender_address())
//     to_balance = get_value('balances', to_address)

//     set_value('balances', sender_address(), sender_balance - amount)
//     set_value('balances', to_address, to_balance + amount)
//     return true
// end

// function transferFrom(from_address, to_address, amount)
//     sender_balance = get_value('balances', from_address)
//     to_balance = get_value('balances', to_address)

//     set_value('balances', from_address, sender_balance - amount)
//     set_value('balances', to_address, to_balance + amount)
//     return true
// end

// function approve(spender_address, amount)
// end

// function allowance(owner_address, spender_address)
// end

// )T");
//     auto symbol = bstate_->new_string_var("symbol");
//     symbol->reset("TOP");
//     auto name = bstate_->new_string_var("name");
//     name->reset("TOP Network");
//     auto totalSupply = bstate_->new_uint64_var("totalSupply");
//     totalSupply->set(20000000000000000);
//     auto decimals = bstate_->new_uint64_var("decimals");
//     decimals->set(18);
//     auto balances = bstate_->new_uint64_map_var("balances");
//     balances->insert(bstate_->get_account_addr(), totalSupply->get());

//     auto tx = top::make_object_ptr<top::data::xtransaction_t>();
//     top::base::xstream_t param_stream(base::xcontext_t::instance());
//     param_stream << static_cast<uint8_t>(1);
//     param_stream << static_cast<uint8_t>(2);
//     param_stream << bstate_->get_account_addr();
//     std::string param(reinterpret_cast<char *>(param_stream.data()), param_stream.size());
//     tx->make_tx_run_contract("balanceOf", param);
//     // tx->make_tx_create_sub_account

//     contract_ctx_ = std::make_shared<top::contract_common::xcontract_execution_context_t>(tx, contract_state_);
//     contract_runtime_->execute_transaction(top::make_observer(contract_ctx_.get()));
// }

// NS_END3
