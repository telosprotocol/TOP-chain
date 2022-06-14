pragma solidity >=0.4.22 <0.9.0;

import "./ERC20.sol";
import "./SafeMath.sol";

/**
 * @title CAT Token
 * @dev Compatible with ERC20/VIP180 Standard.
 * Special thanks go to openzeppelin-solidity project.
 */
contract Cat is ERC20 {
    using SafeMath for uint256;

    // Name of token
    string public constant name = "CAT Token";
    // Symbol of token
    string public constant symbol = "CAT";
    // Decimals of token
    uint8 public constant decimals = 18;
    // Total supply of the tokens
    uint256 internal totalSupply_;

    // balances: (_holder => _value)
    mapping(address => uint256) public balances;


    // allowed: (_owner, => (_spender, _value))
    mapping (address => mapping (address => uint256)) internal allowed;

    event Transfer(
        uint zero,
        address address_,
        uint256 balance_
        );
        

    constructor()  {
    totalSupply_ = 1 * (10 ** 10) * (10 ** 18); // 10 000 000 000 tokens of 18 decimals.
    balances[msg.sender] = totalSupply_;
    emit Transfer(0, msg.sender, totalSupply_);  // init mint of coins complete.
    }

    // Get the total supply of the coins
    function totalSupply() public override  view returns (uint256) {
    return totalSupply_;
    }

    // Get the balance of _owner
    function balanceOf(address _owner) public override  view returns (uint256 balance) {
    return balances[_owner];
    }

    /** Make a Transfer.
    * @dev This operation will deduct the msg.sender's balance.
    * @param _to address The address the funds go to.
    * @param _value uint256 The amount of funds.
    */
    function transfer(address _to, uint256 _value) public override  returns (bool) {
    require(_to != address(0), "Cannot send to all zero address.");
    require(_value <= balances[msg.sender], "msg.sender balance is not enough.");

    // SafeMath.sub will throw if there is not enough balance.
    balances[msg.sender] = balances[msg.sender].sub(_value);
    balances[_to] = balances[_to].add(_value);
    emit Transfer(msg.sender, _to, _value);
    return true;
    }

    /**
    * @dev Check the allowed funds that _spender can take from _owner.
    * @param _owner address The address which owns the funds.
    * @param _spender address The address which will spend the funds.
    * @return A uint256 specifying the amount of tokens still available for the spender.
    */
    function allowance(address _owner, address _spender) public override  view returns (uint256) {
        return allowed[_owner][_spender];
    }

    /**
    * @dev Transfer tokens from one address to another
    * @param _from address The address which you want to send tokens from
    * @param _to address The address which you want to transfer to
    * @param _value uint256 the amount of tokens to be transferred
    */
    function transferFrom(address _from, address _to, uint256 _value) public override  returns (bool) {
        require(_value <= balances[_from], "_from doesnt have enough balance.");
        require(_value <= allowed[_from][msg.sender], "Allowance of msg.sender is not enough.");
        require(_to != address(0), "Cannot send to all zero address.");

        balances[_from] = balances[_from].sub(_value);
        balances[_to] = balances[_to].add(_value);
        allowed[_from][msg.sender] = allowed[_from][msg.sender].sub(_value);
        emit Transfer(_from, _to, _value);
        return true;
    }

    /**
    * @dev Approve the passed address to spend the specified amount of tokens on behalf of msg.sender.
    * @param _spender The address which will spend the funds.
    * @param _value The amount of tokens to be spent.
    */
    function approve(address _spender, uint256 _value) public override  returns (bool) {
        allowed[msg.sender][_spender] = _value;
        emit Approval(msg.sender, _spender, _value);
        return true;
        }
}