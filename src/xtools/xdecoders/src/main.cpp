// Copyright (c) 2022-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "../xvote_data_decoder.h"
#include "../xtoken_to_ticket_swap_contract_data_decoder.h"
#include "../xticket_to_token_swap_contract_data_decoder.h"

#include <CLI11.hpp>

NS_BEG3(top, tools, decoders)

static int main_impl(int const argc, char * argv[]) {
    CLI::App data_decoder{"top binary data decoder"};

    auto const vote_data_decoder_cmd = data_decoder.add_subcommand("vote_data", "decode vote contract data");

    vote_data_decoder_cmd->add_option_function<std::string>(
        "--decode_vote_contract_data", [](std::string const & input) { return xvote_data_decoder_t::decode(input); }, "decode vote data from vote contract");

    vote_data_decoder_cmd->add_option_function<std::string>(
        "--decode_token_to_ticket_swap_contract_data",
        [](std::string const & input) { return xtoken_to_ticket_swap_contract_data_decoder_t::decode(input); },
        "decode vote data from token to ticket swap contract");

    vote_data_decoder_cmd->add_option_function<std::string>(
        "--decode_ticket_to_token_swap_contract_data",
        [](std::string const & input) { return xticket_to_token_swap_contract_data_decoder_t::decode(input); },
        "decode vote date from ticket to vote swap contract");

    data_decoder.parse(argc, argv);
    return 0;
}

NS_END3

int main(int const argc, char * argv[]) {
    return top::tools::decoders::main_impl(argc, argv);
}
