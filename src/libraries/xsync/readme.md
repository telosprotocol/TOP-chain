Optimizations:
1, xchain_collector cache queried chain info from store, need cache all chains, since for exist chains, we need addresses to query latest info from store
2, to support VNode, need add a map : XIP->ChainCache (std::vector is used, but can switch to map since xvnode_address_t support < compare)
3, dynamic_cast xrequest_t can be replaced by type id
4, enumeration priority queue, can add inner hash map to accurate search speed. can refer to #xsync_executor/readme.md
5, note that any blocks requests should sort by block height from high to low
6, consider multi-blocks per request from remote vnode