#ifndef NET_PROTO_REQUEST_H
#define NET_PROTO_REQUEST_H

/*
  When a linked list request is sent over to a peer, any new information
  along that linked list (within the length set in the type) will be
  automatically sent. 

  TODO: include multiple peers to send that information down
 */

struct net_proto_linked_list_request_t{
private:
	id_t_ start_id = ID_BLANK_ID;
	uint64_t start_length = 0;
	// cache
	id_t_ curr_id = ID_BLANK_ID;
	uint64_t curr_length = 0;
public:
	data_id_t id;
	net_proto_linked_list_request_t();
	~net_proto_linked_list_request_t();
};

struct net_proto_request_standard_t{
private:
	id_t_ peer_id = ID_BLANK_ID;
	std::vector<id_t_> ids;
	std::vector<uint64_t> mod_inc;
public:
	net_proto_request_standard_t();
	~net_proto_request_standard_t();
	void list_virtual_data(data_id_t *id);
	void set_peer_id(id_t_);
	id_t_ get_peer_id();
	// modes of operation are defined by the container
	void set_ids(std::vector<id_t_>);
	std::vector<id_t_> get_ids();
	std::vector<uint64_t> get_mod_inc();
};

/*
  Request a set of IDs, done on a whitelist basis and type checking
  is not a part of this system
 */

struct net_proto_id_request_t : public net_proto_request_standard_t{
private:
public:
	data_id_t id;
	net_proto_id_request_t();
	~net_proto_id_request_t();
};

/*
  Request based on type. ID vector, in this case, will act as a blacklist
 */

struct net_proto_type_request_t : public net_proto_request_standard_t{
private:
	std::array<uint8_t, 32> type = ID_BLANK_TYPE;
public:
	data_id_t id;
	net_proto_type_request_t();
	~net_proto_type_request_t();
	void update_type(
		std::array<uint8_t, 32> type_);
};

#endif
