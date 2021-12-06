#pragma once

#include <string>

const static std::string str_file = 
R"(
[log]
debug = true
path = ./xelect_test.log
off = false

[node]
show_cmd = false
local_port = 9000
country = CN
first_node = false
local_ip = 127.0.0.1
node_id = DEMOEXCHANGEACCOUNT!DEMOEXCHANGEACCOUNT!
public_endpoints = 127.0.0.1:9000,127.0.0.1:9001,127.0.0.1:9002,127.0.0.1:9003

[db]
path = ./xelect_tests_db

)";
