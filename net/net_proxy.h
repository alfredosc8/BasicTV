#ifndef NET_PROXY_H
#define NET_PROXY_H

#include "net_ip.h"
#include "../id/id.h"
#include "../util.h"

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
public:
	data_id_t id;
	net_proxy_t();
	~net_proxy_t();
	void set_flags(uint8_t flags_);
	uint8_t get_flags();
};
#endif
