#!/bin/bash
# source /etc/profile

# set -x

PROJECT_DIR=`pwd`
TARGET_DIR=$1
REPORT_DIR=$2
TEMP_HTML=$3
TARGET_CASE=$4

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

    for dir in $(find . -type d -name '*'${dir_key}'*' -not -path src/xtopcom/xdepends);do
        dir_params=${dir_params}' -d '${dir}
        # dir_base=$(basename $dir)
        # if [[ "${dir_base}" == "${dir_key}" ]];then
        #     dir_params=${dir_params}' -d '${dir}
        # fi
    done
done


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
    
    echo "running "${utest}

    if [ "$utest_file" == "xevm_engine_test" ]; then
        time ./${utest_file} ${PROJECT_DIR}/tests/xevm_engine_test/test_cases/ --gtest_output="xml:"${utest_file}"_report.xml" --gtest_filter=-*.*BENCH*
    else
        time ./${utest_file} --gtest_output="xml:"${utest_file}"_report.xml" --gtest_filter=-*.*BENCH*
    fi

    cp -r ${utest_file}_report.xml ${UT_REPORT_DIR}/${utest_file}.xml

    cd ${PROJECT_DIR}

    lcov -c ${dir_params} -o ${COVER_DIR}/${utest_file}.info
    
    if [[ -e tmp.info ]];then
        lcov -a ${COVER_DIR}/tmp.info -a ${COVER_DIR}/${utest_file}.info -o ${COVER_DIR}/tmp.info
    else
        lcov -a ${COVER_DIR}/init.info -a ${COVER_DIR}/${utest_file}.info -o ${COVER_DIR}/tmp.info
    fi
    rm -f ${COVER_DIR}/${utest_file}.info
done

# clean 
lcov --remove ${COVER_DIR}/tmp.info '*.h' '*/test*' '*/tests*' '*/third_party*' '*/usr/include*' '*/usr/lib*' '*/xdepends*' '*/xtools*' -o ${COVER_DIR}/all_tests.info

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
tar -zcvf ut_coverage.tar.gz ./${REPORT_DIR}
