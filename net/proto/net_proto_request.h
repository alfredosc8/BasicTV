#ifndef NET_PROTO_REQUEST_H
#define NET_PROTO_REQUEST_H

/*
  When a linked list request is sent over to a peer, any new information
  along that linked list (within the length set in the type) will be
  automatically sent. 
 */

/*
  For multicasting requests, just create new requests for simplicity.
  On arrival, it can remove or append the obsolete ones
 */

struct net_proto_request_bare_t{
private:
	uint64_t request_time = 0;
	id_t_ sender_peer_id = ID_BLANK_ID;
	id_t_ receiver_peer_id = ID_BLANK_ID;
public:
	net_proto_request_bare_t();
	~net_proto_request_bare_t();
	void set_sender_peer_id(id_t_);
	id_t_ get_sender_peer_id();
	void set_receiver_peer_id(id_t_);
	id_t_ get_receiver_peer_id();
	
	void update_request_time();
	uint64_t get_request_time(){return request_time;}
	void list_bare_virtual_data(data_id_t *id);
};

struct net_proto_request_set_t : public net_proto_request_bare_t{
private:
	std::vector<id_t_> ids;
	std::vector<uint64_t> mod_inc;
public:
	net_proto_request_set_t();
	~net_proto_request_set_t();
	void list_set_virtual_data(data_id_t *id);
	// modes of operation are defined by the container
	void set_ids(std::vector<id_t_>);
	std::vector<id_t_> get_ids();
	std::vector<uint64_t> get_mod_inc();
};

/*
  Request a set of IDs, done on a whitelist basis and type checking
  is not a part of this system
 */

struct net_proto_id_request_t : public net_proto_request_set_t{
private:
public:
	data_id_t id;
	net_proto_id_request_t();
	~net_proto_id_request_t();
};

/*
  Request based on type. ID vector, in this case, will act as a blacklist
 */

struct net_proto_type_request_t : public net_proto_request_set_t{
private:
	type_t_ type = ID_BLANK_TYPE;
public:
	data_id_t id;
	net_proto_type_request_t();
	~net_proto_type_request_t();
	void update_type(
		type_t_ type_);
	type_t_ get_type(){return type;}
};

/*
  Request an ID and all following (or soon to be preceding) IDs to a point. To
  help programming, there is a cache to keep track of which IDs have been
  fulfilled, telling us what we still need to send.
 */

struct net_proto_linked_list_request_t : public net_proto_request_bare_t{
private:
	id_t_ start_id = ID_BLANK_ID;
	uint64_t start_length = 0;
	// cache
	id_t_ curr_id = ID_BLANK_ID;
	uint64_t curr_length = 0;
	/*
	  This is a pre-computed list of all IDs that we currently
	  know of that need to be sent over. This list doesn't have anything
	  removed from it, so it can be continually updated with the next
	  and previous linked list entries until we reach the limit.

	  To prevent spam blocking and other concerns, we MUST send IDs in
	  a way where there is a distinguishable link between the start ID
	  and all following IDs (linked lists aren't a 1:1, but instead can
	  list multiple IDs in front and behind, the IDs need to be sent over
	  so there is a valid list of references between the start ID and the
	  ID being sent over). This might be a performance problem over time,
	  but that depends on if Tor multiplexing is happening or not.
	*/
	std::vector<std::pair<id_t_, bool> > id_state_list;
public:
	data_id_t id;
	void increase_id();
	void set_curr_id(id_t_ id_, uint32_t length);
	id_t_ get_curr_id();
	net_proto_linked_list_request_t();
	~net_proto_linked_list_request_t();
};


#endif
