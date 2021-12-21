
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
        CMAKE_EXTRA_OPTIONS+=" -DXENABLE_TESTS=ON"
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
    xmutisign_close)
        CMAKE_EXTRA_OPTIONS+=" -DMUTI_SIGN_CLOSE=ON"
        echo "MUTI_SIGN_CLOSE ON"
    ;;
    drop_commit)
        CMAKE_EXTRA_OPTIONS+=" -DDROP_COMMIT_TEST=ON"
        echo "LEADER_DROP_COMMIT_TEST ON"
    ;;
    eherror)
        CMAKE_EXTRA_OPTIONS+=" -DXENABLE_EXCEPTION_AS_ERROR=ON"
        echo "Enable XTHROW as an xerror log"
    ;;
    noratelimit)
        CMAKE_EXTRA_OPTIONS+=" -DXDISABLE_RATELIMIT=ON"
        echo "Disable rate limit"
    ;;
    mock_stake_zec)
        CMAKE_EXTRA_OPTIONS+=" -DXENABLE_MOCK_ZEC_STAKE=ON"
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
    scale)
        CMAKE_EXTRA_OPTIONS+=" -DENABLE_SCALE=ON"
        echo "DECAY REDEEM TGAS DISK ENABLE_SCALE ON"
    ;;
    config_check)
        CMAKE_EXTRA_OPTIONS+=" -DXENABLE_CONFIG_CHECK=ON"
        echo "enable config check when node boot"
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
    mock_ca)
        CMAKE_EXTRA_OPTIONS+=" -DMOCK_CA=ON"
        echo "BUILD WITH MOCK CA"
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
    no_tx_batch)
        CMAKE_EXTRA_OPTIONS+=" -DNO_TX_BATCH=ON"
        echo "BUILD WITH NO_TX_BATCH"
    ;;
    create_user)
        CMAKE_EXTRA_OPTIONS+=" -DENABLE_CREATE_USER=ON"
        echo "BUILD WITH ENABLE_CREATE_USER"
    ;;
    long_confirm_check)
        CMAKE_EXTRA_OPTIONS+=" -DLONG_CONFIRM_CHECK=ON"
        echo "BUILD WITH LONG_CONFIRM_CHECK"
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
    leak_trace)
        CMAKE_EXTRA_OPTIONS+=" -DLEAK_TRACER=ON"
        echo "BUILD WITH LEAK_TRACER tool"
    ;;
    store_unit_block)
        CMAKE_EXTRA_OPTIONS+=" -DSTORE_UNIT_BLOCK=ON"
        echo "BUILD WITH store unit block tool"
    ;;
    chain_forked_by_default)
        CMAKE_EXTRA_OPTIONS+=" -DXCHAIN_FORKED_BY_DEFAULT=ON"
        echo "BUILD WITH XCHAIN_FORKED_BY_DEFAULT (all fork points are forked BY DEFAULT)"
    ;;
    metrics_dataobject)
        CMAKE_EXTRA_OPTIONS+=" -DENABLE_METRICS_DATAOBJECT=ON"
        echo "BUILD WITH ENABLE_METRICS_DATAOBJECT"
    ;;
    disable_core_signal_capture)
        CMAKE_EXTRA_OPTIONS+=" -DDISABLE_CORE_SIGNAL_CAPTURE=ON"
        echo "BUILD WITH DISABLE_CORE_SIGNAL_CAPTURE"
    ;;
    *)
        CMAKE_EXTRA_OPTIONS=" -DXENABLE_TESTS=OFF -DXENABLE_CODE_COVERAGE=OFF"
    ;;
    esac
done
