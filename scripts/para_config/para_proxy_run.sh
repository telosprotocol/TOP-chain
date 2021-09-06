kill -9 `ps -ef |grep xpara_proxy_chain|awk '{print $2}'`

rm -rf ~/topnetwork/db/* ~/topnetwork/log/*
rm -rf /tmp/para/rec1/
rm -rf /tmp/para/rec2/
rm -rf /tmp/para/rec3/
rm -rf /tmp/para/rec4/
rm -rf /tmp/para/rec5/
rm -rf /tmp/para/rec6/
rm -rf /tmp/para/rec7/
rm -rf /tmp/para/edge/
rm -rf /tmp/para/adv1/
rm -rf /tmp/para/adv2/
rm -rf /tmp/para/adv3/
rm -rf /tmp/para/adv4/
rm -rf /tmp/para/adv5/
rm -rf /tmp/para/adv6/
rm -rf /tmp/para/adv7/
rm -rf /tmp/para/adv8/
rm -rf /tmp/para/adv9/
rm -rf /tmp/para/con1/
rm -rf /tmp/para/con2/
rm -rf /tmp/para/con3/
rm -rf /tmp/para/con4/
rm -rf /tmp/para/con5/
rm -rf /tmp/para/con6/
rm -rf /tmp/para/con7/
rm -rf /tmp/para/con8/
rm -rf /tmp/para/con9/
rm -rf /tmp/para/zec1/
rm -rf /tmp/para/zec2/
rm -rf /tmp/para/zec3/
rm -rf /tmp/para/zec4/

rm -rf /cores/*

mkdir -p /tmp/para/rec1/
mkdir -p /tmp/para/rec2/
mkdir -p /tmp/para/rec3/
mkdir -p /tmp/para/rec4/
mkdir -p /tmp/para/rec5/
mkdir -p /tmp/para/rec6/
mkdir -p /tmp/para/rec7/
mkdir -p /tmp/para/edge/
mkdir -p /tmp/para/adv1/
mkdir -p /tmp/para/adv2/
mkdir -p /tmp/para/adv3/
mkdir -p /tmp/para/adv4/
mkdir -p /tmp/para/adv5/
mkdir -p /tmp/para/adv6/
mkdir -p /tmp/para/adv7/
mkdir -p /tmp/para/adv8/
mkdir -p /tmp/para/adv9/
mkdir -p /tmp/para/con1/
mkdir -p /tmp/para/con2/
mkdir -p /tmp/para/con3/
mkdir -p /tmp/para/con4/
mkdir -p /tmp/para/con5/
mkdir -p /tmp/para/con6/
mkdir -p /tmp/para/con7/
mkdir -p /tmp/para/con8/
mkdir -p /tmp/para/con9/
mkdir -p /tmp/para/zec1/
mkdir -p /tmp/para/zec2/
mkdir -p /tmp/para/zec3/
mkdir -p /tmp/para/zec4/

touch /tmp/para/rec1/xtop.log
touch /tmp/para/rec2/xtop.log
touch /tmp/para/rec3/xtop.log
touch /tmp/para/rec4/xtop.log
touch /tmp/para/rec5/xtop.log
touch /tmp/para/rec6/xtop.log
touch /tmp/para/rec7/xtop.log
touch /tmp/para/edge/xtop.log
touch /tmp/para/adv1/xtop.log
touch /tmp/para/adv2/xtop.log
touch /tmp/para/adv3/xtop.log
touch /tmp/para/adv4/xtop.log
touch /tmp/para/con1/xtop.log
touch /tmp/para/con2/xtop.log
touch /tmp/para/con3/xtop.log
touch /tmp/para/con4/xtop.log
touch /tmp/para/con5/xtop.log
touch /tmp/para/con6/xtop.log
touch /tmp/para/con7/xtop.log
touch /tmp/para/zec1/xtop.log
touch /tmp/para/zec2/xtop.log
touch /tmp/para/zec3/xtop.log
touch /tmp/para/zec4/xtop.log
ulimit -c unlimited
rm -f core*
sleep 2

./xpara_proxy_chain ./para_config/config.rec1.json &
sleep 0.5
./xpara_proxy_chain ./para_config/config.rec2.json &
sleep 0.5
./xpara_proxy_chain ./para_config/config.rec3.json &
sleep 0.5
./xpara_proxy_chain ./para_config/config.rec4.json &
sleep 0.5
./xpara_proxy_chain ./para_config/config.rec5.json &
sleep 0.5
./xpara_proxy_chain ./para_config/config.zec1.json &
sleep 0.5
./xpara_proxy_chain ./para_config/config.zec2.json &
sleep 0.5
./xpara_proxy_chain ./para_config/config.zec3.json &
sleep 0.5
./xpara_proxy_chain ./para_config/config.zec4.json &
sleep 0.5
./xpara_proxy_chain ./para_config/config.con1.json &
sleep 0.5
./xpara_proxy_chain ./para_config/config.con2.json &
sleep 0.5
./xpara_proxy_chain ./para_config/config.con3.json &
sleep 0.5
./xpara_proxy_chain ./para_config/config.con4.json &
sleep 0.5
./xpara_proxy_chain ./para_config/config.con5.json &
sleep 300
./xpara_proxy_chain ./para_config/config.con6.json &
sleep 0.5
./xpara_proxy_chain ./para_config/config.con7.json &
sleep 0.5
./xpara_proxy_chain ./para_config/config.con8.json &
sleep 0.5
./xpara_proxy_chain ./para_config/config.con9.json &
sleep 0.5
./xpara_proxy_chain ./para_config/config.adv1.json &
sleep 0.5
./xpara_proxy_chain ./para_config/config.adv2.json &
sleep 0.5
./xpara_proxy_chain ./para_config/config.adv3.json &
sleep 0.5
./xpara_proxy_chain ./para_config/config.adv4.json &
sleep 0.5
./xpara_proxy_chain ./para_config/config.adv5.json &
sleep 0.5
./xpara_proxy_chain ./para_config/config.adv6.json &
sleep 0.5
./xpara_proxy_chain ./para_config/config.adv7.json &
sleep 0.5
./xpara_proxy_chain ./para_config/config.adv8.json &
sleep 0.5
./xpara_proxy_chain ./para_config/config.adv9.json &
sleep 0.5
./xpara_proxy_chain ./para_config/config.edge.json &
sleep 0.5
