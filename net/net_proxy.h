#ifndef NET_PROXY_H
#define NET_PROXY_H

#include "net_ip.h"
#include "../id/id.h"

/*
  net_proxy_t: proxy information

  This is just the metadata for a proxy, and does not tie itself to a socket,
  but a socket ties itself to this.

  Tor circuits created automatically aren't exported or networked, since there
  is no real value in saving any of the information here (upload and download
  speeds are too sporadic)
 */

struct net_proxy_t : public net_ip_t{
private:
	uint8_t flags = 0;
	id_t_ outbound_stat_sample_set_id = ID_BLANK_ID;
	id_t_ inbound_stat_sample_set_id = ID_BLANK_ID;
public:
	data_id_t id;
	net_proxy_t();
	~net_proxy_t();
	void set_flags(uint8_t flags_);
	uint8_t get_flags();
	// no need for setters
	void set_outbound_stat_sample_set_id(id_t_ outbound_sample_set_id_);
	id_t_ get_outbound_stat_sample_set_id();
	void set_inbound_stat_sample_set_id(id_t_ inbound_sample_set_id_);
	id_t_ get_inbound_stat_sample_set_id();
};
#endif
