#!/usr/bin/python3

import time, sys, datetime

def beijing_to_utc_timestamp(beijing_time):
    # 将输入的字符串转为datetime对象
    dt = datetime.datetime.strptime(beijing_time, '%Y-%m-%d %H:%M:%S')
    # 设置时区为北京时间
    local_tz = datetime.timezone(datetime.timedelta(hours=8))
    dt = dt.replace(tzinfo=local_tz)
    # 转换为UTC时间
    utc_dt = dt.astimezone(datetime.timezone.utc)
    # 返回UTC时间戳
    return int(utc_dt.timestamp())

def utc_timestamp_to_beijing(utc_timestamp):
    # 将UTC时间戳转换成本地时间（北京时间）的datetime对象
    beijing_time = datetime.datetime.fromtimestamp(utc_timestamp) + datetime.timedelta(hours=8)
    # 将datetime对象格式化成字符串
    beijing_time_str = beijing_time.strftime('%Y-%m-%d %H:%M:%S')
    return beijing_time_str

def utc_to_top_clock(utc_timestamp):
    return (utc_timestamp - 1573189200) // 10

def top_clock_to_utc(top_clock):
    return top_clock * 10 + 1573189200

def beijing_to_clock(beijing_time):
    utc_timestamp = beijing_to_utc_timestamp(beijing_time)
    print(utc_to_top_clock(utc_timestamp))

def clock_to_beijing(clock):
    utc_timestamp = top_clock_to_utc(clock)
    print(utc_timestamp_to_beijing(utc_timestamp))

def is_beijing_time(input_str):
    try:
        time.strptime(input_str, '%Y-%m-%d %H:%M:%S')
        return True
    except ValueError:
        return False

def is_clock(input_str):
    try:
        int(input_str)
        return True
    except ValueError:
        return False

def parse_time(input_str):
    if is_beijing_time(input_str):
        return beijing_to_clock(input_str)

    if is_clock(input_str):
        return clock_to_beijing(int(input_str))
    
    raise ValueError('Unable to parse input as either Top Clock like 10608840 or Beijing time like "2023-03-20 10:00:00"')

def main(argv):
    if len(argv) != 1:
        print("no input")
        sys.exit(-1)
    parse_time(argv[0])

if __name__ == "__main__":
    main(sys.argv[1:])
