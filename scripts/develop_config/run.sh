kill -9 `ps -ef | grep topio | grep -v grep | awk '{print $2}'`

rm -rf ~/topnetwork/db/* ~/topnetwork/log/*
rm -rf /tmp/rec1/
rm -rf /tmp/rec2/
rm -rf /tmp/rec3/
rm -rf /tmp/rec4/
rm -rf /tmp/rec5/
rm -rf /tmp/rec6/
rm -rf /tmp/rec7/
rm -rf /tmp/edge/
rm -rf /tmp/adv1/
rm -rf /tmp/adv2/
rm -rf /tmp/adv3/
rm -rf /tmp/adv4/
rm -rf /tmp/adv5/
rm -rf /tmp/adv6/
rm -rf /tmp/adv7/
rm -rf /tmp/adv8/
rm -rf /tmp/adv9/
rm -rf /tmp/con1/
rm -rf /tmp/con2/
rm -rf /tmp/con3/
rm -rf /tmp/con4/
rm -rf /tmp/con5/
rm -rf /tmp/con6/
rm -rf /tmp/con7/
rm -rf /tmp/con8/
rm -rf /tmp/con9/
rm -rf /tmp/zec1/
rm -rf /tmp/zec2/
rm -rf /tmp/zec3/
rm -rf /tmp/zec4/
rm -rf /tmp/arc1/
rm -rf /tmp/arc2/

rm -rf /cores/*

mkdir -p /tmp/rec1/
mkdir -p /tmp/rec2/
mkdir -p /tmp/rec3/
mkdir -p /tmp/rec4/
mkdir -p /tmp/rec5/
mkdir -p /tmp/rec6/
mkdir -p /tmp/rec7/
mkdir -p /tmp/edge/
mkdir -p /tmp/adv1/
mkdir -p /tmp/adv2/
mkdir -p /tmp/adv3/
mkdir -p /tmp/adv4/
mkdir -p /tmp/adv5/
mkdir -p /tmp/adv6/
mkdir -p /tmp/adv7/
mkdir -p /tmp/adv8/
mkdir -p /tmp/adv9/
mkdir -p /tmp/con1/
mkdir -p /tmp/con2/
mkdir -p /tmp/con3/
mkdir -p /tmp/con4/
mkdir -p /tmp/con5/
mkdir -p /tmp/con6/
mkdir -p /tmp/con7/
mkdir -p /tmp/con8/
mkdir -p /tmp/con9/
mkdir -p /tmp/zec1/
mkdir -p /tmp/zec2/
mkdir -p /tmp/zec3/
mkdir -p /tmp/zec4/
mkdir -p /tmp/arc1/
mkdir -p /tmp/arc2/

touch /tmp/rec1/xtop.log
touch /tmp/rec2/xtop.log
touch /tmp/rec3/xtop.log
touch /tmp/rec4/xtop.log
touch /tmp/rec5/xtop.log
touch /tmp/rec6/xtop.log
touch /tmp/rec7/xtop.log
touch /tmp/edge/xtop.log
touch /tmp/adv1/xtop.log
touch /tmp/adv2/xtop.log
touch /tmp/adv3/xtop.log
touch /tmp/adv4/xtop.log
touch /tmp/con1/xtop.log
touch /tmp/con2/xtop.log
touch /tmp/con3/xtop.log
touch /tmp/con4/xtop.log
touch /tmp/con5/xtop.log
touch /tmp/con6/xtop.log
touch /tmp/con7/xtop.log
touch /tmp/zec1/xtop.log
touch /tmp/zec2/xtop.log
touch /tmp/zec3/xtop.log
touch /tmp/zec4/xtop.log
touch /tmp/arc1/xtop.log
touch /tmp/arc2/xtop.log

ulimit -c unlimited
rm -f core*
sleep 0.5

./topio node startNode -c ./config/config.rec1.json &
sleep 0.5
./topio node startNode -c ./config/config.rec2.json &
sleep 0.5
./topio node startNode -c ./config/config.rec3.json &
sleep 0.5
./topio node startNode -c ./config/config.rec4.json &
sleep 0.5
./topio node startNode -c ./config/config.rec5.json &
sleep 0.5
./topio node startNode -c ./config/config.zec1.json &
sleep 0.5
./topio node startNode -c ./config/config.zec2.json &
sleep 0.5
./topio node startNode -c ./config/config.zec3.json &
sleep 0.5
./topio node startNode -c ./config/config.zec4.json &
sleep 0.5
./topio node startNode -c ./config/config.con1.json &
sleep 0.5
./topio node startNode -c ./config/config.con2.json &
sleep 0.5
./topio node startNode -c ./config/config.con3.json &
sleep 0.5
./topio node startNode -c ./config/config.con4.json &
sleep 0.5
./topio node startNode -c ./config/config.con5.json &
sleep 200
./topio node startNode -c ./config/config.con6.json &
sleep 0.5
./topio node startNode -c ./config/config.con7.json &
sleep 0.5
./topio node startNode -c ./config/config.con8.json &
sleep 0.5
./topio node startNode -c ./config/config.con9.json &
sleep 0.5
./topio node startNode -c ./config/config.adv1.json &
sleep 0.5
./topio node startNode -c ./config/config.adv2.json &
sleep 0.5
./topio node startNode -c ./config/config.adv3.json &
sleep 0.5
./topio node startNode -c ./config/config.adv4.json &
sleep 0.5
./topio node startNode -c ./config/config.adv5.json &
sleep 0.5
./topio node startNode -c ./config/config.adv6.json &
sleep 0.5
./topio node startNode -c ./config/config.adv7.json &
sleep 0.5
./topio node startNode -c ./config/config.adv8.json &
sleep 0.5
./topio node startNode -c ./config/config.adv9.json &
sleep 0.5
./topio node startNode -c ./config/config.edge.json &
sleep 0.5
#./topio node startNode -c ./config/config.arc1.json &
#sleep 0.5
#./topio node startNode -c ./config/config.arc2.json &
#sleep 90
#./topio node startNode -c ./config/config.adv4.conf &
sleep 1
#./topio node startNode -c ./config/config.con5.conf &
