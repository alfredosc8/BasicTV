#ifndef NET_INTERFACE_IP_H
#define NET_INTERFACE_IP_H

#include "net_interface_medium.h"
#include "net_interface.h"

extern INTERFACE_ADD_ADDRESS_COST(ip);
extern INTERFACE_CALCULATE_MOST_EFFICIENT_DROP(ip);
extern INTERFACE_CALCULATE_MOST_EFFICIENT_TRANSFER(ip);
extern INTERFACE_SEND(ip);
extern INTERFACE_RECV_ALL(ip);
#endif
