#ifndef NET_INTERFACE_HARDWARE_H
#define NET_INTERFACE_HARDWARE_H

#include "../../id/id_set.h"

#include "net_interface_medium.h"
#include "net_interface_packet.h"

#define NET_INTERFACE_HARDWARE_ADD_ADDRESS_UNDEFINED 0
/*
  If we add an address to a hardware device, there is no cost associated with
  it, and we can add it freely
*/
#define NET_INTERFACE_HARDWARE_ADD_ADDRESS_FREE 1
/*
  If we add a address to a hardware device, there are software devices that
  need to be dropped for it to work
*/
#define NET_INTERFACE_HARDWARE_ADD_ADDRESS_DROP 2

#define NET_INTERFACE_TRANSPORT_FLAG_LOSSLESS (1 << 0)
#define NET_INTERFACE_TRANSPORT_FLAG_LOSSY (1 << 1)
#define NET_INTERFACE_TRANSPORT_FLAG_GUARANTEED (1 << 3)


struct net_interface_hardware_dev_t{
private:
	/*
	  Hard maximum on the number of software_dev_t's that can be bound
	  to this hardware device. This is NOT the same as a case by case
	  adding, where the variables at play need to be considered before
	  adding it (multiple frequencies inside of the same bandwidth).
	*/
	uint64_t max_soft_dev = 0;
	uint8_t outbound_transport_type = 0;
	uint8_t outbound_transport_flags = 0;
	uint8_t inbound_transport_type = 0;
	uint8_t inbound_transport_flags = 0;
	uint8_t medium = 0;
	
	std::vector<uint8_t> soft_dev_list;
	id_t_ inbound_throughput_number_set_id = ID_BLANK_ID;
	id_t_ outbound_throughput_number_set_id = ID_BLANK_ID;
public:
	data_id_t id;
	net_interface_hardware_dev_t();
	~net_interface_hardware_dev_t();
	GET_SET(max_soft_dev, uint64_t);
	GET_SET(outbound_transport_type, uint8_t);
	GET_SET(outbound_transport_flags, uint8_t);
	GET_SET(inbound_transport_type, uint8_t);
	GET_SET(inbound_transport_flags, uint8_t);
	GET_SET(medium, uint8_t);
	
	/* ADD_DEL_VECTOR(soft_dev_list, id_t_); */
	/* GET_SIZE_VECTOR(soft_dev_list); */
	GET_SET_ID_VECTOR(soft_dev_list);
};


#endif
