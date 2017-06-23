#ifndef NET_INTERFACE_TCP_H
#define NET_INTERFACE_TCP_H

#include "net_interface.h"
#include "net_interface_packet.h"

extern INTERFACE_PACKETIZE(ip, tcp);
extern INTERFACE_DEPACKETIZE(ip, tcp);

#endif
