#ifndef NET_INTERFACE_MEDIUM_H
#define NET_INTERFACE_MEDIUM_H

#define NET_INTERFACE_MEDIUM_UNDEFINED 0
#define NET_INTERFACE_MEDIUM_IP 1
#define NET_INTERFACE_MEDIUM_RADIO 2

// some macros for standard naming
#define INTERFACE_ADD_ADDRESS_COST(interface) uint8_t net_interface_##interface##_add_address_cost(id_t_ hardware_dev_id, id_t_ address_id)
#define INTERFACE_ADD_ADDRESS(interface) id_t_ net_interface_##interface##_add_address(id_t_ hardware_dev_id, id_t_ address_id)
#define INTERFACE_CALCULATE_MOST_EFFICIENT_DROP(interface) std::vector<id_t_> net_interface_##interface##_calculate_most_efficient_drop(id_t_ hardware_dev_id, id_t_ address_id)
#define INTERFACE_CALCULATE_MOST_EFFICIENT_TRANSFER(interface) std::vector<std::pair<id_t_, id_t_> > net_interface_##interface##_calculate_most_efficient_transfer(id_t_ hardware_dev_id, id_t_ address_id)

#define INTERFACE_SEND(interface) void net_interface_##interface##_send(id_t_ hardware_dev_id, id_t_ software_dev_id, std::vector<uint8_t> *payload)
#define INTERFACE_RECV_ALL(interface) void net_interface_##interface##_recv_all(id_t_ hardware_dev_id, id_t_ software_dev_id)

#include "../../util.h"
#include "../../id/id_api.h"

struct net_interface_medium_t{
public:
	uint8_t (*add_address_cost)(
		id_t_ hardware_dev_id,
		id_t_ address_id) = nullptr;

	id_t_ (*add_address)(
		id_t_ hardware_dev_id,
		id_t_ address_id) = nullptr;

	// calls to _drop and _transfer are based on some flags that
	// tell if we can transfer the information

	/*
	  Given a set of net_interface_software_dev_t IDs and
	  a desired net_interface_*_address_t, find the most
	  efficient configuration that includes the new 
	  address ID, and return the IDs that are to be 
	  dropped.
	*/
	std::vector<id_t_> (*calculate_most_efficient_drop)(
		id_t_ hardware_dev_id,
		id_t_ address_id) = nullptr;
	/*
	  Instead of dropping the connections entirely, this gives the option of
	  allowing transferring software devices over to another hardware
	  device.

	  This is important for multiple radio receivers and possibly
	  for UDP if lines are becoming saturated, but TCP wouldn't work, as
	  the state cannot be preserved. Attempts to transfer TCP sockets
	  from one hardware device to another would just return an empty vector,
	  (depending on how badly we need to socket, the calling code can
	  call perform_drop, under the assumption that a blank transfer_vector
	  doesn't mean nothing can be freed, but instead that nothing can be
	  preserved).

	  Returns a vector of pairs, first element being the hardware device
	  recommended to transfer towards, and the second element being the
	  software_dev in question.
	*/
	std::vector<std::pair<id_t_, id_t_> > (*calculate_most_efficient_transfer)(
		id_t_ hardware_dev_id,
		id_t_ address_id) = nullptr;

	void (*send)(id_t_ hardware_dev_id,
		     id_t_ software_dev_id,
		     std::vector<uint8_t> *payload) = nullptr;
	void (*recv_all)(
		id_t_ hardware_dev_id,
		id_t_ software_dev_id) = nullptr;
	
	net_interface_medium_t(
		uint8_t (*add_address_cost_)(id_t_ hardware_dev_id, id_t_ address_id),
		id_t_ (*add_address_)(id_t_ hardware_dev_id, id_t_ addres_id),
		std::vector<id_t_> (*calculate_most_efficient_drop_)(id_t_ hardware_dev_id, id_t_ address_id),
		std::vector<std::pair<id_t_, id_t_> > (*calculate_most_efficient_transfer_)(id_t_ hardware_dev_id, id_t_ address_id),
		void (*send_)(id_t_, id_t_, std::vector<uint8_t> *payload),
		void (*recv_all_)(id_t_ hardware_dev_id, id_t_ software_dev_id)){

		add_address_cost = add_address_cost_;
		add_address = add_address_;
		calculate_most_efficient_drop = calculate_most_efficient_drop_;
		calculate_most_efficient_transfer = calculate_most_efficient_transfer_;
		send = send_;
		recv_all = recv_all_;
	}
};

#endif
