#include "id.h"
#include "id_api.h"

static void id_import_raw(uint8_t* var, uint8_t flags, uint64_t size, std::vector<uint8_t> *vector){
	if(size == 0){
		return;
	}
	if(flags & ID_DATA_BYTE_VECTOR){
		std::vector<uint8_t> *local_vector =
			(std::vector<uint8_t>*)var;
		// not the fastest
		local_vector->clear();
		local_vector->insert(local_vector->end(),
				   size,
				   0);
		var = local_vector->data();
	}else if(flags & ID_DATA_EIGHT_BYTE_VECTOR){
		std::vector<uint64_t> *local_vector =
			(std::vector<uint64_t>*)var;
		local_vector->clear();
		local_vector->insert(local_vector->end(),
				     size,
				     0);
		var = (uint8_t*)local_vector->data();
	}else if(flags & ID_DATA_ID_VECTOR){
		std::vector<id_t_> *local_vector =
			(std::vector<id_t_>*)var;
		local_vector->clear();
		local_vector->insert(local_vector->end(),
				     size,
				     ID_BLANK_ID);
		var = (uint8_t*)local_vector->data();
	}else{
		// sanity check
		//P_V(flags, P_SPAM);
		//P_V(size, P_SPAM);
		memset(var, 0, size);
	}
	if(vector->size() < size){
		P_V(flags, P_NOTE);
		P_V(size, P_NOTE);
		P_V(vector->size(), P_NOTE);
		print("not enough runway to export information, see where it went off track", P_ERR);
	}
	memcpy(var, vector->data(), size);
	vector->erase(vector->begin(), vector->begin()+size);
	convert::nbo::from((uint8_t*)var, size);
}

/*
  TODO: I mean seriously, clean this up
*/
#define ID_IMPORT(var) id_import_raw((uint8_t*)&var, 0, sizeof(var), &data)

void data_id_t::import_data(std::vector<uint8_t> data){
	data = id_api::raw::decrypt(data);
	data = id_api::raw::decompress(data);
	id_t_ trans_id = ID_BLANK_ID;
	uint8_t extra = 0;
	ID_IMPORT(extra);
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
	while(data.size() > sizeof(transport_i_t) + sizeof(transport_size_t)){
		ID_IMPORT(trans_i);
		ID_IMPORT(trans_size);
		// P_V(trans_i, P_SPAM);
		// P_V(trans_size, P_SPAM);
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
		}else if(unlikely(trans_size > data_vector[trans_i].get_length())){
			print("fetched size is greater than the local version", P_ERR);
			return;
		}
		id_import_raw((uint8_t*)data_vector[trans_i].get_ptr(),
			      data_vector[trans_i].get_flags(),
			      trans_size,
			      &data);
	}
}
