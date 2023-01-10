#!/bin/bash

while getopts "j:x:h" arg;
do
    case $arg in
        j)
            TJSON=$OPTARG
            ;;
        x)
            TOP_FILE=$OPTARG
            ;;
        h)
            echo -e "-j: 指定拓扑文件路径\n"
            echo -e "-x: 指定部署的xtopchain程序包绝对路径\n"
            exit 0
            ;;
        ?)
            echo "unexpect param!"
            exit -1
            ;;
    esac
done

echo -e "\n============================================================"
date
echo "[TRACE] >>> Verify XTOPCHAIN & HOSTS"

PWD=$(pwd)
WORK_DIR=${PWD}/workdir
RUNFILE=${WORK_DIR}/run_normal.yml
COPYFILE=${WORK_DIR}/dispense.yml

if [ ! -f "$TJSON" ];then
	echo "TJSON ("${TJSON}") no exist!"
 	exit -1
fi

if [ ! -f "$TOP_FILE" ];then
	echo "TOP FILE ("${TOP_FILE}") no exist!"
 	exit -1
fi
CHAIN_FILE=$(basename ${TOP_FILE})

TRAGET_GROUP="all"

# clean file
if [[ -d ${WORK_DIR} ]];then
    rm -fr ${WORK_DIR}
fi

mkdir -p ${WORK_DIR}
cd ${WORK_DIR}

# get host auth
authName=$(jq -r ".connect.username" $TJSON)
authPort=$(jq -r ".connect.port" $TJSON)
authPawd=$(jq -r ".connect.password" $TJSON)

# make hosts
allIpHost=$(jq -r ".rec|.[].host" $TJSON)
allArray=(${allIpHost})

# fix all
cat > hosts << EOF
[all]
EOF

for item in ${allArray[*]};do
cat >> hosts << EOF
$item ansible_ssh_user=$authName ansible_ssh_pass=$authPawd ansible_ssh_port=$authPort
EOF
done

echo "[TRACE] >>> Verify PASS"

echo -e "\n============================================================"
date
echo "[TRACE] >>> Prepare CONFIG and YML FILEs"

recNodes=''
rec_count=$(jq -r ".public.max_election_committee_size" $TJSON)
for i in $(seq 1 $rec_count);do
  let j="$i-1"
  rec_pubkey=$(jq -r ".rec|.[$j].pubkey" $TJSON)
  rec_account=$(jq -r ".rec|.[$j].account" $TJSON)
  recNodes=${recNodes}"\""${rec_account}"\":\""${rec_pubkey}"\","
done
recNodes=${recNodes%?}

pubEndPoints=''
busPort=$(jq -r ".public.BusPort" $TJSON)
rec_count=$(jq -r ".public.max_election_committee_size" $TJSON)
let rec_count_part="${rec_count}-3"
for i in $(seq 1 $rec_count_part);do
  let j="$i-1"
  rec_host=$(jq -r ".rec|.[$j].host" $TJSON)
  pubEndPoints=${pubEndPoints}${rec_host}":"${busPort}","
done
pubEndPoints=${pubEndPoints%?}

httpPort=$(jq -r ".public.httpPort" $TJSON)
grpcPort=$(jq -r ".public.grpcPort" $TJSON)
wsPort=$(jq -r ".public.wsPort" $TJSON)
busPort=$(jq -r ".public.BusPort" $TJSON)
recEleInt=$(jq -r ".public.rec_election_interval" $TJSON)
zecEleInt=$(jq -r ".public.zec_election_interval" $TJSON)
zoneEleInt=$(jq -r ".public.zone_election_trigger_interval" $TJSON)
comMinSize=$(jq -r ".public.min_election_committee_size" $TJSON)
comMaxSize=$(jq -r ".public.max_election_committee_size" $TJSON)
clsEleInt=$(jq -r ".public.cluster_election_interval" $TJSON)
advClsCount=$(jq -r ".public.auditor_group_count" $TJSON)
conClsCount=$(jq -r ".public.validator_group_count" $TJSON)
advMinSize=$(jq -r ".public.min_auditor_group_size" $TJSON)
advMaxSize=$(jq -r ".public.max_auditor_group_size" $TJSON)
conMinSize=$(jq -r ".public.min_validator_group_size" $TJSON)
conMaxSize=$(jq -r ".public.max_validator_group_size" $TJSON)
logLevel=$(jq -r ".public.logLevel" $TJSON)
TARGET_DIR=$(jq -r ".public.workdir" $TJSON)
DB_FOLDER=$(jq -r ".public.dbPath" $TJSON)
PDB_FOLDER=$(jq -r ".public.pdbPath" $TJSON)
LOG_FOLDER=$(jq -r ".public.logPath" $TJSON)
DB_DIR=${TARGET_DIR}/${DB_FOLDER}
LOG_DIR=${TARGET_DIR}/${LOG_FOLDER}

# fix config
cat > config.tmp << EOF
{
  "ip": "local_ip",
  "node_id": "rp_node_id",
  "http_port": ${httpPort},
  "grpc_port": ${grpcPort},
  "ws_port": ${wsPort},
  "log_level": ${logLevel},
  "log_path": "${LOG_DIR}",
  "db_path": "${DB_DIR}",
  "node_type": "rp_node_type",
  "public_key": "rp_pub_key",
  "sign_key": "rp_pri_key",
  "auditor_group_count":${advClsCount},
  "validator_group_count":${conClsCount},
  "min_election_committee_size":${comMinSize},
  "max_election_committee_size":${comMaxSize},
  "min_auditor_group_size":${advMinSize},
  "max_auditor_group_size":${advMaxSize},
  "min_validator_group_size":${conMinSize},
  "max_validator_group_size":${conMaxSize},
  "rec_election_interval":${recEleInt},
  "zec_election_interval":${zecEleInt},
  "zone_election_trigger_interval":${zoneEleInt},
  "cluster_election_interval":${clsEleInt},
  "edge_election_interval":17,
  "archive_election_interval":13,
  "genesis": {
        "accounts" : {
            "tcc": {
                "T00000LcxcHVTKki5KqCKmX5BbbMSGrUPhTEpwnu": {"balance": "1000000000"},
                "T00000LfazE9WjtUu4xx5caD9w9dWwiRGggHxxvo": {"balance": "1000000000"},
                "T00000LfyErVp716mVR89UdJMQaZJ6W9Hv7NsHT4": {"balance": "1000000000"}
            },
            "genesis_funds_account": {
                "T00000Lhj29VReFAT958ZqFWZ2ZdMLot2PS5D5YC": {"balance": "2999997000000000"},
                "T00000LhPZXie5GqcZqoxu6BkfMUqo7e9x1EaFS6": {"balance": "6000000000000000"},
                "T00000LKMWBfiHPeN9TShPdB9ctLDr7mmcZa46Da": {"balance": "3400000000000000"}
            }
        },
        "seedNodes": {${recNodes}},
        "timestamp": 1599555555
    },
  "node_p2p_port": ${busPort},
  "p2p_endpoints": "${pubEndPoints}",
  "p2p_url_endpoints" : "http://unnecessary.org",
  "root_hash": ""
}
EOF

cat > dispense.yml << EOF
---
- hosts: "{{ host }}"
  tasks:
  - copy:
      src: "${WORK_DIR}/{{ inventory_hostname }}.config"
      dest: ${TARGET_DIR}/config.json
EOF

cat > run_normal.yml << EOF
---
- hosts: "{{ host }}"
  remote_user: root
  tasks:
  - name: run xtop
    shell: "(ulimit -c unlimited && export GLIBCXX_FORCE_NEW=1 && cd ${TARGET_DIR} && ./${CHAIN_FILE} config.json > /dev/null 2>&1 &)"
    async: 8
    poll: 0
EOF

echo -e "\n============================================================"
date
echo "[TRACE] >>> CLEAN HOSTS PROCESS"

ansible -i ${WORK_DIR}/hosts ${TRAGET_GROUP} -m shell -a "ps -ef|grep '[.]/${CHAIN_FILE}' | grep -v '/dev/null' | awk '{print \$2}' | xargs -I {} bash -c 'if \[ ! -z {} \] ;then kill -9 {} ;fi;'"

echo -e "\n============================================================"
date
echo "[TRACE] >>> CLEAN HOSTS WORK FOLDER"

ansible -i ${WORK_DIR}/hosts ${TRAGET_GROUP} -m file -a "path=${TARGET_DIR} state=absent"

echo -e "\n============================================================"
date
echo "[TRACE] >>> CREATE HOSTS WORK FOLDER"

ansible -i ${WORK_DIR}/hosts ${TRAGET_GROUP} -m file -a "path=${TARGET_DIR} state=directory"
ansible -i ${WORK_DIR}/hosts ${TRAGET_GROUP} -m file -a "path=${DB_DIR} state=directory"
ansible -i ${WORK_DIR}/hosts ${TRAGET_GROUP} -m file -a "path=${LOG_DIR} state=directory"

echo -e "\n============================================================"
date
echo "[TRACE] >>> Dispath XTOPCHAIN"

cp $TOP_FILE $WORK_DIR/${CHAIN_FILE}
tar -zcvf ${CHAIN_FILE}.tar.gz ${CHAIN_FILE}
XTOP_TAR_FILE=${WORK_DIR}/${CHAIN_FILE}.tar.gz
ansible -i ${WORK_DIR}/hosts ${TRAGET_GROUP} -m unarchive -a "src=${XTOP_TAR_FILE} dest=${TARGET_DIR} mode=0755 copy=yes"
if [[ $? -ne 0 ]];then
    echo 'ansible dispath xtopchain failed'
    exit -1
fi

echo -e "\n============================================================"
date
echo "[TRACE] >>> COMPLETE CONFIG FILEs:"

# fix rec config
rec_count=$(jq -r ".rec|.[]" $TJSON|grep account|wc -l)
for i in $(seq 1 $rec_count);do
    let k="$i-1"
    this_ip=$(jq -r ".rec|.[$k]|.host" $TJSON)
    this_pub_key=$(jq -r ".rec|.[$k]|.pubkey" $TJSON)
    this_pri_key=$(jq -r ".rec|.[$k]|.prikey" $TJSON)
    this_account=$(jq -r ".rec|.[$k]|.account" $TJSON) 
    cp -fr config.tmp ${this_ip}.config
    sed -i "s#local_ip#$this_ip#g" ${this_ip}.config
    sed -i "s#rp_node_id#$this_account#g" ${this_ip}.config
    sed -i "s#rp_node_type#advance#g" ${this_ip}.config
    sed -i "s#rp_pub_key#$this_pub_key#g" ${this_ip}.config
    sed -i "s#rp_pri_key#$this_pri_key#g" ${this_ip}.config
done

echo -e "\n============================================================"
date
echo "[TRACE] >>> Dispense CONFIG FILEs"

# transfer config
ansible-playbook -i ${WORK_DIR}/hosts -e host=${TRAGET_GROUP} ${COPYFILE}
if [[ $? -ne 0 ]];then
    echo "dispense config failed!"
    exit -1
fi

echo -e "\n============================================================"
date
echo "[TRACE] >>> CHECK CONFIG FILEs"

ansible -i ${WORK_DIR}/hosts ${TRAGET_GROUP} -m shell -a "ls ${TARGET_DIR}/config.json"
if [[ $? -ne 0 ]];then
  echo "some node can not dispense config.json!"
  exit -1
fi

echo -e "\n============================================================"
date
echo "[TRACE] >>> START REC NODEs"

# run node
ansible-playbook -i ${WORK_DIR}/hosts -e host=${TRAGET_GROUP} ${RUNFILE} 
sleep 60

ansible -i ${WORK_DIR}/hosts ${TRAGET_GROUP} -m shell -a "ps -ef|grep '[.]/${CHAIN_FILE}'|grep -v dev|awk '{print \$2}'|sort -nr|head -n1|xargs -I {} bash -c 'netstat -anop|grep {}'"
stat=$(ansible -i ${WORK_DIR}/hosts ${TRAGET_GROUP} -m shell -a "ps -ef|grep '[.]/${CHAIN_FILE}'|grep -v dev")

# echo $stat
ret=$(echo $stat|grep -o FAILED|wc -l)
if [ $ret -gt 0 ];then
    echo -e "\n============================================================"
    date
    echo "[TRACE] >>> XTOPCHAIN DEPLOY FAILED"
    echo 'node down sum: '$ret
    echo $stat|sed 's#>>#\n#g'|grep FAILED
    exit -1
fi

echo -e "\n============================================================"
date
echo "[TRACE] >>> XTOPCHAIN DEPLOY SUCCESS"



