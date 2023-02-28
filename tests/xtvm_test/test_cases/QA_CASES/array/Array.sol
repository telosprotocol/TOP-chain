pragma solidity ^0.8.10;

contract Array {
    
    function a(uint256 x,uint256 y) public returns(uint256){
        uint[] memory a = new uint[](3);
        a[0] = 1;
        a[1] = 2;
        a[2] = 3;
        return a[x] + a[y];
    }
}
