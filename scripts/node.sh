touch TEST_DATA_1
rm -r TEST_DATA_1 TEST_1_OUTPUT
gdb --args ../basictv --print_backtrace true --net_interface_ip_tcp_port 58487 --console_port 59001 --data_folder TEST_DATA_1 --net_proto_ip_tcp_bootstrap_ip 127.0.0.1 --net_proto_ip_tcp_bootstrap_port 58486 --print_color false --print_level 0 | tee TEST_1_OUTPUT
