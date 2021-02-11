kill -9 `ps -ux |grep xtopchain|awk '{print $2}'`

rm -rf /tmp/rec1/
rm -rf /tmp/rec2/
rm -rf /tmp/rec3/
rm -rf /tmp/zec1/
rm -rf /tmp/zec2/
rm -rf /tmp/zec3/
rm -rf /tmp/adv1/
rm -rf /tmp/adv2/
rm -rf /tmp/adv3/
rm -rf /tmp/con1/
rm -rf /tmp/con2/
rm -rf /tmp/con3/
rm -rf /tmp/con4/
rm -rf /tmp/edge/

rm -rf core.*

mkdir -p /tmp/rec1/
mkdir -p /tmp/rec2/
mkdir -p /tmp/rec3/
mkdir -p /tmp/zec1/
mkdir -p /tmp/zec2/
mkdir -p /tmp/zec3/
mkdir -p /tmp/adv1/
mkdir -p /tmp/adv2/
mkdir -p /tmp/adv3/
mkdir -p /tmp/con1/
mkdir -p /tmp/con2/
mkdir -p /tmp/con3/
mkdir -p /tmp/con4/
mkdir -p /tmp/edge/

ulimit -c unlimited

sleep 2

./xtopchain config/config.rec1.json &
sleep 1
./xtopchain config/config.rec2.json &
sleep 1
./xtopchain config/config.rec3.json &
sleep 1
./xtopchain config/config.zec1.json &
sleep 1
./xtopchain config/config.zec2.json &
sleep 1
./xtopchain config/config.zec3.json &
sleep 1
./xtopchain config/config.adv1.json &
sleep 1
./xtopchain config/config.adv2.json &
sleep 1
./xtopchain config/config.adv3.json &
sleep 1
./xtopchain config/config.con1.json &
sleep 1
./xtopchain config/config.con2.json &
sleep 1
./xtopchain config/config.con3.json &
sleep 1
./xtopchain config/config.con4.json &
sleep 1
./xtopchain config/config.edge.json &
sleep 1
