#include "id.h"
#include "id_api.h"
#include "id_import.h"

static void id_import_raw_real(
	std::vector<uint8_t> *vector,
	uint8_t *var,
	uint8_t flags,
	uint32_t size,
	bool nbo = true){
	if(vector->size() < size){
		P_V(flags, P_NOTE);
		P_V(size, P_NOTE);
		P_V(vector->size(), P_NOTE);
		HANG();
		print("not enough runway to import information, see where it went off track", P_ERR);
	}
	memcpy(var, vector->data(), size);
	vector->erase(vector->begin(), vector->begin()+size);
	if(nbo){
		convert::nbo::from((uint8_t*)var, size);
	}
}

static void id_import_raw(
	uint8_t *var,
	uint8_t flags, 
	uint64_t size,
	std::vector<uint8_t> *vector){
	if(flags & ID_DATA_BYTE_VECTOR){
		print("reading as a byte vector", P_SPAM);
		std::vector<uint8_t> *local_vector =
			(std::vector<uint8_t>*)var;
		// not the fastest
		local_vector->clear();
		local_vector->insert(
			local_vector->end(),
			size,
			0);
		id_import_raw_real(
			vector,
			local_vector->data(),
			flags,
			size);
	}else if(flags & ID_DATA_EIGHT_BYTE_VECTOR){
		print("reading as an eight byte vector", P_SPAM);
		std::vector<uint64_t> *local_vector =
			(std::vector<uint64_t>*)var;
		local_vector->clear();
		local_vector->insert(
			local_vector->end(),
			size,
			0);
		id_import_raw_real(
			vector,
			reinterpret_cast<uint8_t*>(local_vector->data()),
			flags,
			size);
	}else if(flags & ID_DATA_ID_VECTOR){
		print("reading as an ID vector", P_SPAM);
		std::vector<id_t_> *local_vector =
			(std::vector<id_t_>*)var;
		local_vector->clear();
		local_vector->insert(
			local_vector->end(),
			size,
			ID_BLANK_ID);
		id_import_raw_real(
			vector,
			reinterpret_cast<uint8_t*>(local_vector->data()),
			flags,
			size);
	}else if(flags & ID_DATA_BYTE_VECTOR_VECTOR){
		print("reading as a byte vector vector", P_SPAM);
		std::vector<std::vector<uint8_t> > *local_vector =
			(std::vector<std::vector<uint8_t> >*)(var);
		local_vector->clear();
		std::vector<uint8_t> global_vector =
			std::vector<uint8_t>(
				vector->begin(),
				vector->begin()+size);
		vector->erase(
			vector->begin(),
			vector->begin()+size);
		global_vector =
			convert::nbo::from(
				global_vector);
		transport_size_t elem_count = 0;
		id_import_raw_real(
			&global_vector,
			reinterpret_cast<uint8_t*>(&elem_count),
			0,
			sizeof(transport_size_t),
			false);
		P_V(elem_count, P_VAR);
		for(uint64_t i = 0;i < elem_count;i++){
			transport_size_t trans_size = 0;
			id_import_raw_real(
				&global_vector,
				reinterpret_cast<uint8_t*>(&trans_size),
				0, 
				sizeof(transport_size_t),
				false);
			std::vector<uint8_t> tmp(trans_size, 0);
			id_import_raw_real(
				&global_vector,
				reinterpret_cast<uint8_t*>(tmp.data()),
				0,
				trans_size,
				false);
			local_vector->push_back(
				tmp);
		}
	}else{
		print("using a simple read", P_SPAM);
		P_V(size, P_VAR);
		memset(var, 0, size);
		id_import_raw_real(
			vector,
			var,
			flags,
			size);
	}
}

/*
  TODO: I mean seriously, clean this up
*/
#define ID_IMPORT(var) id_import_raw((uint8_t*)&var, 0, sizeof(var), &data)

void data_id_t::import_data(std::vector<uint8_t> data){
	id_t_ trans_id = ID_BLANK_ID;
	uint8_t extra =
		data[0];	
	ASSERT((0b11111100 & extra) == 0, P_ERR);
	if((extra & ID_EXTRA_ENCRYPT) &&
	   encrypt_blacklist_type(
		   get_id_type(id))){
		print("contradiction between encrypt_blacklist_type and extra byte", P_WARN);
		HANG();
	}
	if(extra & ID_EXTRA_ENCRYPT){
		data = id_api::raw::decrypt(data);
	}
	if(extra & ID_EXTRA_COMPRESS){
		data = id_api::raw::decompress(data);
	}	
	ID_IMPORT(extra); // just to remove it
	ID_IMPORT(trans_id);
	ID_IMPORT(modification_incrementor);
	// P_V_S(convert::array::id::to_hex(trans_id), P_SPAM);
	// P_V_S(convert::type::from(get_id_type(trans_id)), P_SPAM);
	// P_V_B(extra, P_SPAM);
	if(get_id_type(trans_id) != get_id_type(id)){
		P_V(get_id_type(trans_id), P_WARN);
		P_V(get_id_type(id), P_WARN);
		print("can't import a mis-matched type", P_ERR);
	}
	set_id(trans_id);
	transport_i_t trans_i = 0;
	transport_size_t trans_size = 0;
	uint8_t network_rules_tmp = 0;
	uint8_t export_rules_tmp = 0;
	uint8_t peer_rules_tmp = 0;
	while(data.size() > sizeof(transport_i_t) + sizeof(transport_size_t)){
		ID_IMPORT(trans_i);
		ID_IMPORT(network_rules_tmp);
		ID_IMPORT(export_rules_tmp);
		ID_IMPORT(peer_rules_tmp);
		ID_IMPORT(trans_size);
		P_V(trans_i, P_SPAM);
		P_V(trans_size, P_SPAM);
		const bool valid_entry =
			trans_i < data_vector.size();
		if(unlikely(!valid_entry)){
			P_V(trans_i, P_WARN);
			print("invalid i entry, probably came from a new version", P_ERR);
			return;
		}else if(unlikely(data_vector[trans_i].get_ptr() == nullptr)){
			print("cannot write to nullptr entry", P_WARN);
			return;
		}
		if(unlikely(trans_size > data.size())){
			print("fetched size is greater than working data", P_ERR);
			return;
		}else if(unlikely(trans_size > data_vector[trans_i].get_length_vector().at(0))){
			print("fetched size is greater than the local version", P_ERR);
			return;
		}
		id_import_raw(
			reinterpret_cast<uint8_t*>(data_vector[trans_i].get_ptr()),
			data_vector[trans_i].get_flags(),
			trans_size,
			&data);
		// only update ID rules if we made it this far
		data_vector[trans_i].set_network_rules(
			network_rules_tmp);
		data_vector[trans_i].set_export_rules(
			export_rules_tmp);
		data_vector[trans_i].set_peer_rules(
			peer_rules_tmp);
	}
}
