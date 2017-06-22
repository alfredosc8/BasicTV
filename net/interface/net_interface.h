#ifndef NET_INTERFACE_H
#define NET_INTERFACE_H

#include "../../id/id.h"
#include "../../id/id_api.h"
#include "../../util.h"
#include "../../state.h"

#include <algorithm>

/*
  GENERIC INTERFACE FOR ALL NETWORKING

  Here is a breakdown of the setups and types:

  net_interface_hardware_dev_t:
  One is created for each physical connection to the outside world. The
  following devices are planned to be used:
  - Radio transmitter
  - Radio receiver
  - Ethernet/WiFi connection (Linux networking interfaces)

  net_interface_hardware_dev_t contains limits on the amount and types
  of net_interface_software_dev_t that can be bound to it. Limiting
  TCP connections, limiting radio receiving to one frequency, and
  what not.

  The only item interfacing with net_interface_hardware_dev_t is
  net_interface_software_dev_t, and net_interface_hardware_dev_t keeps
  tabs on all connections going to a physical device.

  Because of simplicity with implementations, net_interface_hardware_dev_t
  doesn't actually communicate with the device. net_interface_software_dev_t
  are all communicating and negotiating with one another, with
  net_interface_hardware_dev_t being a catalogue and information directory.


  net_interface_software_dev_t:
  net_interface_software_dev_t is responsible for actual communications
  in software with the device in question. net_interface_software_dev_t
  is effectively net_socket_t, but as a more abstract system. Whereas
  with TCP/IP or UDP/IP, the decoding is offloaded to the Linux kernel
  (via POSIX), packet radio might not have this sort of treatment
  (especially on Windows), so net_interface_software_dev_t might become
  a bit large with the addition of modems.

  net_proto_socket_t contains net_interface_software_dev_t, and handles
  loading chunks of information from net_interface_software_dev_t into memory,
  that falls outside of the scope of this file (see net/proto/net_proto_socket.h
  for information about that).
*/

/*
  TRANSPORT TYPE
  NET_TRANSPORT_UNDEFINED is just undefined, simple enough

  NET_TRANSPORT_DISABLED means that the net_interface_harware_t cannot
  handle data in that direction (inbound/outbound_transport_type)

  NET_PROTO_SWITCH_OVERHEAD means that there is an overhead and some
  custom stipulations with allowing more sockets. My RTL-SDR has a
  1MHz bandwidth, and multiple connections can be made if they all
  fall within that 1MHz band.

  NET_PROTO_SWITCH_FREE means that we are free to use this device with
  multiple net_interface_software_dev_t without any problems, switching
  overheads, or stipulations. However, the max_soft_dev limit is always
  observed and followed (but we can set it arbitrarially high).

  TRANSPORT FLAGS
  NET_TRANSPORT_FLAG_LOSSLESS means that there is a protocol error
  detection and correction implemented, and that any data that is received
  can be safely assumed to be correct

  NET_TRANSPORT_FLAG_LOSSY means that datagrams can be lost, and we cannot
  assume a direct order between the sending and the receiving (UDP)

  NET_TRANSPORT_GUARANTEED means that any datagrams received by 
  net_interface_hardware_dev_t can be considered to be correct, and there
  is no need for a programmed protocol implementation of error correcting
  inside of net_interface_software_dev_t. TCP and UDP packets are either
  correct or non-existant (non-existance is handled via LOSSY and LOSSLESS).
  This is used more in FSK and multiple modulation schemes, which would
  be defined as one layer up in net_interface_software_dev_t. 
*/

/*
  
 */

#define NET_INTERFACE_HARDWARE_ADD_ADDRESS_UNDEFINED 0
/*
  If we add a net_interface_software_dev_t to a hardware device, there
  is no cost associated with it, and we can add it freely
*/
#define NET_INTERFACE_HARDWARE_ADD_ADDRESS_FREE 1
/*
  If we add a net_interface_software_dev_t to a hardware device, there
  are software devices that need to be dropped for it to work
*/
#define NET_INTERFACE_HARDWARE_ADD_ADDRESS_DROP 2

#define NET_TRANSPORT_FLAG_LOSSLESS (1 << 0)
#define NET_TRANSPORT_FLAG_LOSSY (1 << 1)
#define NET_TRANSPORT_FLAG_GUARANTEED (1 << 3)

/*
  The mediums shouldn't include information about the encoding scheme at all,
  since we are on a lower level here
*/
#define NET_INTERFACE_MEDIUM_UNDEFINED 0
#define NET_INTERFACE_MEDIUM_IP 1
/* #define NET_INTERFACE_MEDIUM_RADIO 2 */

/*
  Any information that needs to be abstracted out should
  be pushed off onto net_proto_peer_t. net_interface_*_address_t
  should only contain information pertaining to the medium
  (frequencies, bandwidth, modulation schemes for radio, ip
  address, port, and protocol
*/

#pragma message("times for availability should be set with a conversion function from HH:MM:SS to offsets from midnight UTC")

struct net_interface_address_t{
private:
	/*
	  If the system in question has a recurring availability:
	  - Some setting that only allows data to be freely forwarded and 
	  accepting connections at certain times (forwarding at night only)
	  - Time slots for radio transmissions and what not, helps increase
	  efficiency for one way broadcasting from high powered towers

	  The times here are not absolute times, but are offsets from midnight UTC.
	  If the broadcasting time runs through midnight
	*/
	uint64_t first_time_micro_s = 0; // actual timestamp
	uint64_t end_to_start_micro_s = 0; // time between each end and start
	uint64_t start_available_micro_s = 0;
       	uint64_t end_available_micro_s = 0;
	/*
	  IP users shouldn't need to fill in latitude and longitude information,
	  but radio users could benefit from including the coordinates of the
	  transmitter to allow for efficient use of directional antennae

	  One REALLY cool thing I want to do is allow for defining numbers as
	  functions over time, and computing them on the fly when the number is
	  decoded (while somehow passing that information back to the caller).
	  That could allow for much easier decoding from satellites and
	  transmitters in motion.
	*/
	std::vector<uint8_t> latitude;
	std::vector<uint8_t> longitude;

	/*
	  Medium of transfer
	*/
	uint8_t medium = 0;
public:
	void list_virtual_data(data_id_t *id);
	GET_SET(first_time_micro_s, uint64_t);
	GET_SET(end_to_start_micro_s, uint64_t);
	GET_SET(start_available_micro_s, uint64_t);
	GET_SET(end_available_micro_s, uint64_t);

	GET_SET(latitude, std::vector<uint8_t>);
	GET_SET(longitude, std::vector<uint8_t>);

	GET_SET(medium, uint8_t);
};

#define NET_INTERFACE_RADIO_MODULATION_UNDEFINED 0
#define NET_INTERFACE_RADIO_MODULATION_BELL_202 1
#define NET_INTERFACE_RADIO_MODULATION_G3RUH_DFSK 2
#define NET_INTERFACE_RADIO_MODULATION_BELL_103 3

#define NET_INTERFACE_RADIO_PACKET_UNDEFINED 0
#define NET_INTERFACE_RADIO_PACKET_AX_25 1
#define NET_INTERFACE_RADIO_PACKET_FX_25 2 // AX.25 with FEC

#define NET_INTERFACE_IP_PACKET_UNDEFINED 0
#define NET_INTERFACE_IP_PACKET_TCP 1
#define NET_INTERFACE_IP_PACKET_UDP 2

struct net_interface_radio_address_t : public net_interface_address_t {
private:
	// uses Hertz SI unit
	std::vector<uint8_t> freq;
	std::vector<uint8_t> bandwidth;
	uint64_t baud = 0;
	uint16_t modulation_scheme = 0;
	uint16_t packet_scheme = 0;
public:
	data_id_t id;
	net_interface_radio_address_t();
	~net_interface_radio_address_t();
	GET_SET(freq, std::vector<uint8_t>);
	GET_SET(bandwidth, std::vector<uint8_t>);
	void set_all_modulation(
		uint64_t baud_,
		uint16_t modulation_scheme_,
		uint16_t packet_scheme_);
	GET(baud, uint64_t);
	GET(modulation_scheme, uint16_t);
	GET(packet_scheme, uint16_t);
};

/*
  "ip_address" just means this can be resolved in some fahsion, it doesn't
  strictly require an IPv4 or IPv6 address (domain names, onion URLs, and I2P
  URLs are allowed as well)
*/

#define NET_INTERFACE_IP_ADDRESS_TYPE_UNDEFINED 0
#define NET_INTERFACE_IP_ADDRESS_TYPE_IPV4 1
#define NET_INTERFACE_IP_ADDRESS_TYPE_IPV6 2
#define NET_INTERFACE_IP_ADDRESS_TYPE_DOMAIN 3

#define NET_INTERFACE_IP_ADDRESS_REQUIRED_INTERMEDIARY_UNDEFINED 0
#define NET_INTERFACE_IP_ADDRESS_REQUIRED_INTERMEDIARY_NONE 1
#define NET_INTERFACE_IP_ADDRESS_REQUIRED_INTERMEDIARY_TOR 2
#define NET_INTERFACE_IP_ADDRESS_REQUIRED_INTERMEDIARY_I2P 3

struct net_interface_ip_address_t : public net_interface_address_t {
private:
	// IPv4 is stored as four bytes in NBO
	// IPv6 is stored as sixteen bytes in NBO
	// Domain names are stored in system byte order
	std::vector<uint8_t> address;
	uint8_t address_type = 0;
	uint8_t required_intermediary = 0;
public:
	data_id_t id;
	net_interface_ip_address_t();
	~net_interface_ip_address_t();
	GET(required_intermediary, uint8_t);
};

/*
  An intermediary is a proxy. Intermediares can be defined as static
  proxies inside of a settings file, but that isn't particularly 
  interesting.

  If Tor and I2P integration goes as planned, then multiple
  net_interface_intermediary_ts will be created with a flag denoting it
  as an automatically created type. Having automatically created types is
  pretty sick for a couple reasons:
  1. Spinning up mulitple Tor circuits/I2P tunnels, allowing us to increase
  the total throughput of the node by distributing the bandwidth (also gives
  us plenty of redundancy in case connections ping out)
  2. Creating and destroying Tor circuits/I2P tunnels until one is found that
  fits a set of criteria (bandwidth, ping time, country code, etc.)
  3. Better manage servicing the I2P network, as BasicTV nodes operating inside
  the I2P network only have access to the outside world through BasicTV
  nodes running both I2P and Tor circuits, so as demand for items increase
  inside of the I2P network, spin up more I2P and non-I2P connections to 
  facilitate transfering
  4. Unique and permanent domain names can be created inside of these networks,
  associate these with the net_interface_ip_address_t type, allowing for
  a more secure connection (as connections between peers on the Tor network
  don't leave the Tor network) and a more persistent IP configuration (IP
  address changes would result in non-preference).

  Since a standard abstraction system is the goal, technically it is possible
  to connect an intermediary to a software device using a radio, but there is
  no current, planned, or even visible use case for that

  An intermediary is one connection, there will be larger drivers at play
  later on that will facilitate creating and destroying intermediaries that
  have a more advanced implementation (Tor and I2P)
*/

struct net_interface_intermediary_t{
private:
	id_t_ intermediary_address = ID_BLANK_ID;
	std::vector<id_t_> listed_soft_dev;

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
};

/*
  Any and all calls specific to the medium at play (namely
  modulation and demodulation) is handled inside of 
  net_interface_medium_t
*/

// some macros for standard naming
#define INTERFACE_ADD_ADDRESS_COST(interface) uint8_t net_interface_##interface##_add_address_cost(id_t_ hardware_dev_id, id_t_ address_id)
#define INTERFACE_CALCULATE_MOST_EFFICIENT_DROP(interface) std::vector<id_t_> net_interface_##interface##_calculate_most_efficient_drop(id_t_ hardware_dev_id, id_t_ address_id)
#define INTERFACE_CALCULATE_MOST_EFFICIENT_TRANSFER(interface) std::vector<std::pair<id_t_, id_t_> > net_interface_##interface##_calculate_most_efficient_transfer(id_t_ hardware_dev_id, id_t_ address_id)

#define INTERFACE_SEND(interface) void net_interface_##interface##_send(id_t_ hardware_dev_id, id_t_ software_dev_id, std::vector<uint8_t> payload)
#define INTERFACE_RECV_ALL(interface) std::vector<uint8_t> net_interface_##interface##_recv_all(id_t_ hardware_dev_id, id_t_ software_dev_id)


struct net_interface_medium_t{
public:
	uint8_t (*add_address_cost)(
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
		     std::vector<uint8_t> payload) = nullptr;
	std::vector<uint8_t> (*recv_all)(
		id_t_ hardware_dev_id,
		id_t_ software_dev_id) = nullptr;
	
	void (*bind_software_dev)(id_t_ address_id, id_t_ software_dev_id);
	void (*unbind_software_dev)(id_t_ address_id, id_t_ software_dev_id);
	net_interface_medium_t(
		uint8_t (*add_address_cost_)(id_t_ hardware_dev_id, id_t_ address_id),
		
		std::vector<id_t_> (*calculate_most_efficient_drop_)(id_t_ hardware_dev_id, id_t_ address_id),
		std::vector<std::pair<id_t_, id_t_> > (*calculate_most_efficient_transfer_)(id_t_ hardware_dev_id, id_t_ address_id),
		
		void (*send_)(id_t_, id_t_, std::vector<uint8_t> payload),
		std::vector<uint8_t> (*recv_all_)(id_t_ hardware_dev_id, id_t_ software_dev_id)){
			
		add_address_cost = add_address_cost_;
		
		calculate_most_efficient_drop = calculate_most_efficient_drop_;
		calculate_most_efficient_transfer = calculate_most_efficient_transfer_;
				
		send = send_;
		recv_all = recv_all_;
	}
};
	
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
	
	std::vector<id_t_> soft_dev_list;
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
	
	ADD_DEL_VECTOR(soft_dev_list, id_t_);
	GET_SIZE_VECTOR(soft_dev_list);
	GET(soft_dev_list, std::vector<id_t_>);
};

struct net_interface_software_dev_t : public state_t{
private:
	std::vector<uint8_t> inbound_data;
	uint64_t last_good_inbound_micro_s = 0;
	std::vector<uint8_t> outbound_data;
	uint64_t last_good_outbound_micro_s = 0;

	// state has state_ptr and state_format
	// state format is the medium
	// state ptr is the socket or whatever
	id_t_ hardware_dev_id = ID_BLANK_ID;	
	id_t_ reliability_number_set_id = ID_BLANK_ID;
	id_t_ address_id = ID_BLANK_ID;

	// intermediary uint8_t is set on changes to intermediary_id
	id_t_ intermediary_id = ID_BLANK_ID;
	uint8_t intermediary = 0;
public:
	data_id_t id;
	net_interface_software_dev_t();
	~net_interface_software_dev_t();
	GET(last_good_inbound_micro_s, uint64_t);
	GET(last_good_outbound_micro_s, uint64_t);
	GET_SET_ID(hardware_dev_id);
	GET_SET_ID(address_id);
	GET_SET_ID(intermediary_id);
	GET(intermediary, uint8_t);
	//GET_SET_ID(reliability_number_set_id); // really no need for public access
};

#define NET_INTERFACE_MEDIUM_COUNT 1

extern net_interface_medium_t interface_medium_lookup(uint8_t medium);

#endif
		
