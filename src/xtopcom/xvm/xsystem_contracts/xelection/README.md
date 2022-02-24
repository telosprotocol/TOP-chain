# Overview
This directory(`xsystem_contracts/xelection`) contains system smart contracts related to the election business. The `/xrec` and `/xzec` subdirectories are grouped according to where the contract is deployed.

## REC Smart Contracts
REC group contains following smart contracts:
* REC standby pool smart contract
* REC elects REC smart contract
* REC elects ZEC smart contract
* REC elects EDGE smart contract
* REC elects ARCHIVE smart contract

## ZEC Smart Contracts
ZEC group contains followint smart contract:

* ZEC standby pool smart contract
* ZEC elects AUDITOR/VALIDATOR group smart contract
* ZEC group association smart contract


# Smart Contracts in some Depth

## REC standby pool
Standby pool contains information about all nodes that can be elected into working group. It periodically updates its registration and staking data from registration related smart contract.

#### nodeJoinNetwork
When node (already registred) joins the network and triggers the `nodeJoinNetwork` action. This action as well as the node information will be formed into a transaction. The transaction will be finnally delivered to REC group. And the data included in the transaction will be added into REC standby pool smart contract's property.

#### on_timer
Triggered by logic timer periodically at the interval specified by `rec_standby_pool_update_interval`. Update REC standby pool contract's property if nodes info changed(stake, credit) or de-registred.


## ZEC standby pool
ZEC standby pool is similar to REC standby pool. It just synchonizes (reads) data from REC standby pool. Since reading data cross chain in a distributed system is hard to reach consistance, ZEC standby pool smart contract records a data indicating which historical data it has been read last time.

#### on_timer
Triggered by clock height at regular intervals(`zec_standby_pool_update_interval`). Update property `XPROPERTY_LAST_READ_REC_STANDBY_POOL_CONTRACT_BLOCK_HEIGHT` and `XPROPERTY_LAST_READ_REC_STANDBY_POOL_CONTRACT_LOGIC_TIME`, which represent the *relatively reliable rec_standby_pool height* and *last updated logic time height*.

*relatively reliable rec_standby_pool height* : the rec_standby_pool data referenced by ZEC when electing CONSENSUS nodes.
*last updated logic time height* : the time clock when *relatively reliable rec_standby_pool height* update.


## REC elect edge
Nonconsensus election. REC nodes elect as many EDGE nodes from REC standby pool as possible.

#### on_timer
Triggered by clock height at regular intervals(`edge_election_interval`). Elect in nodes which are in standby_pool.edge and elect out which are not.


## REC elect archive
Nonconsensus election. REC nodes elect as many ARCHIVE nodes from REC standby pool as possible.

#### on_timer
Triggered by clock height at regular intervals(`archive_election_interval`). Elect in nodes which are in standby_pool.archive and elect out which are not.


## REC elect REC
Consensus election. REC nodes elect next round REC nodes from REC standby pool.

#### on_timer
Triggered by clock height at regular intervals(`rec_election_interval`). Every round of election will replace a certain percentage of the current nodes. The higher the rec_stake the node is, the higher the possibility of being elect in & lower being elected out.


## REC elect ZEC
Consensus election. REC nodes elect next round ZEC nodes from REC standby pool. Every round of election will replace a certain percentage of the current nodes. The higher the zec_stake the node is, the higher the possibility of being elect in & lower being elected out.

#### on_timer
Triggered by clock height at regular intervals(`zec_election_interval`).


## ZEC elect consensus
Consensus election. ZEC nodes elect next round CONSENSUS nodes(including AUDITOR and VALIDATOR nodes) from ZEC standby pool. Every round of election will replace a certain percentage of the current nodes. The higher the zec_stake the node is, the higher the possibility of being elect in & lower being elected out.

It should be noted that a node can't work in two relative cluster at the same time. Therefore the standby nodes will be filtered by some certain rules according to the current election result.

rules:
- A node can't work in two auditor groups at the same time.
- A node can't work in two validator groups at the same time.
- A node can't work in an auditor group and it's associated validator groups at the same time.

## ZEC group association
Maintain correspondence to the consensus cluster. Initialize the consensus cluster correspondence according to the parameters on the chain.
Dynamic capacity expansion is not supported for now.


# chain_governance
The above `interval`s ,`cluster group size`s as well as some `rotation_ratio`s are governanced in TCC contract.
