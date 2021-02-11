#!/usr/bin/python3

import sys, getopt

def main(argv):
    try:
        opts, _ = getopt.getopt(argv, "h:l:")
    except getopt.GetoptError:
        print(sys.argv[0], ' -h <XIP2.high_addr> -l <XIP2.low_addr>')
        sys.exit()

    xip_high = 0xFFFFFFFFFFFFFFFF
    xip_low = 0xFFFFFFFFFFFFFFFF

    for opt, arg in opts:
        if opt == '-h':
            xip_high = int(arg, 16)
        elif opt == '-l':
            xip_low = int(arg, 16)

    high_binary_str = "{:064b}".format(xip_high)
    low_binary_str = "{:064b}".format(xip_low)

    if len(high_binary_str) > 64:
        print('XIP high too large')
        sys.exit()
    if len(low_binary_str) > 64:
        print('XIP low too large')
        sys.exit()

    print("XIP low:")
    address_domain = int(low_binary_str[0:1], 2)
    if address_domain == 0:
        print('\taddress domain: XIP')
    else:
        print('\taddress domain: XIP2')

    address_type = int(low_binary_str[1:3], 2)
    if address_type == 0:
        print('\taddress type: dynamic')
    elif address_type == 1:
        print('\taddress type: multicast')
    elif address_type == 2:
        print('\taddress type: nat')
    else:
        print('\taddress type: static')

    network_type = int(low_binary_str[3:8], 2)
    if network_type == 22:
        print('\tnetwork type: chain')
    else:
        print('\tnetwork type: unknown')

    network_version = int(low_binary_str[8:11], 2)
    print('\tnetwork version: {0}'.format(network_version))

    network_id = int(low_binary_str[11:32], 2)
    if network_id == 0b111111111111111111111:
        network_id = 'broadcast'

    zone_id = int(low_binary_str[32:39], 2)
    if zone_id == 0b1111111:
        zone_id = 'broadcast'

    cluster_id = int(low_binary_str[39:46], 2)
    if cluster_id == 0b1111111:
        cluster_id = 'broadcast'

    group_id = int(low_binary_str[46:54], 2)
    if group_id == 0b11111111:
        group_id = 'broadcast'

    slot_id = int(low_binary_str[54:64], 2)
    if slot_id == 0b1111111111:
        slot_id = 'broadcast'

    print('\tnetwork id: {0}\n\tzone id: {1}\n\tcluster id: {2}\n\tgroup id: {3}\n\tslot id: {4}'.format(network_id, zone_id, cluster_id, group_id, slot_id))

    if zone_id == 0:
        if cluster_id == 1:
            if 1 <= group_id < 64:
                print("\taddress type: auditor")
            elif 64 <= group_id < 127:
                print("\taddress type: validator")
        else:
            print("\taddress type: unknown cluster id")
    elif zone_id == 1:
        if cluster_id == 0:
            if group_id == 0:
                print("\taddress type: committee")
            else:
                print("\taddress type: unknown group id")
        else:
            print("\taddress type: unknown cluster id")
    elif zone_id == 2:
        if cluster_id == 0:
            if group_id == 0:
                print("\taddress type: zec")
            else:
                print("\taddress type: unknown group id")
        else:
            print("\taddress type: unknown cluster id")
    elif zone_id == 3:
        print("\taddress type: frozen")
    elif zone_id == 14:
        print("\taddress type: archive")
    elif zone_id == 15:
        print("\taddress type: edge")
    else:
        print("\taddress type: unknown zone id")

    print("\nXIP high:")
    group_size = int(high_binary_str[0:10], 2)
    print('\tgroup size: {0}'.format(group_size))

    blk_height = int(high_binary_str[10:64], 2)
    print('\tblk height: {0}'.format(blk_height))

if __name__ == "__main__":
    main(sys.argv[1:])
