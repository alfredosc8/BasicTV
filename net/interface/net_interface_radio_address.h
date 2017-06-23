#ifndef NET_INTERFACE_RADIO_ADDRESS_H
#define NET_INTERFACE_RADIO_ADDRESS_H

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

#endif
