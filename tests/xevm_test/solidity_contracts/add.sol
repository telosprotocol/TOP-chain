pragma solidity 0.6.4;

contract xevm_test {
    uint64 public global;
    event catchAdd(uint64 a, uint64 b, uint64 c);
    event catchAddOne(uint64 a, uint64 ret);
    event catchGlobal(uint64 c);

    function add(uint64 a, uint64 b) public {
        uint64 c = a + b;
        emit catchAdd(a, b, c);
    }

    function addOne(uint64 a) public {
        uint64 b = a + 1;
        emit catchAddOne(a, b);
    }

    function addGlobal() public {
        global = global + 1;
        emit catchGlobal(global);
    }
}
