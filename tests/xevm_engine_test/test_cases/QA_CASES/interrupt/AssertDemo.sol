pragma solidity =0.8.13;

contract AssertDemo {
    string name;

    constructor() public {
        name = "welight";
    }

    function get() public view returns (string memory) {
        return name;
    }

    event Set(address indexed_from, string n);

    function set(string memory n) public {
        assert(false);
        name = n;
        emit Set(msg.sender, n);
    }
}
