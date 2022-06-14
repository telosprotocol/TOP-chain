pragma solidity 0.6.4;

contract test_balance {
    uint256 public totalBalance;
    event Deposit(address, uint256);
    event Withdraw(bool, address, uint256);

    receive() external payable {
        totalBalance += msg.value;
        emit Deposit(msg.sender, msg.value);
    }

    function withdraw_balance(uint256 amount) public {
        totalBalance -= amount;
        bool res;
        (res, ) = address(msg.sender).call{gas: 100000, value: amount}("");
        emit Withdraw(res, msg.sender, amount);
    }
}
