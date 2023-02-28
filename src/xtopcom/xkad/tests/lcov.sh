set -e

distrib=`cat /etc/*-release | uniq -u | grep ^ID= | grep -oP "ID=\"*\K\w+"`
pids=$(ps -ef | grep SimpleHTTPServer | grep -v grep | awk -F' ' '{print $2}')
echo "pids: $pids"
if [ -n "$pids" ]; then
    kill -9 $pids
fi

parent_path=$(pwd) # xchain path

lcov_path="/tmp/lcov_test"
mkdir -p ${lcov_path}
/usr/bin/rm -rf ${lcov_path}/*
cd ${lcov_path}
echo "cd lcov_path:${lcov_path}"

#xmodule_list="xpbase xtransport xkad xgossip xelect_net"
xmodule_list="xpbase xtransport xkad xgossip "

merge_lcov_param=""
for xmodule in ${xmodule_list};
do
    echo "cover for module:${xmodule} #################"
    binname="${xmodule}_test"

    # begin handle test for module
    # excute test_bin
    ${parent_path}/cbuild/bin/Linux/${binname}
    
    echo "begin analyze module:${binname} =========================1"
    lcov_capture_path="${parent_path}/cbuild/src/xtopcom/${xmodule}/CMakeFiles/${xmodule}.dir/src/"
    
    lcov -d $lcov_capture_path -o ${binname}.info1  -c
    
    lcov --remove ${binname}.info1 \
        '/usr/*' \
        $parent_path'/src/xtopcom/xdepends/*' \
        $parent_path'/src/xtopcom/xbasic/*' \
        $parent_path'/src/xtopcom/xcommon/*' \
        $parent_path'/src/xtopcom/xcrypto/*' \
        $parent_path'/src/xtopcom/xdata/*' \
        $parent_path'/src/xtopcom/xnetwork/*' \
        $parent_path'/src/xtopcom/xkad/proto/'$distrib'/*' \
        -o ${binname}.info
    echo "analyze module:${binname} finished =========================2"
    # end handle test for module

    merge_lcov_param="${merge_lcov_param} -a ${binname}.info"
done

# merge all modules info
lcov ${merge_lcov_param}  -o all.info

mkdir -p result
genhtml -o result all.info
echo "=========================3"
cd result

echo ''
echo ''
echo ''

echo "visit this link to view code recover: http://0.0.0.0:8000"
echo ''
echo ''
python -m SimpleHTTPServer > /dev/null 2>&1 &

echo ''
