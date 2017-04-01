#include "../../id/id.h"
#include "../net_const.h"
#include "../net_socket.h"
#include "../../util.h"
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

  It is possible to disable encryption for certain parts of the stream to allow
  for sending encrypt_pub_key_t along the socket without having to take care
  of SSL or creating an entire non-encrypted connection.

  TODO: re-add stats somehow
 */

#define NET_PROTO_SOCKET_NO_ENCRYPT (1 << 1)

struct net_proto_socket_t{
private:
	id_t_ socket_id = ID_BLANK_ID;
	id_t_ peer_id = ID_BLANK_ID;
	uint8_t flags = 0;
	uint64_t last_recv_micro_s = 0;
	std::vector<uint8_t> std_data;
	std::vector<uint8_t> working_buffer;
	std::vector<std::pair<std::vector<uint8_t>, std::vector<uint8_t> > > block_buffer;
	void update_working_buffer();
	void update_block_buffer();
	void load_blocks();
public:
	data_id_t id;
	net_proto_socket_t();
	~net_proto_socket_t();

	// getters and setters
	void set_socket_id(id_t_ socket_id_){socket_id = socket_id_;}
	id_t_ get_socket_id(){return socket_id;}
	void set_peer_id(id_t_ peer_id_){peer_id = peer_id_;}
	id_t_ get_peer_id(){return peer_id;}
	void set_flags(uint8_t flags_){flags = flags_;}
	uint8_t get_flags(){return flags;}

	// send stuff
	void send_id(id_t_ id_);
	void send_id_vector(std::vector<id_t_> id_vector);
	void update();
	uint64_t get_last_recv_micro_s(){return last_recv_micro_s;}
	// TODO: should probably do something else...
	bool is_alive(){return get_time_microseconds()-last_recv_micro_s <= 30*1000*1000;}
};

#endif
