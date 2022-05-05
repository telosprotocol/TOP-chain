pragma solidity >=0.4.22 <0.9.0;

import "./ERC20Basic.sol";
/**
 * @title ERC20 interface
 * @dev Enhanced interface with allowance functions.
 * See https://github.com/ethereum/EIPs/issues/20
 */
abstract contract ERC20 is ERC20Basic {
    // Check the allowed value that the _owner allows the _spender to take from his balance.
    function allowance(address _owner, address _spender) external virtual view returns (uint256);

    // Transfer _value from the balance of holder _from to the receiver _to.
    function transferFrom(address _from, address _to, uint256 _value) external virtual returns (bool);

    // Approve _spender to take some _value from the balance of msg.sender.
    function approve(address _spender, uint256 _value) external virtual returns (bool);

    // Fired when an approval is made.
    event Approval(
        address indexed owner,
        address indexed spender,
        uint256 value
    );
}