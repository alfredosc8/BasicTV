#include "net_proto.h"
#include "net_proto_request.h"
#include "net_proto_socket.h"
#include "../../id/id_api.h"

#include "../../settings.h"


// routine request jargon
static uint64_t last_request_fast_time_micro_s = 0;
static uint64_t last_request_slow_time_micro_s = 0;

static std::vector<uint8_t> routine_request_fast_vector = {
	TYPE_ENCRYPT_PUB_KEY_T,
	TYPE_NET_PROTO_PEER_T,
	TYPE_NET_PROTO_CON_REQ_T
};
static std::vector<uint8_t> routine_request_slow_vector = {
	TYPE_TV_ITEM_T,
	TYPE_TV_CHANNEL_T,
	TYPE_WALLET_SET_T
};

// standard request jargon
static std::vector<id_t_> id_request_buffer;
static std::vector<std::pair<id_t_, int64_t> > linked_list_request_buffer;

net_proto_request_bare_t::net_proto_request_bare_t(){
}

net_proto_request_bare_t::~net_proto_request_bare_t(){}

void net_proto_request_bare_t::list_bare_virtual_data(data_id_t *id){
	id->add_data_id(&origin_peer_id, 1);
	id->add_data_id(&destination_peer_id, 1);
	id->add_data_raw((uint8_t*)&request_time, sizeof(request_time));
	id->add_data_raw((uint8_t*)&ttl_micro_s, sizeof(ttl_micro_s));
}

id_t_ net_proto_request_bare_t::get_origin_peer_id(){
	return origin_peer_id;
}

void net_proto_request_bare_t::set_origin_peer_id(id_t_ origin_peer_id_){
	origin_peer_id = origin_peer_id_;
}

id_t_ net_proto_request_bare_t::get_destination_peer_id(){
	return destination_peer_id;
}
void net_proto_request_bare_t::set_destination_peer_id(id_t_ destination_peer_id_){
	destination_peer_id = destination_peer_id_;
}

void net_proto_request_bare_t::update_request_time(){
	request_time = get_time_microseconds();
}

net_proto_request_set_t::net_proto_request_set_t(){}

net_proto_request_set_t::~net_proto_request_set_t(){}

void net_proto_request_set_t::list_set_virtual_data(data_id_t *id){
	id->add_data_id_vector(&ids, 65536);
	id->add_data_eight_byte_vector(&mod_inc, 65536);
}

void net_proto_request_set_t::set_ids(std::vector<id_t_> ids_){
	ids = ids_;
	mod_inc.clear();
	for(uint64_t i = 0;i < ids_.size();i++){
		data_id_t *id_ptr =
			PTR_ID_FAST(ids_[i], );
		if(likely(id_ptr == nullptr)){
			mod_inc.push_back(0);
		}else{
			mod_inc.push_back(
				id_ptr->get_mod_inc());
		}
	}
	update_request_time();
}

std::vector<id_t_> net_proto_request_set_t::get_ids(){
	return ids;
}

std::vector<uint64_t> net_proto_request_set_t::get_mod_inc(){
	if(mod_inc.size() != ids.size()){
		print("mod_inc is a different size than ids, assuming all old", P_ERR);
		return {}; 
	}
	return mod_inc;
}

// IDs

net_proto_id_request_t::net_proto_id_request_t() : id(this, TYPE_NET_PROTO_ID_REQUEST_T){
	list_set_virtual_data(&id);
	list_bare_virtual_data(&id);
	id.set_lowest_global_flag_level(
		ID_DATA_NETWORK_RULE_PUBLIC,
		ID_DATA_EXPORT_RULE_NEVER,
		ID_DATA_RULE_UNDEF);
}

net_proto_id_request_t::~net_proto_id_request_t(){}

// Type

net_proto_type_request_t::net_proto_type_request_t() : id(this, TYPE_NET_PROTO_TYPE_REQUEST_T){
	list_set_virtual_data(&id);
	list_bare_virtual_data(&id);
	id.add_data_raw(&type, sizeof(type));
	id.set_lowest_global_flag_level(
		ID_DATA_NETWORK_RULE_PUBLIC,
		ID_DATA_EXPORT_RULE_NEVER,
		ID_DATA_RULE_UNDEF);
}	

net_proto_type_request_t::~net_proto_type_request_t(){}

// Linked list subscription

net_proto_linked_list_request_t::net_proto_linked_list_request_t() : id(this, TYPE_NET_PROTO_LINKED_LIST_REQUEST_T){
	list_bare_virtual_data(&id);
	id.set_lowest_global_flag_level(
		ID_DATA_NETWORK_RULE_PUBLIC,
		ID_DATA_EXPORT_RULE_NEVER,
		ID_DATA_RULE_UNDEF);
}

net_proto_linked_list_request_t::~net_proto_linked_list_request_t(){
}

void net_proto_linked_list_request_t::set_curr_id(id_t_ id_, uint32_t length){
	curr_id = id_;
	curr_length = length;
}

void net_proto_linked_list_request_t::increase_id(){
	if(curr_length == 0){
		return;
	}
	data_id_t *id_ =
		PTR_ID(curr_id, );
	if(id_ == nullptr){
		return;
	}
	std::pair<std::vector<id_t_>, std::vector<id_t_> > linked_list =
		id_->get_linked_list();
	if(linked_list.second.size() == 0){
		curr_id = ID_BLANK_ID;
	}else{
		curr_id = linked_list.second[0];
	}
	curr_length--;
}

id_t_ net_proto_linked_list_request_t::get_curr_id(){
	return curr_id;
}

void net_proto_type_request_t::update_type(type_t_ type_){
	type = type_;
	set_ids(
		id_api::cache::get(
			convert::type::from(
				type_)));
	update_request_time();
}

// TODO: combine this file and routine_requests



void net_proto::request::add_fast_routine_type(std::string type){
	routine_request_fast_vector.push_back(
		convert::type::to(
			type));
}

void net_proto::request::add_slow_routine_type(std::string type){
	routine_request_slow_vector.push_back(
		convert::type::to(
			type));
}

void net_proto::request::del_fast_routine_type(std::string type){
	auto iterator =
		std::find(
			routine_request_fast_vector.begin(),
			routine_request_fast_vector.end(),
			convert::type::to(type));
	if(iterator != routine_request_fast_vector.end()){
		routine_request_fast_vector.erase(
			iterator);
	}
}

void net_proto::request::del_slow_routine_type(std::string type){
	auto iterator =
		std::find(
			routine_request_slow_vector.begin(),
			routine_request_slow_vector.end(),
			convert::type::to(type));
	if(iterator != routine_request_slow_vector.end()){
		routine_request_slow_vector.erase(
			iterator);
	}
}

void net_proto::request::add_id(id_t_ id){
	// could probably speed this up
	for(uint64_t i = 0;i < id_request_buffer.size();i++){
		if(id == id_request_buffer[i]){
			print("requesting ID already in request vector, should implement some form of QoS", P_SPAM);
			return;
		}
	}
	for(uint64_t i = 0;i < id_request_buffer.size();i++){
		if(get_id_hash(id) == get_id_hash(id_request_buffer[i])){
			id_request_buffer.insert(
				id_request_buffer.begin()+i,
				id);
			break;
		}
	}
	id_request_buffer.push_back(id);
}

void net_proto::request::add_id(std::vector<id_t_> id){
	// could probably speed this up
	for(uint64_t c = 0;c < id.size();c++){
		for(uint64_t i = 0;i < id_request_buffer.size();i++){
			while(get_id_hash(id[c]) == get_id_hash(id_request_buffer[i])){
				id_request_buffer.insert(
					id_request_buffer.begin()+i,
					id[c]);
				if(c < id.size()){
					c++;
				}
			}
		}
		id_request_buffer.push_back(id[c]);
	}
}

/*
  Signed-ness is important for dictating direction
 */

void net_proto::request::add_id_linked_list(id_t_ id, int64_t length){
	for(uint64_t i = 0;i < linked_list_request_buffer.size();i++){
		if(unlikely(get_id_hash(linked_list_request_buffer[i].first) ==
			    get_id_hash(id))){
			std::pair<id_t_, int64_t> request_entry =
				std::make_pair(
					id, length);
			linked_list_request_buffer.insert(
				linked_list_request_buffer.begin()+i,
				request_entry);
			return;
		}
	}
	linked_list_request_buffer.push_back(
		std::make_pair(
			id, length));
}

void net_proto::request::del_id(id_t_ id){
	std::vector<id_t_> id_request_vector =
		id_api::cache::get(
			"net_proto_id_request_t");
	for(uint64_t i = 0;i < id_request_vector.size();i++){
		net_proto_id_request_t *id_request =
			PTR_DATA(id_request_vector[i],
				 net_proto_id_request_t);
		if(id_request == nullptr){
			continue;
		}
		std::vector<id_t_> id_vector =
			id_request->get_ids();
		auto id_ptr =
			std::find(
				id_vector.begin(),
				id_vector.end(),
				id);
		if(id_ptr != id_vector.end()){
			id_vector.erase(
				id_ptr);
			id_request->set_ids(
				id_vector);
			return;
		}
	}
	print("cannot delete request for ID I didn't request", P_ERR);
}

static void net_proto_routine_request_create(
	std::vector<type_t_> type_vector,
	uint64_t request_interval_micro_s,
	uint64_t *last_request_time_micro_s){
	const uint64_t time_micro_s =
		get_time_microseconds();
	if(time_micro_s-(*last_request_time_micro_s) > request_interval_micro_s){
		print("creating routine request with frequency " + std::to_string(request_interval_micro_s) + "micro_s", P_SPAM);
		for(uint64_t i = 0;i < type_vector.size();i++){
			P_V_S(convert::type::from(type_vector[i]), P_VAR);
			// all request names are in the perspective of the
			// sender, not the receiver. This makes the receiver
			// the person we send it to, and the sender ourselves.
			id_t_ recv_peer_id =
				net_proto::peer::random_peer_id();
			if(recv_peer_id == ID_BLANK_ID){
				print("we have no other peer information whatsoever, not creating any network requests", P_DEBUG);
			}else{
				std::vector<id_t_> id_vector =
					id_api::cache::get(
						type_vector[i]);
				std::vector<uint64_t> mod_vector =
					id_api::bulk_fetch::mod(
						id_vector);
				net_proto_type_request_t *type_request =
					new net_proto_type_request_t;
				type_request->update_type(
					type_vector[i]);
				type_request->set_ttl_micro_s(
					request_interval_micro_s);
				type_request->set_destination_peer_id(
					net_proto::peer::random_peer_id());
				type_request->set_origin_peer_id(
					net_proto::peer::get_self_as_peer());
			}
 		}
		*last_request_time_micro_s = time_micro_s;
 	}
}

static void net_proto_routine_request_loop(){
 	net_proto_routine_request_create(
 		routine_request_fast_vector,
		1*1000*1000,
 		&last_request_fast_time_micro_s);
 	net_proto_routine_request_create(
 		routine_request_slow_vector,
		1*1000*1000,
 		&last_request_slow_time_micro_s);
}

static void net_proto_create_id_request_loop(){
	std::vector<std::pair<std::vector<id_t_>, id_t_> > id_peer_pair;
	for(uint64_t i = 0;i < id_request_buffer.size();i++){
		const id_t_ preferable_peer_id =
			net_proto::peer::optimal_peer_for_id(
				id_request_buffer[i]);
		if(preferable_peer_id == ID_BLANK_ID){
			print("preferable_peer_id is blank", P_ERR);
		}
		ASSERT(preferable_peer_id != net_proto::peer::get_self_as_peer(), P_ERR);
		for(uint64_t c = 0;c < id_peer_pair.size();c++){
			if(id_peer_pair[c].second == preferable_peer_id){
				id_peer_pair[c].first.push_back(
					id_request_buffer[i]);
				break;
			}
		}
		id_peer_pair.push_back(
			std::make_pair(
				std::vector<id_t_>({id_request_buffer[i]}),
				preferable_peer_id));
	}
	// print("sending " + std::to_string(id_peer_pair.size()) + " requests, totalling " + std::to_string(id_request_buffer.size()) + " IDs", P_SPAM);
	id_request_buffer.clear();
	const id_t_ self_peer_id =
		net_proto::peer::get_self_as_peer();
	for(uint64_t i = 0;i < id_peer_pair.size();i++){
		try{
			net_proto_id_request_t *id_request_ptr =
				new net_proto_id_request_t;
			id_request_ptr->set_ttl_micro_s(
				30*1000*1000); // TODO: should make this a setting
			id_request_ptr->set_ids(
				id_peer_pair[i].first);
			id_request_ptr->set_origin_peer_id(
				self_peer_id);
			id_request_ptr->set_destination_peer_id(
				id_peer_pair[i].second);
		}catch(...){
			print("failed to create net_proto_id_request_t", P_WARN);
			// not sure how this would happen...
			id_request_buffer.insert(
				id_request_buffer.end(),
				id_peer_pair[i].first.begin(),
				id_peer_pair[i].first.end());
		}
	}
}

#pragma message("net_proto_create_linked_list_request_loop is not implemented")

static void net_proto_create_linked_list_request_loop(){
	print("implement me", P_CRIT);
}

/*
  The only way that type requests are sent out to the network would be through
  routine requests, which is only referenced in this block, doesn't have a
  buffer associated with it, and is simpler to implement.
 */

static void net_proto_simple_request_loop(){
	net_proto_create_id_request_loop();
	//net_proto_create_linked_list_request_loop();
}

template
<typename T>
void net_proto_request_cleanup(T request_ptr,
			       uint64_t timestamp_micro_s){
	ASSERT(request_ptr != nullptr, P_ERR);
	if(request_ptr->get_request_time()+request_ptr->get_ttl_micro_s() >= timestamp_micro_s){
		print("request has expired, deleting", P_NOTE);
		id_api::destroy(request_ptr->id.get_id());
	}
}

#define NET_PROTO_REQUEST_CLEANUP_CREATOR(type)			\
	if(true){						\
		std::vector<id_t_> vector =			\
			id_api::cache::get(#type);		\
		for(uint64_t i = 0;i < vector.size();i++){	\
			try{					\
				net_proto_request_cleanup(	\
					PTR_DATA(vector[i],	\
						 type),		\
					timestamp_micro_s);	\
			}catch(...){}				\
		}						\
	}							\

static void net_proto_all_request_cleanup(){
	const uint64_t timestamp_micro_s =
		get_time_microseconds();
	NET_PROTO_REQUEST_CLEANUP_CREATOR(net_proto_id_request_t);
	NET_PROTO_REQUEST_CLEANUP_CREATOR(net_proto_type_request_t);
	NET_PROTO_REQUEST_CLEANUP_CREATOR(net_proto_linked_list_request_t);
}


void net_proto_requests_loop(){
	net_proto_routine_request_loop();
	net_proto_simple_request_loop();
	net_proto_all_request_cleanup();
}
