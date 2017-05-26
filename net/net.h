#ifndef NET_H
#define NET_H
#include <string>
#include <vector>
#include <curl/curl.h>

#include "../id/id.h"
#include "../id/id_api.h"

namespace net{
	namespace stats{
		// easy way to standardize basic network information across
		// multiple handlers of the same data type
		void add_throughput_datum( // throughput of data
			uint64_t byte_volume,
			uint64_t start_time_micro_s,
			uint64_t end_time_micro_s,
			id_t_ socket_id,
			id_t_ proxy_id);
		void add_latency_datum( // ping/pong, when implemented
			// ping/pong, when made, has a fixed size
			uint64_t start_time_micro_s,
			uint64_t end_time_micro_s,
			id_t_ socket_id,
			id_t_ proxy_id);
		// individual ID stats is handled on protocol
		// level somehow (tie net_proto_socket_t and
		// net_proto_peer_t to it)
		id_t_ pull_net_stats_from_global(id_t_ net_id);
	};
	std::string get_cache(std::string url, int stale_time);
	void force_url(std::string url);
	std::string get_url(std::string url, int stale_time = 30);
	std::string get(std::string url, int stale_time = 30);
};
#endif

/*
  net_stats_t

  Currently, this only deals with ping time and throughput statistics.
  Everything else right now is independent enough to work without it.
 */

std::string net_get_ip();

#include "net_socket.h"
#include "proto/net_proto.h"
