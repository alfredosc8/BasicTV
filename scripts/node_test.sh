#I can't test node connectivity without spinning up a few independent nodes
#This, when launched, should create a 4-node network that only (really) shares the peer information

touch TEST_DATA_1
rm -r TEST_DATA_1

touch TEST_DATA_2
rm -r TEST_DATA_2

touch TEST_DATA_3
rm -r TEST_DATA_3

touch TEST_DATA_4
rm -r TEST_DATA_4

gnome-terminal \
	       --window -e 'gdb --args ../basictv --network_port 58487 --console_port 59001 --data_folder TEST_DATA_1 --bootstrap_ip 127.0.0.1 --bootstrap_port 58486' \
	       --tab -e 'gdb --args ../basictv --network_port 58488 --console_port 59002 --data_folder TEST_DATA_2 --bootstrap_ip 127.0.0.1 --bootstrap_port 58486' \
	       --tab -e 'gdb --args ../basictv --network_port 58489 --console_port 59003 --data_folder TEST_DATA_3 --bootstrap_ip 127.0.0.1 --bootstrap_port 58486' \
	       --tab -e 'gdb --args ../basictv --network_port 58490 --console_port 59004 --data_folder TEST_DATA_4 --bootstrap_ip 127.0.0.1 --bootstrap_port 58486'
