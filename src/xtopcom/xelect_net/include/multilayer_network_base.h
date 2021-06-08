#if 0
#pragma once
#include <mutex>

#include "xpbase/base/top_config.h"

namespace top {

namespace elect {

class MultilayerNetworkBase {
public:
    MultilayerNetworkBase()                                          = default;
    MultilayerNetworkBase(MultilayerNetworkBase const &)             = delete;
    MultilayerNetworkBase & operator=(MultilayerNetworkBase const &) = delete;
    MultilayerNetworkBase(MultilayerNetworkBase &&)                  = delete;
    MultilayerNetworkBase & operator=(MultilayerNetworkBase &&)      = delete;
    virtual ~MultilayerNetworkBase()                                 = default;

    /**
     * @brief parse params and config
     * 
     * @param argc argc
     * @param argv argv
     * @param edge_config params stored in edge_config after parse 
     * @return int 
     */
    virtual int HandleParamsAndConfig(int argc, char** argv, top::base::Config& edge_config) = 0;
    /**
     * @brief parse params
     * 
     * @param argc argc
     * @param argv argv
     * @param args_parser the instance of parse 
     * @return int 
     */
    virtual int ParseParams(int argc, char** argv, top::ArgsParser& args_parser) = 0;
    /**
     * @brief do some init
     * 
     * @param config config struct
     * @return true 
     * @return false 
     */
    virtual bool Init(const base::Config& config) = 0;
    /**
     * @brief run
     * 
     * @param config 
     * @return true 
     * @return false 
     */
    virtual bool Run(const base::Config& config) = 0;
    /**
     * @brief stop
     * 
     */
    virtual void Stop() = 0;
};

}  // namespace elect

}  // namespace top
#endif