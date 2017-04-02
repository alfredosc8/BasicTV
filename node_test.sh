#I can't test node connectivity without spinning up a few independent nodes
#This, when launched, should create a 4-node network that only (really) shares the peer information


#default settings
#run this in another terminal
#valgrind ./basictv --print_level 0 --throw_level 4 --audio false --video false --network_port 58486 &

#clearing not important directories

touch TEST_DATA_1
rm -r TEST_DATA_1

touch TEST_DATA_2
rm -r TEST_DATA_2

touch TEST_DATA_3
rm -r TEST_DATA_3

touch TEST_DATA_4
rm -r TEST_DATA_4

#since the directories are being cleared, we need to make more time for setting up each node because
#of possible slow private key initialization times (especially on Pi, desktop should be fine)

#valgrind ./basictv --print_level 0 --throw_level 4 --audio false --video false --network_port 58487 --console_port 59001 --data_folder TEST_DATA_1 --net_proto_custom_bootstrap_ip 127.0.0.1 --net_proto_custom_bootstrap_port 58486 &
#sleep 30
#valgrind ./basictv --print_level 0 --throw_level 4 --audio false --video false --network_port 58488 --console_port 59002 --data_folder TEST_DATA_2 --net_proto_custom_bootstrap_ip 127.0.0.1 --net_proto_custom_bootstrap_port 58486 &
#sleep 30
#valgrind ./basictv --print_level 0 --throw_level 4 --audio false --video false --network_port 58489 --console_port 59003 --data_folder TEST_DATA_3 --net_proto_custom_bootstrap_ip 127.0.0.1 --net_proto_custom_bootstrap_port 58486 &

#seems to be easier to do this inside of gnome-terminal than tmux

#tmux \
#  new-session  "valgrind ./basictv --print_level 0 --throw_level 4 --audio false --video false --network_port 58487 --console_port 59001 --data_folder TEST#_DATA_1 --net_proto_custom_bootstrap_ip 127.0.0.1 --net_proto_custom_bootstrap_port 58486
# ; read" \; \
#  split-window "valgrind ./basictv --print_level 0 --throw_level 4 --audio false --video false --network_port 58488 --console_port 59002 --data_folder TEST#_DATA_2 --net_proto_custom_bootstrap_ip 127.0.0.1 --net_proto_custom_bootstrap_port 58486 &
# ; read" \; \
#  split-window "valgrind ./basictv --print_level 0 --throw_level 4 --audio false --video false --network_port 58489 --console_port 59003 --data_folder TEST#_DATA_3 --net_proto_custom_bootstrap_ip 127.0.0.1 --net_proto_custom_bootstrap_port 58486 &
# ; read" \; \
#  split-window "valgrind ./basictv --print_level 0 --throw_level 4 --audio false --video false --network_port 58490 --console_port 59004 --data_folder TEST#_DATA_4 --net_proto_custom_bootstrap_ip 127.0.0.1 --net_proto_custom_bootstrap_port 58486 &
# ; read" \; \
#  select-layout even-vertical

gnome-terminal \
	       --window -e 'gdb --args ./basictv --print_level 0 --throw_level 4 --audio false --video false --network_port 58487 --console_port 59001 --data_folder TEST_DATA_1 --net_proto_custom_bootstrap_ip 127.0.0.1 --net_proto_custom_bootstrap_port 58486' \
	       --tab -e 'gdb --args ./basictv --print_level 0 --throw_level 4 --audio false --video false --network_port 58488 --console_port 59002 --data_folder TEST_DATA_2 --net_proto_custom_bootstrap_ip 127.0.0.1 --net_proto_custom_bootstrap_port 58486' \
	       --tab -e 'gdb --args ./basictv --print_level 0 --throw_level 4 --audio false --video false --network_port 58489 --console_port 59003 --data_folder TEST_DATA_3 --net_proto_custom_bootstrap_ip 127.0.0.1 --net_proto_custom_bootstrap_port 58486' \
	       --tab -e 'gdb --args ./basictv --print_level 0 --throw_level 4 --audio false --video false --network_port 58490 --console_port 59004 --data_folder TEST_DATA_4 --net_proto_custom_bootstrap_ip 127.0.0.1 --net_proto_custom_bootstrap_port 58486'
