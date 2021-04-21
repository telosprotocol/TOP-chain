# 序
## TOP链基本术语 & 介绍
    1.SEED：创世的种子节点，首批启动的链节点，互相组网并完成创世
    2.REC：根选举委员会，主要负责共识时钟、节点入网、节点注册、自身REC选举和ZEC选举，创世的首届REC由SEED节点担任，全网有且只有一个REC集群
    3.ZEC：ZONE选举委员会，主要负责审计/验证集群的选举和部分系统合约[工作量统计/增发/奖励]的业务共识，全网有且只有一个ZEC集群
    4.审计集群：主要负责普通交易路由和共识审计，创世期间由SEED节点担任审计节点[单个节点可以担任不同集群中的工作节点]，审计集群数量可以配置，最少1个，最多2个
    5.验证集群：主要负责普通交易的共识，创世期间由SEED节点担任验证节点[单个节点可以担任不同集群中的工作节点]，验证集群数量可以配置，最少1个，最多4个
    6.EDGE：主要负责转发客户端发送的交易，创世期间由SEED节点担任EDGE节点[单个节点可以担任不同集群中的工作节点]，全网有且只有一个EDGE集群
    7.集群大小：TOP链普通共识采用xBFT共识机制，负责交易共识的单个集群最少需要3个不同的节点工作
## TOP链创世流程简介
### 从零搭建一条TOP创世链需要经过以下几个步骤
    1.准备账号，包括创世账号/SEED节点账号
    2.准备组网配置文件
    3.每个SEED节点使用单独的SEED账号启动链程序完成组网创世
    4.查询创世信息&发送交易
## 前置说明
    1.节点程序启动时追加启动参数配置文件，通过配置文件覆盖程序中的关键参数，从而按照配置的拓扑进行组网创世
    2.AllInOne是单个机器启动多个链进程，每个链进程代表一个独立的节点程序，多个链节点进程相互通讯组网完创世
    3.多机集群是多个机器，每个机器启动单个节点程序，机器之间相互通讯组网完成创世
    4.本文档描述的部署操作，成功组建的测试网是单个shard、集群不换届的组网；可以通过新节点注册入网增加候选池触发集群选举
# 部署AllInOne
## 确认环境：
    OS：Centos
    内核版本：Linux version 3.10.0-1127.13.1.el7.x86_64
    用户：root
## 编译
### 安装编译环境
    sudo yum install -y wget cmake3 readline-devel.x86_64 gcc gcc-g++ tree

    ***必须安装gcc 4.8.5版本***
    执行以下命令检查：
    gcc -v
    预期结果如下：
    gcc version 4.8.5 20150623 (Red Hat 4.8.5-39) (GCC) 
### 编译程序包
    进入工程根目录执行编译命令： ./build.sh debug mainnet_activated
    编译成功后检查：
    ls -l cbuild/bin/Linux/xtopchain
    ls -l cbuild/bin/Linux/topio
    ls -l cbuild/lin/Linux/libxtopchain.so.1.1.3
    其中xtopchain是纯粹的链程序文件，topio是包含了客户端功能的链程序文件，topio要配合libxtopchain.so才能正常工作；
## 准备安装文件
### 下载部署依赖包
    mkdir -p /home/top && cd /home/top && wget xxx...deploy_allinone.tar.gz
    tar -zxvf deploy_allinone.tar.gz
### 复制程序
    将编译成功后生成的xtopchain/topio/libxtopchain.so复制到/home/top目录下
### 检查安装依赖文件
    tree -a /home/top/
    /home/top/
    ├── config
    │   ├── config.rec1.json
    │   ├── config.rec2.json
    │   ├── config.rec3.json
    │   ├── config.rec4.json
    │   ├── config.rec5.json
    │   └── config.rec6.json
    ├── deploy_allinone.tar.gz
    ├── .edge_config.json
    ├── keystore
    │   ├── T00000LcxcHVTKki5KqCKmX5BbbMSGrUPhTEpwnu
    │   ├── T00000LfazE9WjtUu4xx5caD9w9dWwiRGggHxxvo
    │   ├── T00000LfyErVp716mVR89UdJMQaZJ6W9Hv7NsHT4
    │   ├── T00000Lhj29VReFAT958ZqFWZ2ZdMLot2PS5D5YC
    │   ├── T00000LhPZXie5GqcZqoxu6BkfMUqo7e9x1EaFS6
    │   ├── T00000LKMWBfiHPeN9TShPdB9ctLDr7mmcZa46Da
    │   ├── T00000LKy8CAPRETJmUr9DZiwCPcc9VFqDBpQEG6
    │   ├── T00000LLLK8teSrpbtcVnaYCtykniJHHeMZegQMU
    │   ├── T00000LPjk6KqsnCu46kHffttVPjToRmitpStvvo
    │   ├── T00000LQkSo2PYR7hVxZmkQphvsxAbiB8V22PwPT
    │   ├── T00000LRauRZ3SvMtNhxcvoHtP8JK9pmZgxQCe7Q
    │   └── T00000LRc5LxEXVdUuFcoLezjPiMw4CLYgvkEmtC
    ├── libxtopchain.so
    ├── run.sh
    ├── topio
    └── xtopchain

    ***
    config文件夹内包含6个节点的启动配置
    .edge_config.json是topio的配置文件
    keystore文件夹内包含3个创世账号、3个TCC账号、6个基金会节点账号的私钥文件
    run.sh是启动脚本
    ***
## 部署组网
    执行部署命令：cd /home/top && sh run.sh
## 查询创世结果
    grep -a 'vnode mgr' /tmp/rec*/log/xtop*log|grep -a consensus|grep -a 'starts at'
    预期可以检索到6条日志，每一条日志表示一个节点当选并开始工作
    ***执行部署后预计5分钟内完成创世***
## 使用链上业务功能
### 环境变量设置topio的工作目录
    export TOPIO_HOME=/home/top
### topio加载创世账号
    输入以下命令加载账号：
    cd /home/top && ./topio wallet setDefaultAccount T00000Lhj29VReFAT958ZqFWZ2ZdMLot2PS5D5YC
    预期结果如下：
    Set default account successfully.
    
    输入以下命令进行检查：
    ./topio wallet listaccounts|grep T00000Lhj29VReFAT958ZqFWZ2ZdMLot2PS5D5YC -A 3
    预期结果如下：
    account #0: T00000Lhj29VReFAT958ZqFWZ2ZdMLot2PS5D5YC [default account]
    owner public-key: BNic1L4jTsDPq1Gq/yXVtiUOJwCGuo/z3Z8gF56BdV5VHgtooUpffMQMMHTiY+olwTth/htKuiKH+EkSxrgqnEg=
    balance: 2999997000.000000 TOP
    nonce: 1
### 创建新账号
    输入以下命令创建线下账号：
    cd /home/top && ./topio wallet createaccount
    预期结果如下：
    Successfully create an account locally!
    Account Address: T00000XXXXXXXX

    输入以下命令转账给线下账号将其创建到线上：
    cd /home/top && ./topio transfer T00000XXXXXXXX 1000000 tx_test
    预期结果如下：
    Transaction hash: 0x...
    Please use command 'topio querytx' to query transaction status later on!!!

    输入以下命令查询交易共识结果：
    cd /home/top && ./topio querytx 0x...
    预期结果如下：
    ...
    "exec_status": "success"
    ...
    "errmsg": "OK"
    ...

    ***转账交易的确认时间一般在30~60秒内完成***
# 部署多机集群
## 确认环境[以6个SEED创世进行介绍]：
    OS：Centos * 7(台)
    内核版本：Linux version 3.10.0-1127.13.1.el7.x86_64
    用户：root/123456
    IP段：192.168.50.135 ~ 192.168.50.141
    主机规划：
        192.168.50.135 作为ansible操作主机
        192.168.50.136 ~ 192.168.50.141 作为部署节点主机 
    ***以上主机均安装centos7 office镜像***
## 编译
    编译准备和编译命令见"部署AllInOne"的"编译"章节
## 准备安装文件
### 安装依赖
    root身份登录192.168.50.135，依次执行以下命令安装依赖
    sudo yum -y update && yum install -y ansible jq tree 
### 修改ssh连接配置
    输入以下命令禁用ssh指纹校验
    echo "StrictHostKeyChecking no" >> /etc/ssh/ssh_config
### 下载部署依赖包
    创建部署文件夹并下载部署依赖包
    mkdir -p /home/top && && wget xxx...deploy_multi_hosts.tar.gz
    tar -zxvf deploy_multi_hosts.tar.gz
### 复制程序
    将编译成功后生成的xtopchain/topio/libxtopchain.so复制到192.168.50.135主机的/home/top目录下
### 检查安装依赖文件
    登录192.168.50.135，执行以下命令检查：
    tree -a /home/top
    /home/top
    ├── deploy_multi_hosts.zip
    ├── .edge_config.json
    ├── keystore
    │   ├── T00000LcxcHVTKki5KqCKmX5BbbMSGrUPhTEpwnu
    │   ├── T00000LfazE9WjtUu4xx5caD9w9dWwiRGggHxxvo
    │   ├── T00000LfyErVp716mVR89UdJMQaZJ6W9Hv7NsHT4
    │   ├── T00000Lhj29VReFAT958ZqFWZ2ZdMLot2PS5D5YC
    │   ├── T00000LhPZXie5GqcZqoxu6BkfMUqo7e9x1EaFS6
    │   ├── T00000LKMWBfiHPeN9TShPdB9ctLDr7mmcZa46Da
    │   ├── T00000LKy8CAPRETJmUr9DZiwCPcc9VFqDBpQEG6
    │   ├── T00000LLLK8teSrpbtcVnaYCtykniJHHeMZegQMU
    │   ├── T00000LPjk6KqsnCu46kHffttVPjToRmitpStvvo
    │   ├── T00000LQkSo2PYR7hVxZmkQphvsxAbiB8V22PwPT
    │   ├── T00000LRauRZ3SvMtNhxcvoHtP8JK9pmZgxQCe7Q
    │   └── T00000LRc5LxEXVdUuFcoLezjPiMw4CLYgvkEmtC
    ├── libxtopchain.so
    ├── run.sh
    ├── topio
    ├── topo.json
    └── xtopchain

    ***
    .edge_config.json是topio的配置文件
    keystore文件夹内包含3个创世账号、3个TCC账号、6个基金会节点账号的私钥文件
    run.sh是启动脚本
    ***
## 部署组网
    root登录192.168.50.135主机，执行以下命令部署：
    cd /home/top && sh run.sh
## 查询创世结果
    root登录192.168.50.135主机，执行以下命令：
    cd /home/top && ansible -i workdir/hosts all -m shell -a "grep 'vnode mgr' /home/top/log/xtop*log -a|grep -a consensus|grep -a 'starts at'"
    预期可以检索到6条日志，每一条日志表示一个节点当选并开始工作
    ***执行部署后预计5分钟内完成创世***
## 使用链上业务功能
### 环境变量设置topio的工作目录
    root登录192.168.50.135主机，执行以下命令部署：
    export TOPIO_HOME=/home/top
### 修改topio配置文件
    root登录192.168.50.135主机，执行以下命令修改：
    sed -i "s#127.0.0.1#192.168.50.135#g" /home/top/.edge_config.json
### topio加载创世账号
    输入以下命令加载账号：
    cd /home/top && ./topio wallet setDefaultAccount T00000Lhj29VReFAT958ZqFWZ2ZdMLot2PS5D5YC
    预期结果如下：
    Set default account successfully.
    
    输入以下命令进行检查：
    ./topio wallet listaccounts|grep T00000Lhj29VReFAT958ZqFWZ2ZdMLot2PS5D5YC -A 3
    预期结果如下：
    account #0: T00000Lhj29VReFAT958ZqFWZ2ZdMLot2PS5D5YC [default account]
    owner public-key: BNic1L4jTsDPq1Gq/yXVtiUOJwCGuo/z3Z8gF56BdV5VHgtooUpffMQMMHTiY+olwTth/htKuiKH+EkSxrgqnEg=
    balance: 2999997000.000000 TOP
    nonce: 1
### 创建新账号
    输入以下命令创建线下账号：
    cd /home/top && ./topio wallet createaccount
    预期结果如下：
    Successfully create an account locally!
    Account Address: T00000XXXXXXXX

    输入以下命令转账给线下账号将其创建到线上：
    cd /home/top && ./topio transfer T00000XXXXXXXX 1000000 tx_test
    预期结果如下：
    Transaction hash: 0x...
    Please use command 'topio querytx' to query transaction status later on!!!

    输入以下命令查询交易共识结果：
    cd /home/top && ./topio querytx 0x...
    预期结果如下：
    ...
    "exec_status": "success"
    ...
    "errmsg": "OK"
    ...

    ***转账交易的确认时间一般在30~60秒内完成***
# 更多
## 配置文件说明
    ```json
    {
        "ip": "127.0.0.1", //每个节点进程分配一个单独的回环地址作为通讯地址
        "node_id": "T0000LNi...TUp", //每个节点进程分配一个独立的节点账号
        "http_port": 19081, //RPC服务端口
        "grpc_port": 19082, //GRPC服务端口
        "ws_port": 19085, //WebSocket服务端口
        "log_level": 0, //日志级别，0：DEBUG，1：INFO，2：KeyINFO
        "log_path": "/tmp/rec1", //每个节点进程分配一个单独的运行日志存放目录
        "db_path": "/tmp/rec1", //每个节点进程分配一个单独的DB存放目录
        "node_type": "advance", //节点入网的类型，创世SEED统一设置advance
        "public_key": "BNR...XW4=", //节点公钥，与分配的节点账号对应
        "sign_key": "KA7...WhY=", //节点私钥，与分配的节点账号对应
        "auditor_group_count": 1, //创世后审计集群个数，可取值1,2
        "validator_group_count": 1, //创世后验证集群个数，可取值1,2,3,4
        "min_election_committee_size": 6, //创世后REC集群的工作节点数下限，该数值<=max_election_committee_size
        "max_election_committee_size": 6, //创世后REC集群的工作节点数上限，该数值=启动SEED节点进程的数量
        "min_auditor_group_size": 3, //创世后单个审计集群的工作节点数下限，该数值<=max_auditor_group_size
        "max_auditor_group_size": 3, //创世后单个审计集群的工作节点数上限，该数值=启动SEED节点进程的数量/(auditor_group_count+validator_group_count)向下取整
        "min_validator_group_size": 3, //创世后单个验证集群的工作节点数下限，该数值<=max_validator_group_size
        "max_validator_group_size": 3, //创世后单个验证集群的工作节点数下限，该数值=启动SEED节点进程的数量/(auditor_group_count+validator_group_count)向下取整
        "rec_election_interval": 181, //REC集群轮换的时间周期，单位1个链时钟(10s)，有足够的候选节点才会进行换届选举
        "zec_election_interval": 211, //ZEC集群轮换的时间周期，单位1个链时钟(10s)，有足够的候选节点才会进行换届选举
        "zone_election_trigger_interval": 5, //审计和验证集群轮换检查时间，单位1个链时钟(10s)
        "cluster_election_interval": 71, //审计和验证集群轮换的时间周期，单位1个链时钟(10s)，有足够的候选节点才会进行换届选举
        "genesis": { //以下是创世块的信息
            "accounts": { //上帝账号
                "tcc": {
                    "T00000LcxcHVTKki5KqCKmX5BbbMSGrUPhTEpwnu":  
                        {"balance": "1000000000"},  //链上治理委员会账号以及初始资金，单位uTOP
                    "T00000LfazE9WjtUu4xx5caD9w9dWwiRGggHxxvo":  
                        {"balance": "1000000000"},  //链上治理委员会账号以及初始资金，单位uTOP
                    "T00000LfyErVp716mVR89UdJMQaZJ6W9Hv7NsHT4":  
                        {"balance": "1000000000"}   //链上治理委员会账号以及初始资金，单位uTOP
                },
                "genesis_funds_account": {
                    "T00000Lhj29VReFAT958ZqFWZ2ZdMLot2PS5D5YC": 
                        {"balance": "2999997000000000"}, //基金会资金账号以及初始资金，单位uTOP
                    "T00000LhPZXie5GqcZqoxu6BkfMUqo7e9x1EaFS6": 
                        {"balance": "6000000000000000"}, //STAKE资金账号以及初始资金，单位uTOP
                    "T00000LKMWBfiHPeN9TShPdB9ctLDr7mmcZa46Da": 
                        {"balance": "3400000000000000"}  //预留资金账号以及初始资金，单位uTOP
                }
            },
            "seedNodes": { //SEED节点信息，格式[账号地址:账号公钥]，作用于组建物理P2P网络和进行共识
                "T00000LKy8CAPRETJmUr9DZiwCPcc9VFqDBpQEG6": "BO1...dyk=",
                "T00000LLLK8teSrpbtcVnaYCtykniJHHeMZegQMU": "BHW...59E=",
                "T00000LPjk6KqsnCu46kHffttVPjToRmitpStvvo": "BKn...dcQ=",
                "T00000LQkSo2PYR7hVxZmkQphvsxAbiB8V22PwPT": "BJE...xx4=",
                "T00000LRauRZ3SvMtNhxcvoHtP8JK9pmZgxQCe7Q": "BBZ...d0k=",
                "T00000LRc5LxEXVdUuFcoLezjPiMw4CLYgvkEmtC": "BAR...Q4Q="
            },
            "timestamp": 1599555555
        },
        "platform": {
            "business_port": 9001, //节点间物理通讯的UDP端口，单机部署时每个进程必须使用独立端口
            "url_endpoint": "http://unnecessary.org", // 测试环境设置成无效的域名即可
            "public_endpoints": "127.0.0.1:9001,127.0.0.2:9002" //节点间物理通讯的SEED URL，格式[ip_1:port_1,...ip_N:port_N]，1<填入的个数<=rec节点个数
        }
    }
    ```
## topio全量命令支持
    [命令行使用](http://developers.topnetwork.org/Tools/TOPIO/CliUsage/Overview/)