#!/bin/bash

WORK_DIR=$(pwd)
REPORT_DIR=${WORK_DIR}/ut_report
TEMP_HTML=.github/scripts/index.html
UHTML=${REPORT_DIR}/index.html

include_case=("xbasic_test" "xchain_timer_test" "xcommon_test" "xelection_test" "xrouter_test" "xsystem_contract_test" "xvnode_test")

# run unittest
echo "testcase count:" $(ls cbuild/bin/Linux/*test|wc -l)

if [ -d ${REPORT_DIR} ];then
    rm -fr ${REPORT_DIR}
fi
mkdir -p ${REPORT_DIR}

run_err_count=0
for utest in $(ls cbuild/bin/Linux/*test);do
    utest_file_name=$(basename $utest)
    time ${utest} --gtest_output="xml:"${utest_file_name}"_report.xml"
    if [[ $? -ne 0 ]];then
        let run_err_count="${run_err_count}+1"
        continue
    fi
    mv ${utest_file_name}"_report.xml" ${REPORT_DIR}/
done

cp $TEMP_HTML $UHTML
echo 'unit xml num: ' $(ls ${REPORT_DIR}|wc -l)
cd ${REPORT_DIR}
summary_tests=0
summary_fails=0
for utxml in $(ls ./*xml);do
    utx_base=$(basename ${utxml})
    utx_name=${utx_base/.xml/.html}
    /usr/local/bin/junit2html $utxml $utx_name
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
done
sed -i 's#<!--content_summary-->#<div class="step"><table><tbody><tr><td class="result"><a>SUMMARY ALL:'$summary_tests' FAIL:'$summary_fails'</a></td></tr></tbody></table></div>\n<!--content_summary-->#g' $UHTML

# archive
tar -zcvf report.tar.gz *html

# assert
let err_count="$test_fail+$test_errors+$run_err_count"
if [ ${err_count} -eq 0 ];then
    echo "no error testcase, done"
else
    echo "error testcase count: ${err_count}"
    exit -1
fi