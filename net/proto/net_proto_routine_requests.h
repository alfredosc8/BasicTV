#ifndef NET_PROTO_ROUTINE_REQUESTS_H
#define NET_PROTO_ROUTINE_REQUESTS_H

/*
  This is for handling routine requests that ought to be made as an entire
  data types.

  This handles all information that should be routinely received, which
  includes, but is not limited to:
  net_con_req_t (connection requests for a direct TCP holepunch)
  net_proto_peer_t (any other node on the network)
  tv_channel_t (TV channel)

  That's it for now
 */

// 5 second
#define NET_PROTO_ROUTINE_REQUEST_DEFAULT_SLOW_INTERVAL 5000000

// .5 seconds
#define NET_PROTO_ROUTINE_REQUEST_DEFAULT_FAST_INTERVAL 500000

// doesn't run it every time, checks settings file for frequency
// TODO: add the ability to add and remove items from the request list
extern void net_proto_routine_requests_loop();
#endif
