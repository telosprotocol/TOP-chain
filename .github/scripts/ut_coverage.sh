#!/bin/bash
# source /etc/profile

# set -x

PROJECT_DIR=`pwd`
TARGET_DIR=$1
REPORT_DIR=$2
TEMP_HTML=$3
TARGET_CASE=$4
AUTO_CLEAR=$5

# package case list
CASE_LIST=(${TARGET_CASE//,/ })

#fix xml path
UT_REPORT_DIR=${PROJECT_DIR}/${REPORT_DIR}/ut

#fix cover path
COVER_DIR=${REPORT_DIR}/cover

#mkdir unit folder
mkdir -p ${UT_REPORT_DIR}
mkdir -p ${COVER_DIR}

# init coverage
cd ${PROJECT_DIR}

dir_params=''

for utest in $(ls ${TARGET_DIR}/*test);do

    utest_file=$(basename $utest)

    # verify valid case
    if [[ ! -z ${CASE_LIST} ]];then
        valid_case=$(echo ${CASE_LIST[@]}|grep -w $utest_file)
        if [ -z "$valid_case" ];then
            echo ${utest}" is not included, jump init"
            continue
        fi
    fi

    echo "init add "${utest}
    dir_key=$(echo ${utest_file} | sed 's/_test//g')

    for dir in $(find . -type d -name '*'${dir_key}'*');do
        dir_params=${dir_params}' -d '${dir}
        # dir_base=$(basename $dir)
        # if [[ "${dir_base}" == "${dir_key}" ]];then
        #     dir_params=${dir_params}' -d '${dir}
        # fi
    done
done

sync_spec_case=$(echo ${CASE_LIST[@]}|grep -w 'test_xmock_system')
if [ ! -z "$sync_spec_case" ];then
    echo ${sync_spec_case}" include, init add spec work"
    for dir in $(find . -type d -name '*xsync_trigger*');do
        dir_base=$(basename $dir)
        if [[ "${dir_base}" == "xsync_trigger" ]];then
            dir_params=${dir_params}' -d '${dir}
        fi
    done
    for dir in $(find . -type d -name '*xsync_executor*');do
        dir_base=$(basename $dir)
        if [[ "${dir_base}" == "xsync_executor" ]];then
            dir_params=${dir_params}' -d '${dir}
        fi
    done
    for dir in $(find . -type d -name '*xsyncbase*');do
        dir_base=$(basename $dir)
        if [[ "${dir_base}" == "xsyncbase" ]];then
            dir_params=${dir_params}' -d '${dir}
        fi
    done
fi

if [[ -z ${dir_params} ]];then
    echo "not found any link directory!"
    exit -1
fi

echo "dir params: "${dir_params}

lcov --no-external -c -i ${dir_params} -o ${COVER_DIR}/init.info

# run unittest
echo "testcase count:" $(ls ${TARGET_DIR}/*test|wc -l)

for utest in $(ls ${TARGET_DIR}/*test);do

    utest_file=$(basename $utest)

    # verify valid case
    if [[ ! -z ${CASE_LIST} ]];then
        valid_case=$(echo ${CASE_LIST[@]}|grep -w $utest_file)
        if [ -z "$valid_case" ];then
            echo ${utest}" is not included, jump test"
            continue
        fi
    fi

    cd $TARGET_DIR

    if [[ ! -z ${AUTO_CLEAR} ]];then
        find ${TARGET_DIR} -type f -not -name '*_test' -a -not -name '*xml' -a -not -name 'test_xmock_system' -print -exec rm -fr {} \;
    fi
    
    echo "running "${utest}
    time ${utest} --gtest_output="xml:"${utest}"_report.xml" --gtest_filter=-*.*BENCH*
    cp -r ${utest}_report.xml ${UT_REPORT_DIR}/${utest_file}.xml

    cd ${PROJECT_DIR}

    lcov -c ${dir_params} -o ${COVER_DIR}/${utest_file}.info
    
    if [[ -e tmp.info ]];then
        lcov -a ${COVER_DIR}/tmp.info -a ${COVER_DIR}/${utest_file}.info -o ${COVER_DIR}/tmp.info
    else
        lcov -a ${COVER_DIR}/init.info -a ${COVER_DIR}/${utest_file}.info -o ${COVER_DIR}/tmp.info
    fi

    rm -f ${COVER_DIR}/${utest_file}.info
done

if [ ! -z "$sync_spec_case" ];then
    echo ${sync_spec_case}" include, test add spec work"
    cd $TARGET_DIR

    if [[ ! -z ${AUTO_CLEAR} ]];then
        find ${TARGET_DIR} -type f -not -name '*_test' -a -not -name '*xml' -a -not -name 'test_xmock_system' -print -exec rm -fr {} \;
    fi
    
    echo "running test_xmock_system"
    time ./test_xmock_system --gtest_output="xml:test_xmock_system_report.xml"
    cp -r test_xmock_system_report.xml ${UT_REPORT_DIR}/test_xmock_system.xml

    cd ${PROJECT_DIR}

    lcov -c ${dir_params} -o ${COVER_DIR}/test_xmock_system.info
    
    if [[ -e tmp.info ]];then
        lcov -a ${COVER_DIR}/tmp.info -a ${COVER_DIR}/test_xmock_system.info -o ${COVER_DIR}/tmp.info
    else
        lcov -a ${COVER_DIR}/init.info -a ${COVER_DIR}/test_xmock_system.info -o ${COVER_DIR}/tmp.info
    fi

    rm -f ${COVER_DIR}/test_xmock_system.info
fi

# clean
lcov --remove ${COVER_DIR}/tmp.info \
    '*/usr/include*' '*/xcrypto*' '*/third_party*' '*/xdepends*' \
    '*/usr/lib*' '*libraries/xaccountblock*' '*libraries/xapplication*' '*libraries/xbookload*' \
    '*/xmock_system*' '*libraries/xelect*' '*/tests*' '*/test*' \
    '*/tests/CMakeFiles*' '*/tests/xchain_timer*' '*/tests/xcontract*' '*/tests/xdata_service*' \
    '*/tests/xelection*' '*/tests/xmake_keys*' '*/tests/xmock_system*' '*/tests/xrouter*' \
    '*/tests/xsigner*' '*/tests/xvm*' '*.h' \
    '*/unittest*' '*libraries/xservices*' '*programs/xchainclient*' \
    '*programs/xchainclient_t*' '*programs/xchainnode*' '*programs/xrpc_data*' \
    '*programs/xtopchain_t*' '*xtopcom/xarchive*' '*src/xtopcom/xkad/bench*' \
    '*xtopcom/xthreadpool*' '*xtopcom/xtrace_sdk*' '*xtopcom/xrrs*' \
    '*/xtopcom/xledger*' '*/libraries/xcomponent_administration*' '*src/xtopcom/xwrouter/demo*' \
    '*src/libraries/xchaininit*' '*src/xtopcom/https_client*' '*src/xtopcom/db_tool*' \
    '*src/xtopcom/xconsensus*' '*xrpc*pb.cc*' '*xrpc*pb.h*' '*xtools/xgenerate_account*' \
    '*xtopcom/xprotobuf*' '*xtopcom/xsecurity*' '*xtopcom/xstobject*' '*/proto*' \
    '*src/programs/xtopchain*' '*xtopcom/xelect_net/demo*' '*src/xtopcom/xnetwork*' -o ${COVER_DIR}/all_tests.info

# gen html
genhtml --output-directory ${COVER_DIR} \
  --demangle-cpp --num-spaces 2 --sort --prefix ${PROJECT_DIR} \
  --title "TOP Test Coverage" --function-coverage \
  --legend ${COVER_DIR}/all_tests.info

# gen junit report
cd ${PROJECT_DIR}
UHTML=$UT_REPORT_DIR/index.html
cp $TEMP_HTML $UHTML
echo 'unit xml num: ' $(ls ${UT_REPORT_DIR}|wc -l)
cd $UT_REPORT_DIR
summary_tests=0
summary_fails=0
for utxml in $(ls $UT_REPORT_DIR);do
    if [[ ! "$utxml" =~ "html" ]];then
        utx_base=$(basename ${utxml})
        utx_name=${utx_base/.xml/.html}
        junit2html $utxml $utx_name
        test_all=$(grep "testsuites" $utxml|grep -oE "tests=\"[0-9]+\""|grep -oE "[0-9]+")
        test_fail=$(grep "testsuites" $utxml|grep -oE "failures=\"[0-9]+\""|grep -oE "[0-9]+")
        test_disable=$(grep "testsuites" $utxml|grep -oE "disabled=\"[0-9]+\""|grep -oE "[0-9]+")
        test_errors=$(grep "testsuites" $utxml|grep -oE "errors=\"[0-9]+\""|grep -oE "[0-9]+")
        test_time=$(grep "testsuites" $utxml|awk -F "time=" '{print $2}'|awk -F "\"" '{print $2}')
        if [ -f $utx_name ];then
            let summary_tests="$summary_tests+$test_all"
            let summary_fails="$summary_fails+$test_fail+$test_errors"
            let issue="$test_fail+$test_disable+$test_errors"
            if [ $issue -gt 0 ];then
                sed -i 's#<!--content_top-->#<!--content_top-->\n<div class="step"><table><tbody><tr><td class="result"><a href="./'$utx_name'">'$utx_name'</a></td><td class="result"><a>AllTest:'$test_all' </a></td><td class="result"><a>Failures:'$test_fail' </a></td><td class="result"><a>Disabled:'$test_disable' </a></td><td class="result"><a>Errors:'$test_errors' </a></td><td class="result"><a>ElapsedTime:'$test_time' </a></td></tr></tbody></table></div>#g' $UHTML
            else
                sed -i 's#<!--content_bottom-->#<div class="step"><table><tbody><tr><td class="result"><a href="./'$utx_name'">'$utx_name'</a></td><td class="result"><a>AllTest:'$test_all' </a></td><td class="result"><a>ElapsedTime:'$test_time' </a></td></tr></tbody></table></div>\n<!--content_bottom-->#g' $UHTML
            fi
        fi
    fi
done
sed -i 's#<!--content_summary-->#<div class="step"><table><tbody><tr><td class="result"><a>SUMMARY ALL:'$summary_tests' FAIL:'$summary_fails'</a></td></tr></tbody></table></div>\n<!--content_summary-->#g' $UHTML

#tar report
cd ${PROJECT_DIR}
tar -zcvf ut_coverage.tar.gz ./ut_report_$(date +'%Y-%m-%d')