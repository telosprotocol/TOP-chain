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
     * @brief get all peers info of root routing table
     * 
     * @return std::string all peers info
     */
    virtual std::string Peers()   = 0;
    /**
     * @brief id and ip:port, such as tnode://b4062cb0463fb6cf6576337cebff8414387ed445739e458d23bb037f2aadc6c41205147f92@127.0.0.1:30303
     * 
     * @return std::string p2p addresss
     */
    virtual std::string P2pAddr()    = 0; 
#ifdef DEBUG
    /**
     * @brief broadcast
     *
     * @param msg_size msg size
     * @param count count
     * @return uint32_t
     */
    virtual uint32_t Broadcast(uint32_t msg_size, uint32_t count) = 0;
#endif
};

typedef std::shared_ptr<MultilayerNetworkInterface> MultilayerNetworkInterfacePtr;

} // end namespace elect

} // end namespace top
