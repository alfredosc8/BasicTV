#include "net_proto.h"
#include "net_proto_request.h"
#include "net_proto_socket.h"
#include "../../id/id_api.h"

net_proto_request_standard_t::net_proto_request_standard_t(){}

net_proto_request_standard_t::~net_proto_request_standard_t(){}

void net_proto_request_standard_t::list_virtual_data(data_id_t *id){
	id->add_data(&ids, 65536);
	id->add_data(&mod_inc, 65536);
}

void net_proto_request_standard_t::set_peer_id(id_t_ peer_id_){
	peer_id = peer_id_;
}

id_t_ net_proto_request_standard_t::get_peer_id(){
	return peer_id;
}

void net_proto_request_standard_t::set_ids(std::vector<id_t_> ids_){
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
}

std::vector<id_t_> net_proto_request_standard_t::get_ids(){
	return ids;
}

std::vector<uint64_t> net_proto_request_standard_t::get_mod_inc(){
	if(mod_inc.size() != ids.size()){
		print("mod_inc is a different size than ids, assuming all old", P_ERR);
		return {}; 
	}
	return mod_inc;
}

// IDs

net_proto_id_request_t::net_proto_id_request_t() : id(this, __FUNCTION__){
	list_virtual_data(&id);
}

net_proto_id_request_t::~net_proto_id_request_t(){}

// Type

net_proto_type_request_t::net_proto_type_request_t() : id(this, __FUNCTION__){
	id.add_data(&type, 32);
}

net_proto_type_request_t::~net_proto_type_request_t(){}

// Linked list subscription

net_proto_linked_list_request_t::net_proto_linked_list_request_t() : id(this, __FUNCTION__){
}

net_proto_linked_list_request_t::~net_proto_linked_list_request_t(){
}

void net_proto_type_request_t::update_type(std::array<uint8_t, 32> type_){
	type = type_;
	set_ids(
		id_api::cache::get(
			convert::array::type::from(
				type_)));
}
