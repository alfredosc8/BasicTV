#I can't test node connectivity without spinning up a few independent nodes
#This, when launched, should create a 4-node network that only (really) shares the peer information


#default settings
valgrind ./basictv --print_level 2 --throw_level 4 --audio false --video false --network_port 58486 &

#clearing not important directories

touch TEST_DATA_1
rm -r TEST_DATA_1

touch TEST_DATA_2
rm -r TEST_DATA_2

touch TEST_DATA_3
rm -r TEST_DATA_3

#since the directories are being cleared, we need to make more time for setting up each node because
#of possible slow private key initialization times (especially on Pi, desktop should be fine)

sleep 5
valgrind ./basictv --print_level 2 --throw_level 4 --audio false --video false --network_port 58487 --console_port 59001 --data_folder TEST_DATA_1 --net_proto_custom_bootstrap_ip 127.0.0.1 --net_proto_custom_bootstrap_port 58486 &
sleep 5
valgrind ./basictv --print_level 2 --throw_level 4 --audio false --video false --network_port 58487 --console_port 59002 --data_folder TEST_DATA_2 --net_proto_custom_bootstrap_ip 127.0.0.1 --net_proto_custom_bootstrap_port 58486 &
sleep 5
valgrind ./basictv --print_level 2 --throw_level 4 --audio false --video false --network_port 58487 --console_port 59003 --data_folder TEST_DATA_3 --net_proto_custom_bootstrap_ip 127.0.0.1 --net_proto_custom_bootstrap_port 58486 &
