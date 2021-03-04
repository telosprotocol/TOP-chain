#!/usr/bin/env python
#-*- coding:utf8 -*-

import os
import socket

config_file = "./conf/demo.conf"

def mkconf():
    if not os.path.exists('./conf'):
        os.mkdir('./conf')
        os.mkdir('./log')
    return

def get_local_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        # doesn't even have to be reachable
        s.connect(('10.255.255.255', 1))
        IP = s.getsockname()[0]
    except:
        IP = '0.0.0.0'
    finally:
        s.close()
    return IP

def dump_field(name, fout = None, field = {}):
    if not field or not fout:
        return
    field_str = "[{0}]\n".format(name)
    fout.write(field_str)
    for (k,v) in field.items():
        line = "{0} = {1}\n".format(k, v)
        fout.write(line)
    fout.write("\n")


def update_config():
    local_ip = get_local_ip()
    log = {
            "path": "./log/bitvpn.log",
            "off": "false",
            "debug": "true",
            }

    node = {
            "first_node": "false",
            "local_ip": local_ip,
            "local_port": 10000,
            "client_mode": "false",
            "country": "CN",
            "public_endpoints": "165.22.51.192:10000,165.22.51.190:10000,159.89.202.159:10000,165.22.51.212:10000",
            "show_cmd": "true",
            }

    db = {
            "path": "./db",
            }

    elect = {
            }

    redis = {
            "ip" : "192.168.50.242",
            "port": 9102,
            }

    with open(config_file, 'w') as fout:
        dump_field("log", fout, log)
        dump_field("node", fout, node)
        dump_field("db", fout, db)
        dump_field("redis", fout, redis)
        fout.close()



if __name__ == "__main__":
    mkconf()
    update_config()
