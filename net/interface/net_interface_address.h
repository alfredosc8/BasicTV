#ifndef NET_INTERFACE_ADDRESS_H
#define NET_INTERFACE_ADDRESS_H

#include "net_interface.h"


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
	
	/*
	  Packet format and packet encapsulation system to use

	  The only current IP combination is TCP and TCP (TCP has pretty gnarly
	  error correction at a protocol leve, so it has a special type here
	  that leaves that alone). UDP and UDP_ORDERED just prefaces the
	  packets with a 32-bit number designating the packet number, and a 
	  32-bit number of the last packet in the set. The packetizer and
	  depacketizer are allowed to use an agreed-upon escape character to
	  communicate lost packet info to one another, but that isn't needed
	  and would add bloat to a system like TCP.
	*/
	uint8_t packet_modulation = 0;
	uint8_t packet_encapsulation = 0;

	/*
	  Required intermediary

	  This is pretty IP-centric, but having a generic interface could be
	  pretty useful for one on one links. I don't see a need for a specific
	  proxy to be used to connect, but instead just to require either Tor or
	  I2P to prevent collateral damage
	 */
	uint8_t required_intermediary = 0;

	/*
	  Last attempted connect time
	 */
	uint64_t last_attempted_connect_time = 0;
public:
	void list_virtual_data(data_id_t *id);
	GET_SET(first_time_micro_s, uint64_t);
	GET_SET(end_to_start_micro_s, uint64_t);
	GET_SET(start_available_micro_s, uint64_t);
	GET_SET(end_available_micro_s, uint64_t);

	GET_SET(latitude, std::vector<uint8_t>);
	GET_SET(longitude, std::vector<uint8_t>);

	GET_SET(last_attempted_connect_time, uint64_t);
	
	void set_medium_modulation_encapsulation(
		uint8_t medium_,
		uint8_t packet_modulation_,
		uint8_t packet_encapsulation_);
	
	GET(medium, uint8_t);
	GET(packet_modulation, uint8_t);
	GET(packet_encapsulation, uint8_t);
	GET(required_intermediary, uint8_t);
};

#endif
