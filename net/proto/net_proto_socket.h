#include "../../id/id.h"
#include "../net_const.h"
#include "../net_socket.h"
#ifndef NET_PROTO_SOCKET_H
#define NET_PROTO_SOCKET_H
#include <SDL2/SDL_net.h>

/*
  net_proto_socket_t: handles protocol specific transcoding

  net_proto_socket_t handles removing requests when the information
  is received, as well as encrypting all IDs for sending and abstracting
  out everything needed to maintain a connection (establishing connections,
  especially with TCP holepunching, requires direct access to the socket to
  start, which falls outside of what this datatype does).

  net_proto_socket_t, for statistics reasons, should only be bound to one peer
  for the lifetime of it. The same logic applies to sockets.
 */

struct net_proto_socket_t{
private:
	id_t_ socket_id = ID_BLANK_ID;
	id_t_ peer_id = ID_BLANK_ID;
	std::vector<uint8_t> working_buffer;
	// finalized buffer, removed DEV_CTRL_1, native endian, etc.
	std::vector<std::vector<uint8_t> > buffer;
	id_t_ outbound_stat_sample_set_id = ID_BLANK_ID;
	id_t_ inbound_stat_sample_set_id = ID_BLANK_ID;
	uint64_t last_update_time_micro_s = 0;
	// read information from sockets, parse it into working buffer
	void read_and_parse();
	// loads information, deletes requests, etc.
	void process_buffer();
	// send with whatever encryption system that has been set up
	// should be rather seamless if we sync the beginning
	void bare_send(std::vector<uint8_t> data);
	void bare_recv(); // writes to working_buffer directly
public:
	data_id_t id;
	net_proto_socket_t();
	~net_proto_socket_t();
	void set_socket_id(id_t_ socket_id_);
	id_t_ get_socket_id();
	void set_peer_id(id_t_ peer_id_);
	id_t_ get_peer_id();
	void send_id(id_t_ id_);
	void send_id_vector(std::vector<id_t_> id_vector);
	void update();
	std::vector<std::pair<uint64_t, id_t_> > get_id_log();
	std::vector<std::vector<uint8_t> > get_buffer();
	uint64_t get_last_update_micro_s();
	bool is_alive();
};

#endif
