#ifndef NET_INTERFACE_INTERMEDIARY_H
#define NET_INTERFACE_INTERMEDIARY_H


#define NET_INTERFACE_INTERMEDIARY_UNDEFINED 0
#define NET_INTERFACE_INTERMEDIARY_NONE 1
#define NET_INTERFACE_INTERMEDIARY_TOR 2
#define NET_INTERFACE_INTERMEDIARY_I2P 3


struct net_interface_intermediary_t{
private:
	id_t_ intermediary_address = ID_BLANK_ID;
	std::vector<id_t_> listed_soft_dev;

	uint8_t intermediary = 0;
	
	id_t_ inbound_throughput_number_set_id = ID_BLANK_ID;
	id_t_ outbound_throughput_number_set_id = ID_BLANK_ID;
public:
	data_id_t id;
	net_interface_intermediary_t();
	~net_interface_intermediary_t();
	GET_SET_ID(intermediary_address);
	void list_soft_dev(id_t_ soft_dev_id_);
	void unlist_soft_dev(id_t_ soft_dev_id_);
	GET(listed_soft_dev, std::vector<id_t_>);
	GET_ID(inbound_throughput_number_set_id);
	GET_ID(outbound_throughput_number_set_id);
	GET_SET(intermediary, uint8_t);
};

#endif
