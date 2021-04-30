<center>网络层模拟搭建多层网络简易教程</center>

# 说明
多层网络的概念是属于链的概念，链有多个分片的概念，每个分片对应一个 p2p 网络，形成不同层级的网络。为了让网络层能独立于链上业务，进行了模拟组网，不依赖于链上业务，但多层网络架构是相同的。

本教程实现了一个网络层的 demo，实现模拟链上组网。

# 步骤
## 分支
目前在开发分支，位于 supernode-smaug-prepare-dev

```
 git checkout -b supernode-smaug-prepare-dev remotes/origin/supernode-smaug-prepare-dev
```

## 编译
```
sh build.sh metrics noratelimit test
```

完成这一步会生成我们的目标二进制文件 xelect\_net\_demo，具体位于：

```
cbuild/bin/Linux/xelect_net_demo
```

## 运行
执行下述命令：

```
cp cbuild/bin/Linux/xelect_net_demo  src/xtopcom/xelect_net/demo/deploy
cd  src/xtopcom/xelect_net/demo
cp -rf deploy  /tmp   # 或者其他目录也行，要求是有较大的 disk 空间
cd /tmp/deploy
```
简单描述一下，上述命令把目标二进制 xelect_net_demo 拷贝到  deploy 目录，其中 deploy 目录是用来进行部署和搭建的目录，包括程序运行产生的db 数据和日志的目录。（所以要求磁盘空间要大一点）

deploy 目录下面的文件有：

```
ls deploy
config  db  log  update_config.py  xelect_net_demo
```

其中 config 目录下默认有一个 static_network.config.default 文件，里面包含了多层网络的一些预定义信息：

```
[xelect_net_demo]
total=163
rec=8
zec=8
edg=2
adv_net=2
adv=70
val_net=4
val=60
```
比如总节点数量为 163 个；rec 节点 8 个； zec 8 个；edge 2 个，；adv 集群有 2 个，其中每个 adv 集群节点 70 个；val 集群有 2 个，其中每个 val 集群有 60 个；

然后另外比较重要的一个文件就是  update_config.py 这个文件了，这个文件是用来部署多节点网络的工具：

```
python update_config.py -h

usage: update_config.py [-h] -m {cent,dist} [-i INIT] [-s START] [-r RESTART]
                        [-k KILL]

xelect_net_demo 运维工具，支持根据 host 文件生成所有节点的
config 文件；支持启动、终止、重启 xelect_net_demo 等

optional arguments:
  -h, --help            show this help message and exit
  -m {cent,dist}, --mode {cent,dist}
                        run mode: cent[ral] for deploy multi-nodes in local-
                        machine; dist[ributed] for deploy multi-nodes in
                        distributed-machines
  -i INIT, --init INIT  only using this in proxy-host(jump-host) to generate
                        all config files
  -s START, --start START
                        start xelect_net_demo
  -r RESTART, --restart RESTART
                        restart xelect_net_demo
  -k KILL, --kill KILL  kill xelect_net_demo

```

简单解释下支持的命令参数：

部署分为***单机多节点部署，以及分布式多机部署***。

### 单机多节点部署
单机多节点部署也就是在一台机器上启动多个节点进行组网。使用的时候需要使用 -m cent 参数，这个参数是**必须**的。第一次运行需要先初始化生成多节点的配置文件以及 db 、log 目录等。具体来说，命令如下：

```
python update_config.py -h   # 任何时候都可以查看 help
cp config/static_network.config.default config/static_network.config          # 多层网络配置
python update_config.py -m cent -i true   # 完成初始化
python update_config.py -m cent -s true   # 启动多节点
```

至此，完成单机多节点搭建。


### 分布式多机部署
分布式多机部署也就是在多台机器上部署组网，每台机器启动一个节点。使用的时候使用  -m dist 参数。


具体步骤此处不做介绍。（暂时略）


# 查看组网、运行情况
```
ps -ef |grep xelect_net_demo
```

db 、log 目录下分别有每个节点的数据和日志，可以具体去到某个 log 目录里分析日志。
