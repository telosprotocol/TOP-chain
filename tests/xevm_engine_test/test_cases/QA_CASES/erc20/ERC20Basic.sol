pragma solidity >=0.4.22 <0.9.0;

abstract contract  ERC20Basic {
    // Total supply of token.
    function totalSupply() external virtual view returns (uint256);
    // Balance of a holder _who
    function balanceOf(address _who) external virtual view returns (uint256);
    // Transfer _value from msg.sender to receiver _to.
    function transfer(address _to, uint256 _value) external virtual returns (bool);
    // Fired when a transfer is made
    event Transfer(
        address indexed from,
        address indexed to,
        uint256 value
    );
}