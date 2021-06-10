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

};

typedef std::shared_ptr<MultilayerNetworkChainQuery> MultilayerNetworkChainQueryPtr;

} // end namespace elect

} // end namespace top
