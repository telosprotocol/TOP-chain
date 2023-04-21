#!/bin/bash

echo "decode service type $1"

# [ver:1]-[network_id:20]-[zone_id:7]-[cluster_id:7]-[group_id:8]-[height:21]
printf "\tversion:    $((($1 >> 63) & 0x01))\n"

printf "\tnetwork id: $((($1 >> 43) & 0x0FFFFF))\n"

zone_id=$((($1 >> 36) & 0x7F))
printf "\tzone id:    $zone_id"
[ $zone_id -eq 0 ] && printf "\tconsensus\n"
[ $zone_id -eq 1 ] && printf "\trec\n"
[ $zone_id -eq 2 ] && printf "\tzec\n"
[ $zone_id -eq 3 ] && printf "\tfrozen\n"
[ $zone_id -eq 4 ] && printf "\tevm\n"
[ $zone_id -eq 14 ] && printf "\tstorage\n"
[ $zone_id -eq 15 ] && printf "\tedge\n"

cluster_id=$((($1 >> 29) & 0x7F))
printf "\tcluster id: $cluster_id"
[ $zone_id -eq 14 ] && [ $cluster_id -eq 1 ] && printf "\tarchive\n"
[ $zone_id -eq 14 ] && [ $cluster_id -eq 2 ] && printf "\texchange\n"

group_id=$((($1 >> 21) & 0xFF))
printf "\tgroup id:   $group_id\n"

printf "\theight:     $(($1 & 0x00000000001FFFFF))\n"
