kill -9 `ps -ef |grep xtopchain|awk '{print $2}'`

rm -rf /cores/*

ulimit -c unlimited
rm -f core*
sleep 2

./xtopchain ./config/config.rec1.json &
sleep 0.5
./xtopchain ./config/config.rec2.json &
sleep 0.5
./xtopchain ./config/config.rec3.json &
sleep 0.5
./xtopchain ./config/config.rec4.json &
sleep 0.5
./xtopchain ./config/config.rec5.json &
sleep 100
./xtopchain ./config/config.zec1.json &
sleep 0.5
./xtopchain ./config/config.zec2.json &
sleep 0.5
./xtopchain ./config/config.zec3.json &
sleep 0.5
./xtopchain ./config/config.zec4.json &
sleep 0.5
./xtopchain ./config/config.con1.json &
sleep 0.5
./xtopchain ./config/config.con2.json &
sleep 0.5
./xtopchain ./config/config.con3.json &
sleep 0.5
./xtopchain ./config/config.con4.json &
sleep 0.5
./xtopchain ./config/config.con5.json &
sleep 0.5
./xtopchain ./config/config.con6.json &
sleep 0.5
./xtopchain ./config/config.con7.json &
sleep 0.5
./xtopchain ./config/config.con8.json &
sleep 0.5
./xtopchain ./config/config.con9.json &
sleep 0.5
./xtopchain ./config/config.adv1.json &
sleep 0.5
./xtopchain ./config/config.adv2.json &
sleep 0.5
./xtopchain ./config/config.adv3.json &
sleep 0.5
./xtopchain ./config/config.adv4.json &
sleep 0.5
./xtopchain ./config/config.adv5.json &
sleep 0.5
./xtopchain ./config/config.adv6.json &
sleep 0.5
./xtopchain ./config/config.adv7.json &
sleep 0.5
./xtopchain ./config/config.adv8.json &
sleep 0.5
./xtopchain ./config/config.adv9.json &
sleep 0.5
./xtopchain ./config/config.edge.json &
sleep 0.5
#./xtopchain ./config/config.arc1.json &
#sleep 0.5
#./xtopchain ./config/config.arc2.json &
#sleep 90
#./xtopchain ./config/config.adv4.conf &
sleep 1
#./xtopchain ./config/config.con5.conf &
