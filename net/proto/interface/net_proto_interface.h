#ifndef NET_PROTO_INTERFACE_H
#define NET_PROTO_INTERFACE_H

#include "../../../state.h"
#include "../../../util.h"
#include "../../../id/id.h"
#include "../../../id/id_api.h"

/*
  net_proto_interface.h: generic interface for any interconnected medium

  This file (currently) isn't used, but is critical in the conversion
  from the current IP-defined system to a more generic OSI Model.
  This is being some to allow for future development in automated radio
  systems.

  Destination information is coupled with the state, so only the state
  needs to be called. 
 */

/*
  Is there a limitation in the protocol that states that the posted upper
  limit is the fastest RAW speed possible. This mainly applies to
  modulation schemes with radio transmissions. I wouldn't consider
  standard internet speeds to be a hard upper limit.
 */
#define NET_PROTO_INTERFACE_STATE_FLAG_HARD_UPPER_INBOUND_RATE (1 << 0)
#define NET_PROTO_INTERFACE_STATE_FLAG_HARD_UPPER_OUTBOUND_RATE (1 << 1)

/*
  Radio specific one again: is all data sent out across the network
  naturally multicasted
 */
#define NET_PROTO_INTERFACE_STATE_FLAG_NATURAL_MULTICAST (1 << 2)

/*
  Radio specific: can transmission only happen in one direction
  (bi-directional transmission and requests are still technically possible,
  but they cannot happen over the same socket)
 */

#define NET_PROTO_INTERFACE_FLAG_PUSH_ONLY (1 << 3)

// poly type, each interface has its own definition
struct net_proto_interface_peer_t{
};

struct net_proto_interface_state_t : public state_t{
private:
	std::vector<uint8_t> inbound_buffer;
	std::vector<uint8_t> outbound_buffer;
	uint64_t flags = 0;
public:
	data_id_t id;
	GET_SET(flags, uint64_t);
	GET_SET(inbound_buffer, std::vector<uint8_t>);
	GET_SET(outbound_buffer, std::vector<uint8_t>);
	void add_to_inbound(std::vector<uint8_t> data){inbound_buffer.insert(inbound_buffer.begin(),data.begin(), data.end());}
	void add_to_outbound(std::vector<uint8_t> data){outbound_buffer.insert(outbound_buffer.begin(),data.begin(), data.end());}
	// deleting would require operating on the payload, so
	// we can just copy and use it directly
	net_proto_interface_state_t();
	~net_proto_interface_state_t();
};

// state is net_proto_interface_state_t
// destination is the ID of the net_proto_interface_peer_*_t, sanity checked
// inside of the implementation function

#define NET_PROTO_INTERFACE_CONNECT_STATE(impl)	void impl##_connect_state(net_proto_interface_state_t *state, id_t_ interface_peer)
#define NET_PROTO_INTERFACE_ITERATE_STATE(impl)	void impl##_iterate_state(net_proto_interface_state_t *state)
#define NET_PROTO_INTERFACE_SEND_STATE(impl)	void impl##_send_state(net_proto_interface_state_t *state, std::vector<uint8_t> data)
#define NET_PROTO_INTERFACE_RECV_STATE(impl)	std::vector<uint8_t> impl##_recv_state(net_proto_interface_state_t *state, uint64_t bytes_to_recv, bool hang)
#define NET_PROTO_INTERFACE_CLOSE_STATE(impl)	void impl##_close_state(net_proto_interface_state_t *state)

extern void peer_sanity_check(id_t_ id, type_t_ type);

#endif
