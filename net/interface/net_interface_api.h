#ifndef NET_INTERFACE_API_H
#define NET_INTERFACE_API_H

#include "../../id/id_api.h"
#include "../../util.h"

/*
  net_interface_api.h: Responsible for managing hardware device,
  software device, and address couplings, drops, and other
  inter-hardware switching.
 */

#define VALID_ADDRESS_ID(id_) if(net_interface::

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
	namespace medium{
		uint8_t from_address(id_t_ address_id);
	};
	/*
	  Don't be afriad to use these
	 */
	namespace ip{
		uint8_t get_address_type(std::string ip);
		namespace raw{
			std::string to_readable(std::pair<std::vector<uint8_t>, uint8_t>);
		}
		namespace readable{
			std::pair<std::vector<uint8_t>, uint8_t> to_raw(std::string readable);
		}
	};
	namespace radio{
	};
};
#endif
