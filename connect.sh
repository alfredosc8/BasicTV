./clean.sh
rm -r scripts/TEST_DIR_1
gdb --args ./basictv --print_level 0 --print_backtrace true --console_port 59001 --net_interface_ip_hostname 127.0.0.1 --net_interface_ip_tcp_port 58487 --net_proto_ip_tcp_bootstrap_ip 96.35.0.163 --net_proto_ip_tcp_bootstrap_port 58486 --data_folder scripts/TEST_DIR_1 | tee output_connect

