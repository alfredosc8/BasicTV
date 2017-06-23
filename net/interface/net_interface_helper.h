#ifndef NET_INTERFACE_HELPER_H
#define NET_INTERFACE_HELPER_H

#include "net_interface_hardware.h"
#include "net_interface_software.h"
#include "net_interface_ip_address.h"

/*
  Try and use these for every function for now, it makes it
  easier to do sanity checks on the information later
 */

#define INTERFACE_SET_HW_PTR(x)					\
	net_interface_hardware_dev_t *hardware_dev_ptr =	\
		PTR_DATA(x,					\
			 net_interface_hardware_dev_t);		\
	PRINT_IF_NULL(hardware_dev_ptr, P_ERR);			\
	
#define INTERFACE_SET_SW_PTR(x)					\
	net_interface_software_dev_t *software_dev_ptr =	\
		PTR_DATA(x,					\
			 net_interface_software_dev_t);		\
	PRINT_IF_NULL(software_dev_ptr, P_ERR);			\
	
#define INTERFACE_SET_ADDR_PTR(x)			\
	net_interface_ip_address_t *ip_address_ptr =	\
		PTR_DATA(x,				\
			 net_interface_ip_address_t);	\
	PRINT_IF_NULL(ip_address_ptr, P_ERR);		\

#define INTERFACE_SET_INTERMEDIARY_PTR(x)			\
	net_interface_intermediary_t *intermediary_ptr =	\
		PTR_DATA(x,					\
			 net_interface_intermediary_t);		\
	PRINT_IF_NULL(intermediary_ptr, P_ERR);			\


#endif
