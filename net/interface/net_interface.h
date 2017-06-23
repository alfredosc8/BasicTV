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


/*
  The mediums shouldn't include information about the encoding scheme at all,
  since we are on a lower level here
*/

/*
  Any information that needs to be abstracted out should
  be pushed off onto net_proto_peer_t. net_interface_*_address_t
  should only contain information pertaining to the medium
  (frequencies, bandwidth, modulation schemes for radio, ip
  address, port, and protocol
*/

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

/*
  Packetizing and protocols themselves are handled inside of
  net_interface_medium_packet_t. This would handle TCP, UDP, packet radio,
  error detection and correction, and all that jazz.

  Internally, this functions like a state codec, except there is no need
  to explicitly initialize, and the state is defined in 
  net_interface_software_dev_t (state_ptr)

  This function doesn't facilitate the transmission per-se, that's offloaded
  back to net_interface_medium_t

  

  TCP is pretty flexible with the sizes. UDP would benefit from exact metadata
  sizing information from net_interface_software_dev_t (?), and would optimize
  performance by pushing us closer to the MTU. Packet radio 
  
 */


/*
  NOTE TO SELF:
  Error correction is ALWAYS going to be more important than transmission
  speed (beyond 9600 baud G3RUH, that is)
 */

/*
  Any and all calls specific to the medium at play (namely
  modulation and demodulation) is handled inside of 
  net_interface_medium_t
*/

#include "net_interface_medium.h"


#define NET_INTERFACE_MEDIUM_COUNT 1
#define NET_INTERFACE_MEDIUM_PACKET_COUNT 1

extern net_interface_medium_t interface_medium_lookup(uint8_t medium);

#include "../net.h"
#endif
		
