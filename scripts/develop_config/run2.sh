
# rm -rf /cores/*
# rm -rf core.*

ulimit -c unlimited

sleep 2

# start munual rec
# rm -rf /tmp/rec1/*; ./xtopchain ./config/config.rec1.json

# start munual zec/adv/con
# rm -rf /tmp/zec1/*; ./xtopchain ./config/config.zec1.json
# rm -rf /tmp/adv1/*; ./xtopchain ./config/config.adv1.json
# rm -rf /tmp/con1/*; ./xtopchain ./config/config.con1.json

# rec
for ((i=1;i<=3;i++)); do
    mkdir -p /tmp/rec${i}
    rm -rf /tmp/rec${i}/*
    touch /tmp/rec${i}/xtop.log
    ./xtopchain ./config/config.rec${i}.json &
    if [ ${i} -eq 1 ]; then
        echo "first rec node starting ..."
        sleep 5
    else
        echo "other rec node starting ..."
        sleep 0.5
    fi
done

# zec
for ((i=1;i<=4;i++)); do
    mkdir -p /tmp/zec${i}
    rm -rf /tmp/zec${i}/*
    touch /tmp/zec${i}/xtop.log
    ./xtopchain ./config/config.zec${i}.json &
    sleep 0.5
done

# adv
for ((i=1;i<=4;i++)); do
    mkdir -p /tmp/adv${i}
    rm -rf /tmp/adv${i}/*
    touch /tmp/adv${i}/xtop.log
    ./xtopchain ./config/config.adv${i}.json &
    sleep 0.5
done

# con
for ((i=1;i<=12;i++)); do
    # if [ ${i} -eq 3 ]; then
    #     continue
    # fi
    # if [ ${i} -eq 2 ]; then
    #     continue
    # fi
    mkdir -p /tmp/con${i}
    rm -rf /tmp/con${i}/*
    touch /tmp/con${i}/xtop.log
    ./xtopchain ./config/config.con${i}.json &
    sleep 0.5
done

# arc
for ((i=1;i<=1;i++)); do
    mkdir -p /tmp/arc${i}
    rm -rf /tmp/arc${i}/*
    touch /tmp/arc${i}/xtop.log
    ./xtopchain ./config/config.arc${i}.json &
    sleep 0.5
done

# edge
mkdir -p /tmp/edge
rm -rf /tmp/edge/*
touch /tmp/edge/xtop.log
./xtopchain ./config/config.edge.json &
sleep 0.5
