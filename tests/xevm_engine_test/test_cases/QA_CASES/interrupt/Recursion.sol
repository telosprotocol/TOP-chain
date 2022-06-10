pragma solidity ^0.8.10;

contract Recursion {    

    function sum2(uint256 n) public pure returns (uint256){
        if(n == 1){
            return n;
        }else{
             return n * sum2(n+1);   
        }
    }
}