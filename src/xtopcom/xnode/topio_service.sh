#!/usr/bin/env bash

HELLO="
Welcome to TOPIO_SERVICE module

This script used to register topio as service so that topio will load when run this machine.

Befor run this script, Please make sure you know what you are doing.
Befor run this script, Please set \$TOPIO_HOME as  topio work dir.

Attention: If you do not know what this is, please don't run this script
Attention: If you do not know what this is, please don't run this script
Attention: If you do not know what this is, please don't run this script
"

printf '%s\n' "$HELLO "


echo "this script used for systemctl service, are your sure to register topio as  service?"


read -n1 -p "Do you want to register topio as service [Y/N]?" register
case $register in
    Y | y)
        echo "";;
    N | n)
        echo ""
        echo "Ok, Stop register topio.service"
        exit 0;;
    *)
        echo ""
        echo "error choice, exit"
        exit -1;;
esac

register="n"

read -n1 -p "[Recheck]Do you want to register topio as service [Y/N]?" register
case $register in
    Y | y)
        echo ""
        echo "fine, hope you know what you are doing";;
    N | n)
        echo ""
        echo "Ok, Stop register topio.service"
        exit 0;;
    *)
        echo ""
        echo "error choice, exit"
        exit -1;;
esac

echo ""
read -p "Please input user which will run topio:" username
if [ -z "$username" ]
then
    echo "Please input right username"
    echo "exit"
    exit -1
fi

id -u $username
if [ $? -eq 1 ]
then
    echo "exit"
    exit -1
fi

if [ $username != "root" ]; then
    current_user=`id -u`
    if [ $current_user -eq 0 ]
    then
        echo "Please do not run this script with root(sudo) mode"
        exit -1
    fi
fi


echo "topio will run in user:$username mode"


echo ""
topio_home=`echo $TOPIO_HOME`
if [ -z "$topio_home" ]
then
    echo "Please set environment var \$TOPIO_HOME first"
    exit -1
else
    echo "use $topio_home as topio work dir."
fi

read -n1 -p "Are you sure to use $topio_home as topio work dir[Y/N]?" use_topio_home
case $use_topio_home in
    Y | y)
        echo ""
        echo "fine, hope you know what you are doing";;
    N | n)
        echo ""
        echo "Ok, Stop register topio.service"
        exit 0;;
    *)
        echo ""
        echo "error choice, exit"
        exit -1;;
esac

TOPIO_SAFEBOX_SERVICE="
[Unit]
Description=the cpp-topnetwork command line interface
After=network.target
After=network-online.target
Wants=network-online.target

[Service]
Type=forking
Environment=TOPIO_HOME=$topio_home
PIDFile=$topio_home/safebox.pid
ExecStart=/usr/bin/topio node safebox
PrivateTmp=true

[Install]
WantedBy=multi-user.target"

printf '%s\n' "$TOPIO_SAFEBOX_SERVICE"
printf '%s\n' "$TOPIO_SAFEBOX_SERVICE" | sudo tee /lib/systemd/system/topio-safebox.service
sudo systemctl enable topio-safebox.service


TOPIO_SERVICE="
[Unit]
Description=the cpp-topnetwork command line interface
After=network.target
After=network-online.target
Wants=network-online.target

[Service]
Type=forking
User=$username
Group=$username
Environment=TOPIO_HOME=$topio_home
PIDFile=$topio_home/topio.pid
ExecStart=/usr/bin/topio node startnode
ExecStop=/usr/bin/topio node stopnode
PrivateTmp=true

[Install]
WantedBy=multi-user.target
"

printf '%s\n' "$TOPIO_SERVICE"
printf '%s\n' "$TOPIO_SERVICE" | sudo tee /lib/systemd/system/topio.service

sudo systemctl enable topio.service
sudo systemctl  daemon-reload

echo ""
echo "install topio.service ok"
echo "now you can use systemctl start/status/stop topio"
echo "good luck!"
