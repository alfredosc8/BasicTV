#ifndef NET_PROTO_CONNECTIONS_H
#define NET_PROTO_CONNECTIONS_H

/*
  A lot of the holepunching code can be abstracted out between incoming and
  outgoing, and this is the overlap (at least for TCP).
 */
#include "net_proto_con_req.h"
extern void net_proto_handle_tcp_holepunch(net_proto_con_req_t *con_req);

extern void net_proto_connection_manager();
#endif
