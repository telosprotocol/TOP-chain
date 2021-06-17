#!/usr/bin/python3

import sys, getopt, os, os.path, re

def main(argv):
    try:
        opts, _ = getopt.getopt(argv, "f:")
    except getopt.GetoptError:
        print(sys.argv[0], ' -f <input file>')
        sys.exit()

    pattern = re.compile("#\d+ (0x[0-9a-fA-F]+) \(.+xtopchain\+0x[0-9a-fA-F]+\)")
    f = open(opts[0][1], "r")
    for line in f.readlines():
        line = line.rstrip()
        m = pattern.search(line)
        if m:
            os.system("addr2line -e xtopchain " + m.group(1))
        else:
            print(line)

if __name__ == "__main__":
    main(sys.argv[1:])
