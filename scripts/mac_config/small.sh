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

rm -rf ./core.*

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

./xtopchain ./config.rec1.json &
sleep 1
./xtopchain ./config.rec2.json &
sleep 1
./xtopchain ./config.rec3.json &
sleep 30
./xtopchain ./config.zec1.json &
sleep 1
./xtopchain ./config.zec2.json &
sleep 1
./xtopchain ./config.zec3.json &
sleep 1
./xtopchain ./config.adv1.json &
sleep 1
./xtopchain ./config.adv2.json &
sleep 1
#./xtopchain ./config.adv3.json &
sleep 1
./xtopchain ./config.con1.json &
sleep 1
./xtopchain ./config.con2.json &
sleep 1
./xtopchain ./config.con3.json &
sleep 1
./xtopchain ./config.con4.json &
sleep 1
./xtopchain ./config.edge.json &
sleep 1
