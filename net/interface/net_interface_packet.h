#ifndef NET_INTERFACE_PACKET_H
#define NET_INTERFACE_PACKET_H

#include "../../util.h"

#define NET_INTERFACE_MEDIUM_PACKET_MTU_UNDEFINED 0

#define NET_INTERFACE_MEDIUM_PACKET_MODULATION_UNDEFINED 0
// IP
#define NET_INTERFACE_MEDIUM_PACKET_MODULATION_TCP 1
#define NET_INTERFACE_MEDIUM_PACKET_MODULATION_UDP 2

#define NET_INTERFACE_MEDIUM_PACKET_ENCAPSULATION_TCP 1
#define NET_INTERFACE_MEDIUM_PACKET_ENCAPSULATION_UDP_ORDERED 2

// Radio
#define NET_INTERFACE_NEDIUM_PACKET_MODULATION_BELL_202 3 // 1200 baud
#define NET_INTERFACE_MEDIUM_PACKET_MODULATION_G3RUH_DFSK 4 // 9600 baud

#define NET_INTERFACE_MEDIUM_PACKET_ENCAPSULATION_AX_25 3
#define NET_INTERFACE_MEDIUM_PACKET_ENCAPSULATION_FX_25 4 // AX.25 with FEC


#define INTERFACE_PACKETIZE(medium, packet_) std::vector<std::vector<uint8_t> > net_interface_##medium##_##packet_##_packetize(id_t_ hardware_dev_id, id_t_ software_dev_id, std::vector<uint8_t> *packet)
#define INTERFACE_DEPACKETIZE(medium, packet_) std::vector<uint8_t> net_interface_##medium##_##packet_##_depacketize(id_t_ hardware_dev_id, id_t_ software_dev_id, std::vector<std::vector<uint8_t> > *packet)

struct net_interface_medium_packet_t{
private:
	uint32_t mtu = 0;
	// format is the modulation scheme used
	uint8_t format = 0;
	// encapsulation is any error correction or packetization,
	uint8_t encapsulation = 0;
public:
	GET(mtu, uint32_t);
	std::vector<std::vector<uint8_t> > (*packetize)(
		id_t_ hardware_dev_id,
		id_t_ software_dev_id,
		std::vector<uint8_t> *packet);

	std::vector<uint8_t> (*depacketize)(
		id_t_ hardware_dev_id,
		id_t_ software_dev_id,
		std::vector<std::vector<uint8_t> > *packet);

	net_interface_medium_packet_t(
		std::vector<std::vector<uint8_t> > (*packetize_)(
			id_t_ hardware_dev_id,
			id_t_ software_dev_id,
			std::vector<uint8_t> *packet),
		std::vector<uint8_t> (*depacketize_)(
			id_t_ hardware_dev_id,
			id_t_ software_dev_id,
			std::vector<std::vector<uint8_t> > *packet),
		uint32_t mtu_){
		packetize = packetize_;
		depacketize = depacketize_;
		mtu = mtu_;
	}
};

extern void sanity_check_modulation_and_encapsulation(
	uint8_t modulation,
	uint8_t encapsulation);

#endif
