// SPDX-License-Identifier: MIT
pragma solidity ^0.8.10;

import "./Foo.sol";

import {Unauthorized, add as func, Point} from "./Foo.sol";

contract Import {
    Foo public foo = new Foo();

    function getFooName() public view returns (string memory) {
        return foo.name();
    }

    function add(uint256 x,uint256 y) public  returns (uint256) {
        return add(x,y);
    }
}
