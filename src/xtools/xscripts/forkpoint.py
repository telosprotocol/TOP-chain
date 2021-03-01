#!/usr/bin/python3

import time, sys

def main(argv):
    if len(argv) != 1:
        print("no input")
        sys.exit(-1)
    print(((int(time.mktime(time.strptime(argv[0],'%Y-%m-%d %H:%M:%S')))) - 1573189200) // 10)

if __name__ == "__main__":
    main(sys.argv[1:])
