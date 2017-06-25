#ifndef NET_INTERFACE_API_H
#define NET_INTERFACE_API_H

#include "../../id/id_api.h"
#include "../../util.h"

/*
  net_interface_api.h: Responsible for managing hardware device,
  software device, and address couplings, drops, and other
  inter-hardware switching.
 */

namespace net_interface{
	namespace bind{
		// returns a valid net_interface_software_dev_t with
		// proper couplings to net_interface_hardware_dev_t and
		// to the given address
		// throws if binding is non-free (need to call either a drop
		// or a transfer first)
		id_t_ address_to_hardware(
			id_t_ address_id,
			id_t_ hardware_dev_id);
		id_t_ software_to_hardware(
			id_t_ software_dev_id,
			id_t_ hardware_dev_id);
	};
	namespace unbind{
		void software_to_hardware(
			id_t_ software_dev_id,
			id_t_ hardware_dev_id);
	};
};
#endif
