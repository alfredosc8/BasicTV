#ifndef NET_PROTO_API_H
#define NET_PROTO_API_H

namespace net_proto{
	namespace request{
		void add_id(id_t_ id);
		void add_id(std::vector<id_t_> id);
		void del_id(id_t_ id);
		void del_id(std::vector<id_t_> id);
	};
	namespace peer{
		void set_self_peer_id(id_t_ self_peer_id_);
		// IP/URL and port, easy wrapper. Assume default if not called
		void set_self_as_peer(std::string ip, uint16_t port);
		id_t_ get_self_as_peer();
	};
	namespace socket{
		/*
		  Attempts to start min amount of simultaneous connections with
		  a peer. Hopefully will decrease the response time if we have
		  enough threads to effectively spread the load.
		 */
		std::vector<id_t_> connect(id_t_ peer_id_, uint32_t min);
		// TODO: should be a lookup cache system in net_proto_peer_t
		std::vector<id_t_> all_proto_socket_of_peer(id_t_ peer_id);
		id_t_ optimal_proto_socket_of_peer(id_t_ peer_id);
		namespace stats{
			std::vector<id_t_> preferable_sort(std::vector<id_t_> socket_ids,
							   id_t_ request_id);
		};
	};
};

#endif
