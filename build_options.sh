#unset cmake cache
CMAKE_EXTRA_OPTIONS+=" -UXBUILD_TEST -UDISABLE_EVM -UDISABLE_RATELIMIT -UDISABLE_REAL_STAKE -UMAINNET_ACTIVATED -USTATIC_CONSENSUS -UELECT_WHEREAFTER -UCONSENSUS_SWAP"

for option in $options
do
    case $option in
    build_ci)
        CMAKE_EXTRA_OPTIONS+=" -DXBUILD_CI=ON"
        echo "Build CI testnet"
    ;;
    build_dev)
        CMAKE_EXTRA_OPTIONS+=" -DXBUILD_DEV=ON"
        echo "Build Dev testnet"
    ;;
    build_galileo)
        CMAKE_EXTRA_OPTIONS+=" -DXBUILD_GALILEO=ON"
        echo "Build Galileo testnet"
    ;;
    build_bounty)
        CMAKE_EXTRA_OPTIONS+=" -DXBUILD_BOUNTY=ON"
        echo "Build Bounty testnet"
    ;;
    test)
        CMAKE_EXTRA_OPTIONS+=" -DXBUILD_TEST=ON"
        if [ $osname == "Linux" ]; then
            CMAKE_EXTRA_OPTIONS+=" -DXENABLE_CODE_COVERAGE=ON"
        fi
        echo "Build with test project"
    ;;
    metrics)
        CMAKE_EXTRA_OPTIONS+=" -DBUILD_METRICS=ON"
        echo "BUILD METRICS mode"
    ;;
    xsecurity)
        CMAKE_EXTRA_OPTIONS+=" -DBUILD_XSECURITY=ON"
        echo "BUILD XSECURITY mode"
    ;;
    gperf)
        CMAKE_EXTRA_OPTIONS+=" -DBUILD_GPERF=ON"
        echo "BUILD GPERF mode"
    ;;
    ghperf)
        CMAKE_EXTRA_OPTIONS+=" -DBUILD_GHPERF=ON"
        echo "BUILD GHPERF mode"
    ;;
    tcmalloc)
        CMAKE_EXTRA_OPTIONS+=" -DTCMALLOC=ON"
        echo "BUILD TCMALLOC mode"
    ;;
    noratelimit)
        CMAKE_EXTRA_OPTIONS+=" -DDISABLE_RATELIMIT=ON"
        echo "Disable rate limit"
    ;;
    mock_stake_zec)
        CMAKE_EXTRA_OPTIONS+=" -DDISABLE_REAL_STAKE=ON"
        echo "Enable mocking stake for ZEC"
    ;;
    debug)
        CMAKE_EXTRA_OPTIONS+=" -DCMAKE_BUILD_TYPE=Debug"  # -g
        CBUILD_DIR="cbuild"
    ;;
    reldbg)
        CMAKE_EXTRA_OPTIONS+=" -DCMAKE_BUILD_TYPE=RelWithDebInfo"  # -O2 -g -DNDEBUG
        CBUILD_DIR="cbuild_reldbg"
    ;;
    release)
        CMAKE_EXTRA_OPTIONS+=" -DCMAKE_BUILD_TYPE=Release"  # -O2(Mac: -O3) -DNDEBUG
        CBUILD_DIR="cbuild_release"
    ;;
    address_sanitizer)
        CMAKE_EXTRA_OPTIONS+=" -DADDRESS_SANITIZER=ON"
        echo "ADDRESS SANITIZER ON"
    ;;
    slash_test)
        CMAKE_EXTRA_OPTIONS+=" -DSLASH_TEST=ON"
        echo "enable slash test when node boot"
    ;;
    workload_test)
        CMAKE_EXTRA_OPTIONS+=" -DWORKLOAD_TEST=ON"
        echo "enable work load test when node boot"
    ;;
    mainnet_activated)
        CMAKE_EXTRA_OPTIONS+=" -DMAINNET_ACTIVATED=ON"
        echo "enable mainnet activation by default"
    ;;
    p_stack)
        CMAKE_EXTRA_OPTIONS+=" -DXENABLE_PSTACK=ON"
        echo "BUILD WITH XENABLE_PSTACK"
    ;;
    memcheck_dbg)
        CMAKE_EXTRA_OPTIONS+=" -DXENABLE_MEMCHECK_DBG=ON"
        echo "BUILD WITH XENABLE_MEMCHECK_DBG"
    ;;
    static_consensus)
        CMAKE_EXTRA_OPTIONS+=" -DSTATIC_CONSENSUS=ON"
        echo "BUILD WITH STATIC CONSENSUS"
    ;;
    elect_whereafter)
        CMAKE_EXTRA_OPTIONS+=" -DELECT_WHEREAFTER=ON"
        echo "BUILD WITH ELECT WHEREAFTER"
    ;;
    consensus_swap)
        CMAKE_EXTRA_OPTIONS+=" -DCONSENSUS_SWAP=ON"
        echo "BUILD WITH CONSENSUS SWAP"
    ;;
    p2p_bendwidth)
        CMAKE_EXTRA_OPTIONS+=" -DXENABLE_P2P_BENDWIDTH=ON"
        echo "BUILD WITH ENABLE P2P BENDWIDTH"
    ;;
    p2p_test)
        CMAKE_EXTRA_OPTIONS+=" -DXENABLE_P2P_TEST=ON"
        echo "BUILD WITH ENABLE P2P TEST"
    ;;
    no_tx_batch)
        CMAKE_EXTRA_OPTIONS+=" -DNO_TX_BATCH=ON"
        echo "BUILD WITH NO_TX_BATCH"
    ;;
    create_user)
        CMAKE_EXTRA_OPTIONS+=" -DENABLE_CREATE_USER=ON"
        echo "BUILD WITH ENABLE_CREATE_USER"
    ;;
    db_kv_statistic)
        CMAKE_EXTRA_OPTIONS+=" -DDB_KV_STATISTIC=ON"
        echo "BUILD WITH DB_KV_STATISTIC"
    ;;
    db_cache)
        CMAKE_EXTRA_OPTIONS+=" -DDB_CACHE=ON"
        echo "BUILD WITH DB_CACHE"
    ;;
    db_tool)
        CMAKE_EXTRA_OPTIONS+=" -DDB_TOOL=ON"
        echo "BUILD WITH DB_TOOL"
    ;;
    rustvm)
        CMAKE_EXTRA_OPTIONS+=" -DBUILD_RUSTVM=ON"
        echo "BUILD RUSTVM(need cargo toolchain)"
    ;;
    disable_evm)
        CMAKE_EXTRA_OPTIONS+=" -DDISABLE_EVM=ON"
        echo "BUILD EVM(need cargo toolchain)"
    ;;
    leak_trace)
        CMAKE_EXTRA_OPTIONS+=" -DLEAK_TRACER=ON"
        echo "BUILD WITH LEAK_TRACER tool"
    ;;
    store_unit_block)
        CMAKE_EXTRA_OPTIONS+=" -DSTORE_UNIT_BLOCK=ON"
        echo "BUILD WITH store unit block tool"
    ;;
    chain_forked_by_default*)
        FORK_VERSION=`echo $option | sed 's/chain_forked_by_default[=]*//'`
        if [[ x$FORK_VERSION = x ]]; then
            FORK_VERSION=999999
        fi
        CMAKE_EXTRA_OPTIONS+=" -DXCHAIN_FORKED_BY_DEFAULT=$FORK_VERSION"
        echo "BUILD WITH -DXCHAIN_FORKED_BY_DEFAULT=$FORK_VERSION"
    ;;
    metrics_dataobject)
        CMAKE_EXTRA_OPTIONS+=" -DENABLE_METRICS_DATAOBJECT=ON"
        echo "BUILD WITH ENABLE_METRICS_DATAOBJECT"
    ;;
    disable_core_signal_capture)
        CMAKE_EXTRA_OPTIONS+=" -DDISABLE_CORE_SIGNAL_CAPTURE=ON"
        echo "BUILD WITH DISABLE_CORE_SIGNAL_CAPTURE"
    ;;
    disable_capture)
        CMAKE_EXTRA_OPTIONS+=" -DDISABLE_SIGNAL_CAPTURE=ON"
        echo "BUILD WITH DISABLE_SIGNAL_CAPTURE"
    ;;
    checkpoint_test)
        CMAKE_EXTRA_OPTIONS+=" -DCHECKPOINT_TEST=ON"
        echo "BUILD WITH CHECKPOINT_TEST"
    ;;
    eth_bridge_test)
        CMAKE_EXTRA_OPTIONS+=" -DETH_BRIDGE_TEST=ON"
        echo "BUILD WITH ETH_BRIDGE_TEST"
    ;;
    period_mock)
        CMAKE_EXTRA_OPTIONS+=" -DPERIOD_MOCK=ON"
        echo "BUILD WITH PERIOD_MOCK"
    ;;
    cross_tx_dbg)
        CMAKE_EXTRA_OPTIONS+=" -DCROSS_TX_DBG=ON"
        echo "BUILD WITH CROSS_TX_DBG"
    ;;
    use_jemalloc)
        CMAKE_EXTRA_OPTIONS+=" -DXUSE_JEMALLOC=ON"
        echo "BUILD WITH XUSE_JEMALLOC"
    ;;
    enable_jeprof)
        CMAKE_EXTRA_OPTIONS+=" -DENABLE_JEPROF=ON"
        echo "BUILD WITH ENABLE_JEPROF"
    ;;
    eth2_sepolia)
        CMAKE_EXTRA_OPTIONS+=" -DETH2_SEPOLIA=ON"
        echo "BUILD WITH ETH2_SEPOLIA"
    ;;
    build_consortium)
        CMAKE_EXTRA_OPTIONS+=" -DXBUILD_CONSORTIUM=ON"
        echo "Build Consortium testnet"
    ;;
    cache_size_statistic)
        CMAKE_EXTRA_OPTIONS+=" -DCACHE_SIZE_STATISTIC=ON"
        echo "BUILD WITH CACHE_SIZE_STATISTIC"
    ;;
    cache_size_statistic_more_detail)
        CMAKE_EXTRA_OPTIONS+=" -DCACHE_SIZE_STATISTIC_MORE_DETAIL=ON"
        echo "BUILD WITH CACHE_SIZE_STATISTIC_MORE_DETAIL"
    ;;
    *)
        echo "unknown build option: "$option
        exit
    ;;
    esac
done
