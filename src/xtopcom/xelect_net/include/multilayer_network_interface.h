#pragma once

#include <cstdint>

#include <memory>

namespace top {

namespace elect {


class MultilayerNetworkInterface {
public:
    MultilayerNetworkInterface(MultilayerNetworkInterface const &)             = delete;
    MultilayerNetworkInterface & operator=(MultilayerNetworkInterface const &) = delete;
    MultilayerNetworkInterface(MultilayerNetworkInterface &&)                  = delete;
    MultilayerNetworkInterface & operator=(MultilayerNetworkInterface &&)      = delete;
    virtual ~MultilayerNetworkInterface()                                      = default;


    MultilayerNetworkInterface()                                               = default;


public:
    /**
     * @brief Get the result of whether the node has joined the network
     * 
     * @return true joined the network successfully
     * @return false not joined the network
     */
    virtual bool     Joined()        = 0;
    /**
     * @brief Get the count of max connect peers
     * 
     * @return uint32_t count of mac connect peers
     */
    virtual uint32_t MaxPeers()      = 0;
    /**
     * @brief Get the count of connecting peers
     * 
     * @return uint32_t count of connecting peers
     */
    virtual uint32_t PeerCount()     = 0;
    /**
     * @brief  Get xnetwork ID which the node joined in
     * 
     * @return uint32_t network id
     */
    virtual uint32_t ChainId()       = 0;
    /**
     * @brief Print OS information
     * 
     * @return std::string os info
     */
    virtual std::string OsInfo()     = 0;
    /**
     * @brief Print elect network info
     * 
     * @return std::string elect network info
     */
    virtual std::string NetInfo()    = 0;
    /**
     * @brief get all peers info
     * 
     * @return std::string all peers info
     */
    virtual std::string AllPeers()   = 0;
    /**
     * @brief get all peers info of root routing table
     * 
     * @return std::string all peers info
     */
    virtual std::string Peers()   = 0;
    /**
     * @brief get all nodes info
     * 
     * @return std::string all nodes info
     */
    virtual std::string AllNodes()   = 0;
    /**
     * @brief broadcast
     * 
     * @param msg_size msg size
     * @param count count
     * @return uint32_t 
     */
    virtual uint32_t Broadcast(
            uint32_t msg_size,
            uint32_t count)          = 0;
    /**
     * @brief global kademlia id
     * 
     * @return std::string global kademlia id
     */
    virtual std::string Gid()        = 0; 
    /**
     * @brief account of this node
     * 
     * @return std::string account of this node
     */
    virtual std::string Account()    = 0; 
    /**
     * @brief id and ip:port, such as tnode://b4062cb0463fb6cf6576337cebff8414387ed445739e458d23bb037f2aadc6c41205147f92@127.0.0.1:30303
     * 
     * @return std::string p2p addresss
     */
    virtual std::string P2pAddr()    = 0; 
    /**
     * @brief print usage or help info
     * 
     * @return std::string help info
     */
    virtual std::string HelpInfo()   = 0; 
};

typedef std::shared_ptr<MultilayerNetworkInterface> MultilayerNetworkInterfacePtr;

} // end namespace elect

} // end namespace top
