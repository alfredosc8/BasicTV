/*
  net_proto.h: The networking protocol
*/
#include "../net_const.h"
#ifndef NET_PROTO_H
#define NET_PROTO_H
#include "../../id/id.h"
#include <string>
#include <SDL2/SDL_net.h>

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

#define NET_PROTO_ESCAPE 17

extern void net_proto_init();
extern void net_proto_loop();
extern void net_proto_close();

#endif
// here for legacy reasons
#include "net_proto_request.h"
#include "net_proto_peer.h"
#include "net_proto_api.h"
#include "../interface/net_interface.h"
#include "../interface/net_interface_ip.h"
#include "../interface/net_interface_ip_address.h"
#include "../interface/net_interface_medium.h"
#include "../interface/net_interface_packet.h"
#include "../interface/net_interface_api.h"
