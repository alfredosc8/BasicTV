/*
  net_proto.h: The networking protocol
 */
#include "../net_const.h"
#ifndef NET_PROTO_H
#define NET_PROTO_H
#include "../../id/id.h"
#include "string"
#include "SDL2/SDL_net.h"

/*
  Multiple connections can be made to a single client perfectly fine. However,
  the node you are connecting to will register all of the sockets as individual
  peers. 

  Multiple connections doesn't make much sense unless you want to increase the
  throughput of the Tor network by opening more circuits. SOCKS proxies are 
  somewhat implemented, but the easy creating and destroying of Tor circuits 
  is not (shouldn't be hard).
 */

#define NET_PROTO_MAX_SOCKET_PER_PEER 16

// single identifies the beginning of the metadata, double is single, etc.
#define NET_PROTO_DEV_CTRL_1 0x11

// adjust this value
#define NET_PROTO_META_LENGTH 12

extern void net_proto_init();
extern void net_proto_loop();
extern void net_proto_close();


typedef uint32_t net_proto_standard_size_t;
typedef uint8_t net_proto_standard_ver_t;
typedef uint8_t net_proto_standard_macros_t;
//once net_proto_standard_unused_t actually suits a purpose, then remove
// this as a type and replace it with whatever
typedef uint32_t net_proto_standard_unused_t;

#define NET_REQUEST_MAX_LENGTH 512

struct net_request_t{
private:
	std::array<uint64_t, NET_REQUEST_MAX_LENGTH> ids = {{0}};
	/*
	  socket_id is a cache value set by the inbound code. This can't be
	  global, since sockets IDs aren't shared between nodes
	 */
	uint64_t socket_id = 0;
	/*
	  Tells the difference between incoming and outgoing requests,
	  although rather crude
	 */
	bool local = false;
public:
	data_id_t id;
	net_request_t();
	~net_request_t();
	void set_socket_id(uint64_t socket_id_);
	uint64_t get_socket_id();
	void add_id(uint64_t id_);
	void del_id(uint64_t id_);
	uint64_t get_id(uint64_t entry);
	bool is_local();
};

#endif
#include "net_proto_peer.h"
