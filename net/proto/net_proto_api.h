#ifndef NET_PROTO_API_H
#define NET_PROTO_API_H

#include "algorithm"

namespace net_proto{
	namespace request{
		void add_id(id_t_ id);
		void add_id(std::vector<id_t_> id);
		void add_id_linked_list(id_t_ id, int64_t length);
		void del_id(id_t_ id);
		void del_id(std::vector<id_t_> id);
		void add_fast_routine_type(std::string type);
		void del_fast_routine_type(std::string type);
		void add_slow_routine_type(std::string type);
		void del_slow_routine_type(std::string type);
	};
	namespace peer{
		void set_self_peer_id(id_t_ self_peer_id_);
		// IP/URL and port, easy wrapper. Assume default if not called
		void set_self_as_peer(std::string ip, uint16_t port);
		id_t_ get_self_as_peer();
		id_t_ random_peer_id();
		id_t_ optimal_peer_for_id(id_t_ id);
	};
	namespace socket{
		/*
		  Attempts to start min amount of simultaneous connections with
		  a peer. Hopefully will decrease the response time if we have
		  enough threads to effectively spread the load.
		 */
		void connect(id_t_ peer_id_, uint32_t min);
		// TODO: should be a lookup cache system in net_proto_peer_t
		std::vector<id_t_> all_proto_socket_of_peer(id_t_ peer_id);
		id_t_ optimal_proto_socket_of_peer(id_t_ peer_id);
		namespace stats{
			std::vector<id_t_> preferable_sort(std::vector<id_t_> socket_ids,
							   id_t_ request_id);
		};
	};
};

#include "net_proto.h"

#endif
