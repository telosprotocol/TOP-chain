echo "clean env"

ps -ef |grep "[.]/topio"|awk '{print $2}'|xargs -I {} bash -c 'kill -9 {}'

set -e

rm -rf /tmp/rec1/
rm -rf /tmp/rec2/
rm -rf /tmp/rec3/
rm -rf /tmp/rec4/
rm -rf /tmp/rec5/
rm -rf /tmp/rec6/

sleep 1

echo "prepare env"
mkdir -p /tmp/rec1/log
mkdir -p /tmp/rec2/log
mkdir -p /tmp/rec3/log
mkdir -p /tmp/rec4/log
mkdir -p /tmp/rec5/log
mkdir -p /tmp/rec6/log
mkdir -p /tmp/rec1/db
mkdir -p /tmp/rec2/db
mkdir -p /tmp/rec3/db
mkdir -p /tmp/rec4/db
mkdir -p /tmp/rec5/db
mkdir -p /tmp/rec6/db

ulimit -c unlimited

echo "start node"
./topio node startNode -c ./config/config.rec1.json > /dev/null 2>&1 &
sleep 1
./topio node startNode -c ./config/config.rec2.json > /dev/null 2>&1 &
sleep 1
./topio node startNode -c ./config/config.rec3.json > /dev/null 2>&1 &
sleep 1
./topio node startNode -c ./config/config.rec4.json > /dev/null 2>&1 &
sleep 1
./topio node startNode -c ./config/config.rec5.json > /dev/null 2>&1 &
sleep 1
./topio node startNode -c ./config/config.rec6.json > /dev/null 2>&1 &
sleep 1

echo "finish"