kill -9 `ps -ef |grep topio |grep -v grep |grep -v sh|awk '{print $2}'`

echo ""
echo "##############using topio config mode(xtopchain mode) to build chain-network###################"
echo ""

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

mkdir -p /tmp/rec1/log/
mkdir -p /tmp/rec2/log/
mkdir -p /tmp/rec3/log/
mkdir -p /tmp/rec4/log/
mkdir -p /tmp/rec5/log/
mkdir -p /tmp/rec6/log/
mkdir -p /tmp/rec7/log/
mkdir -p /tmp/edge/log/
mkdir -p /tmp/adv1/log/
mkdir -p /tmp/adv2/log/
mkdir -p /tmp/adv3/log/
mkdir -p /tmp/adv4/log/
mkdir -p /tmp/adv5/log/
mkdir -p /tmp/adv6/log/
mkdir -p /tmp/adv7/log/
mkdir -p /tmp/adv8/log/
mkdir -p /tmp/adv9/log/
mkdir -p /tmp/con1/log/
mkdir -p /tmp/con2/log/
mkdir -p /tmp/con3/log/
mkdir -p /tmp/con4/log/
mkdir -p /tmp/con5/log/
mkdir -p /tmp/con6/log/
mkdir -p /tmp/con7/log/
mkdir -p /tmp/con8/log/
mkdir -p /tmp/con9/log/
mkdir -p /tmp/zec1/log/
mkdir -p /tmp/zec2/log/
mkdir -p /tmp/zec3/log/
mkdir -p /tmp/zec4/log/
mkdir -p /tmp/arc1/log/
mkdir -p /tmp/arc2/log/



ulimit -c unlimited
rm -f core*
sleep 2

./topio -c  ./config/config.rec1.json --datadir /tmp/rec1 &
sleep 0.5
./topio -c  ./config/config.rec2.json --datadir /tmp/rec2 &
sleep 0.5
./topio -c  ./config/config.rec3.json --datadir /tmp/rec3 &
sleep 0.5
./topio -c  ./config/config.rec4.json --datadir /tmp/rec4 &
sleep 0.5
./topio -c  ./config/config.rec5.json --datadir /tmp/rec5 &
sleep 0.5
./topio -c  ./config/config.zec1.json --datadir /tmp/zec1 &
sleep 0.5
./topio -c  ./config/config.zec2.json --datadir /tmp/zec2 &
sleep 0.5
./topio -c  ./config/config.zec3.json --datadir /tmp/zec3 &
sleep 0.5
./topio -c  ./config/config.zec4.json --datadir /tmp/zec4 &
sleep 0.5
./topio -c  ./config/config.con1.json --datadir /tmp/con1 &
sleep 0.5
./topio -c  ./config/config.con2.json --datadir /tmp/con2 &
sleep 0.5
./topio -c  ./config/config.con3.json --datadir /tmp/con3 &
sleep 0.5
./topio -c  ./config/config.con4.json --datadir /tmp/con4 &
sleep 0.5
./topio -c  ./config/config.con5.json --datadir /tmp/con5 &
sleep 300
./topio -c  ./config/config.con6.json --datadir /tmp/con6 &
sleep 0.5
./topio -c  ./config/config.con7.json --datadir /tmp/con7 &
sleep 0.5
./topio -c  ./config/config.con8.json --datadir /tmp/con8 &
sleep 0.5
./topio -c  ./config/config.con9.json --datadir /tmp/con9 &
sleep 0.5
./topio -c  ./config/config.adv1.json --datadir /tmp/adv1 &
sleep 0.5
./topio -c  ./config/config.adv2.json --datadir /tmp/adv2 &
sleep 0.5
./topio -c  ./config/config.adv3.json --datadir /tmp/adv3 &
sleep 0.5
./topio -c  ./config/config.adv4.json --datadir /tmp/adv4 &
sleep 0.5
./topio -c  ./config/config.adv5.json --datadir /tmp/adv5 &
sleep 0.5
./topio -c  ./config/config.adv6.json --datadir /tmp/adv6 &
sleep 0.5
./topio -c  ./config/config.adv7.json --datadir /tmp/adv7 &
sleep 0.5
./topio -c  ./config/config.adv8.json --datadir /tmp/adv8 &
sleep 0.5
./topio -c  ./config/config.adv9.json --datadir /tmp/adv9 &
sleep 0.5
./topio -c  ./config/config.edge.json --datadir /tmp/edge &
sleep 0.5
#./topio -c  ./config/config.arc1.json --datadir /tmp/g &
#sleep 0.5
#./topio -c  ./config/config.arc2.json --datadir /tmp/g &
#sleep 90
#./topio -c  ./config/config.adv4.conf &
sleep 1
#./topio -c  ./config/config.con5.conf &
