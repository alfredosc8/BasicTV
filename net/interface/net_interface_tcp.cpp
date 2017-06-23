#include "net_interface.h"
#include "net_interface_ip.h"
#include "net_interface_tcp.h"
#include "net_interface_hardware.h"
#include "net_interface_software.h"
#include "net_interface_intermediary.h"
#include "net_interface_medium.h"
#include "net_interface_ip_address.h"

#include "net_interface_helper.h"

/*
  The best way to think about how this packetizer works is thinking of
  this as an intermediate between the information coming in and going
  directly to whatever the underlying software device (socket) is.

  TCP is pretty weird, since a lot of the cool things it does are offloaded
  to the software device directly (i.e. a socket device is created that does
  a lot of the fancy stuff for us), so it just passes the information directly
  down as one giant vector.

  UDP segments the information into the highest possible MTU (defined as the
  lowest of either the software_dev MTU and the hardware_dev MTU), including
  a numbering scheme for making sure all of the information needed arrives in
  order. A vector of recently sent packets should be kept around so we can
  retransmit the information needed (TODO: allow for the packetizer to
  send information about itself to and from, so we can actually get some decent
  performance out of UDP, since UDP doesn't have the sort of retransmission
  latency that radio does).

  This is where packet radio starts to get pretty hairy, since this would be
  where software modems and modulation schemes start to come into play
*/

INTERFACE_PACKETIZE(ip, tcp){
	INTERFACE_SET_HW_PTR(hardware_dev_id);
	INTERFACE_SET_SW_PTR(software_dev_id);
	std::vector<std::vector<uint8_t> > retval(
		{*packet});
	packet->clear();
	return retval;
}

INTERFACE_DEPACKETIZE(ip, tcp){
	INTERFACE_SET_HW_PTR(hardware_dev_id);
	INTERFACE_SET_SW_PTR(software_dev_id);
	std::vector<uint8_t> retval =
		convert::vector::collapse_2d_vector(
			*packet);
	packet->clear();
	return retval;
}
