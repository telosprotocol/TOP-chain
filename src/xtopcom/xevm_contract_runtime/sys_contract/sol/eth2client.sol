// SPDX-License-Identifier: GPL-3.0
pragma solidity ^0.8.0;

enum ClientMode {
    Invalid,
    SubmitLightClientUpdate,
    SubmitHeader
}

interface eth2client {
    function init(bytes calldata genesis) external returns (bool success);

    function initialized() external view returns (bool inited);

    function block_hash_safe(
        uint64 height
    ) external view returns (bytes32 hash);

    function finalized_beacon_block_header()
        external
        view
        returns (bytes memory header);

    function finalized_beacon_block_root() external view returns (bytes32 root);

    function finalized_beacon_block_slot() external view returns (uint64 slot);

    function get_light_client_state()
        external
        view
        returns (bytes memory state);

    function is_confirmed(
        uint256 height,
        bytes32 data
    ) external view returns (bool known);

    function is_known_execution_header(
        uint64 height
    ) external view returns (bool known);

    function last_block_number() external view returns (uint64 number);

    function submit_beacon_chain_light_client_update(
        bytes calldata update
    ) external returns (bool success);

    function submit_execution_headers(
        bytes calldata headers
    ) external returns (bool success);

    function reset() external returns (bool success);

    function disable_reset() external returns (bool success);

    function get_client_mode() external view returns (ClientMode mode);

    function get_unfinalized_tail_block_number()
        external
        view
        returns (uint64 number);
}
