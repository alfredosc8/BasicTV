./clean.sh
#valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes
gdb --args ../basictv --net_port 58487 --console_port 59001 --data_folder TEST_DATA_1 --bootstrap_ip 127.0.0.1 --bootstrap_port 58486 --net_hostname 127.0.0.1 --print_level 1 --print_delay 100 --print_stats true
# --print_delay 250
