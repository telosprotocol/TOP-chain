#!/bin/bash

username=`whoami`
if [ $username = "root" ]
then
    topio_home=/$username/topnetwork
else
    topio_home=/home/$username/topnetwork
fi

if [ -z ${TOPIO_HOME} ];then
sed -i "/TOPIO_HOME/d"     ~/.bashrc
sed -i '$a\export TOPIO_HOME='$topio_home        ~/.bashrc
source ~/.bashrc
else
echo $TOPIO_HOME
fi