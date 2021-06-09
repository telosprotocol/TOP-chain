#pragma once

#include "xelect_net/include/multilayer_network_interface.h"

namespace top {

namespace elect {


class MultilayerNetworkChainQuery : public MultilayerNetworkInterface {
public:
    MultilayerNetworkChainQuery(MultilayerNetworkChainQuery const &)             = delete;
    MultilayerNetworkChainQuery & operator=(MultilayerNetworkChainQuery const &) = delete;
    MultilayerNetworkChainQuery(MultilayerNetworkChainQuery &&)                  = delete;
    MultilayerNetworkChainQuery & operator=(MultilayerNetworkChainQuery &&)      = delete;
    virtual ~MultilayerNetworkChainQuery()                                      = default;


    MultilayerNetworkChainQuery()                                               = default;;


public:
    /**
     * @brief Get the result of whether the node has joined the network
     *
     * @return true joined the network successfully
     * @return false not joined the network
     */
    bool Joined() override;
    /**
     * @brief get root routing table peers info
     *
     * @return std::string all peers info
     */
    std::string Peers() override;
    /**
     * @brief id and ip:port, such as tnode://b4062cb0463fb6cf6576337cebff8414387ed445739e458d23bb037f2aadc6c41205147f92@127.0.0.1:30303
     *
     * @return std::string p2p addresss
     */
    std::string P2pAddr() override;
#ifdef DEBUG
    /**
     * @brief broadcast
     *
     * @param msg_size msg size
     * @param count count
     * @return uint32_t
     */
    uint32_t Broadcast(uint32_t msg_size, uint32_t count) override;
#endif
#if 0
    /**
     * @brief Get the count of max connect peers
     * 
     * @return uint32_t count of mac connect peers
     */
    uint32_t     MaxPeers()         override;
    /**
     * @brief Get the count of connecting peers
     * 
     * @return uint32_t count of connecting peers
     */
    uint32_t     PeerCount()        override;
    /**
     * @brief  Get xnetwork ID which the node joined in
     * 
     * @return uint32_t network id
     */
    uint32_t     ChainId()          override;
    /**
     * @brief Print OS information
     * 
     * @return std::string os info
     */
    std::string  OsInfo()           override;
    /**
     * @brief Print elect network info
     * 
     * @return std::string elect network info
     */
    std::string  NetInfo()          override;
    /**
     * @brief global kademlia id
     * 
     * @return std::string global kademlia id
     */
    std::string  Gid()              override;
    /**
     * @brief account of this node
     * 
     * @return std::string account of this node
     */
    std::string  Account()          override;
    /**
     * @brief get all peers info
     * 
     * @return std::string all peers info
     */
    std::string  AllPeers()         override;
    /**
     * @brief get all nodes info
     * 
     * @return std::string all nodes info
     */
    std::string  AllNodes()         override;
    /**
     * @brief print usage or help info
     * 
     * @return std::string help info
     */
    std::string HelpInfo() override;
#endif
};

typedef std::shared_ptr<MultilayerNetworkChainQuery> MultilayerNetworkChainQueryPtr;

} // end namespace elect

} // end namespace top
