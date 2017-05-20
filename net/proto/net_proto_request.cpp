#include "net_proto.h"
#include "net_proto_request.h"
#include "net_proto_socket.h"
#include "../../id/id_api.h"

net_proto_request_bare_t::net_proto_request_bare_t(){}

net_proto_request_bare_t::~net_proto_request_bare_t(){}

void net_proto_request_bare_t::list_bare_virtual_data(data_id_t *id){
	id->add_data_id(&sender_peer_id, 1);
	id->add_data_id(&receiver_peer_id, 1);
	id->add_data_raw((uint8_t*)&request_time, 8);
}

id_t_ net_proto_request_bare_t::get_sender_peer_id(){
	return sender_peer_id;
}

void net_proto_request_bare_t::set_sender_peer_id(id_t_ sender_peer_id_){
	sender_peer_id = sender_peer_id_;
}

id_t_ net_proto_request_bare_t::get_receiver_peer_id(){
	return receiver_peer_id;
}

void net_proto_request_bare_t::set_receiver_peer_id(id_t_ receiver_peer_id_){
	receiver_peer_id = receiver_peer_id_;
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
	id.noexp_all_data();
}

net_proto_id_request_t::~net_proto_id_request_t(){}

// Type

net_proto_type_request_t::net_proto_type_request_t() : id(this, TYPE_NET_PROTO_TYPE_REQUEST_T){
	list_set_virtual_data(&id);
	list_bare_virtual_data(&id);
	id.add_data_raw(&type, sizeof(type));
	id.noexp_all_data();
}

net_proto_type_request_t::~net_proto_type_request_t(){}

// Linked list subscription

net_proto_linked_list_request_t::net_proto_linked_list_request_t() : id(this, TYPE_NET_PROTO_LINKED_LIST_REQUEST_T){
	list_bare_virtual_data(&id);
	id.noexp_all_data();
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
	data_id_t *id =
		PTR_ID(curr_id, );
	if(id == nullptr){
		return;
	}
	curr_id = id->get_next_linked_list();
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
