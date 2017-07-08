#include "id.h"
#include "id_api.h"
#include "../net/proto/net_proto.h"

static void id_export_raw(std::vector<uint8_t> tmp, std::vector<uint8_t> *vector){
	if(tmp.size() == 0){
		print("attempted to export completely blank data set", P_WARN);
		return;
	}
	tmp = convert::nbo::to(tmp);
	vector->insert(vector->end(), tmp.begin(), tmp.end());
}


// first half is the datum (local), second half are passed parameters
static bool should_export(std::pair<uint8_t, uint8_t> network_flags,
			  std::pair<uint8_t, uint8_t> export_flags,
			  std::pair<uint8_t, uint8_t> peer_flags){
	bool network_allows = (network_flags.second <= network_flags.first || network_flags.second == ID_DATA_RULE_UNDEF);
	bool export_allows = (export_flags.second <= export_flags.first || export_flags.second == ID_DATA_RULE_UNDEF);
	bool peer_allows = (peer_flags.second <= peer_flags.first || peer_flags.second == ID_DATA_RULE_UNDEF);
	P_V(network_allows, P_SPAM);
	P_V(export_allows, P_SPAM);
	P_V(peer_allows, P_SPAM);
	return network_allows && export_allows && peer_allows;
}

//#define ID_EXPORT(var, list) id_export_raw((uint8_t*)&var, sizeof(var), &list)
/*
  ID_EXPORT exports the size of the payload, and the payload itself in NBO
  
  The only variable not used for this is the beginning extra byte
 */
#define ID_EXPORT(var, list) id_export_raw(std::vector<uint8_t>((uint8_t*)&var, (uint8_t*)&var+sizeof(var)), &list)

std::vector<uint8_t> data_id_t::export_data(
	uint8_t flags_,
	uint8_t extra,
	uint8_t network_rules,
	uint8_t export_rules,
	uint8_t peer_rules){
	ASSERT((extra & ID_EXTRA_ENCRYPT) && (extra & ID_EXTRA_COMPRESS), P_WARN);
	if(flags_ != 0){
		print("we have no current use for a generic flag", P_WARN);
	}
	std::vector<uint8_t> retval;
	if(encrypt_blacklist_type(
		   get_id_type(id))){
		print("forcing no encryption on basis of encryption blacklist", P_SPAM);
		extra &= ~ID_EXTRA_ENCRYPT;
	}
	if(get_id_hash(id) != get_id_hash(
		   net_proto::peer::get_self_as_peer())){
		std::raise(SIGINT);
		print("can't export somebody else's modified data", P_ERR);
	}
	retval.push_back(0); // current_extra
	// ID_EXPORT(current_extra, retval);
	ID_EXPORT(id, retval);
	ID_EXPORT(modification_incrementor, retval);
	for(uint64_t i = 0;i < data_vector.size();i++){
		if(should_export(std::make_pair(data_vector[i].get_network_rules(), network_rules),
				 std::make_pair(data_vector[i].get_export_rules(), export_rules),
				 std::make_pair(data_vector[i].get_peer_rules(), peer_rules)) == false){
			print("skipping based on export rules", P_SPAM);
			continue;
		}
		std::vector<uint8_t> data_to_export;
		if(data_vector[i].get_flags() & ID_DATA_BYTE_VECTOR){
			//print("reading in a byte vector", P_SPAM);
			std::vector<uint8_t> *vector =
				(std::vector<uint8_t>*)data_vector[i].get_ptr();
			if(vector->data() == nullptr){
				//print("vector is empty, skipping", P_SPAM);
				continue;
			}
			data_to_export =
				std::vector<uint8_t>(
					(uint8_t*)vector->data(),
					(uint8_t*)vector->data()+
					(sizeof(uint8_t)*vector->size()));
		}else if(data_vector[i].get_flags() & ID_DATA_ID_VECTOR){
			//print("reading in an ID vector", P_SPAM);
			std::vector<id_t_> *vector =
				(std::vector<id_t_>*)data_vector[i].get_ptr();
			if(vector->data() == nullptr){
				//print("vector is empty, skipping", P_SPAM);
				continue;
			}
			data_to_export =
				std::vector<uint8_t>(
					(uint8_t*)vector->data(),
					(uint8_t*)vector->data()+
					(sizeof(id_t_)*vector->size()));
		}else if(data_vector[i].get_flags() & ID_DATA_EIGHT_BYTE_VECTOR){
			//print("reading in a 64-bit vector", P_SPAM);
			std::vector<uint64_t> *vector =
				(std::vector<uint64_t>*)data_vector[i].get_ptr();
			if(vector->data() == nullptr){
				//print("vector is empty, skipping", P_SPAM);
				continue;
			}
			data_to_export =
				std::vector<uint8_t>(
					(uint8_t*)vector->data(),
					(uint8_t*)vector->data()+
					(sizeof(uint8_t)*vector->size()));
		}else if(data_vector[i].get_flags() & ID_DATA_BYTE_VECTOR_VECTOR){
			// nested vectors work a bit differently
			// this code makes two (safe) assumptions
			// 1. This data will only be read from a data_vector[i] on
			// the other side with a BYTE_VECTOR_VECTOR flag
			// 2. fail-safe
			std::vector<std::vector<uint8_t> > *vector =
				(std::vector<std::vector<uint8_t> >*)data_vector[i].get_ptr();
			if(vector->data() == nullptr){
				//print("vector is empty, skipping", P_SPAM);
				continue;
			}
			transport_size_t vector_size =
				vector->size();
			data_to_export.insert(
				data_to_export.end(),
				reinterpret_cast<uint8_t*>(&vector_size),
				reinterpret_cast<uint8_t*>(&vector_size)+sizeof(transport_size_t));
			P_V(vector->size(), P_VAR);
			for(uint64_t c = 0;c < vector_size;c++){
				transport_size_t trans_size_tmp =
					(*vector)[c].size();
				data_to_export.insert(
					data_to_export.end(),
					reinterpret_cast<uint8_t*>(&trans_size_tmp),
					reinterpret_cast<uint8_t*>(&trans_size_tmp)+sizeof(transport_size_t));
				if((*vector)[c].data() == nullptr){
					//print("vector is empty, skipping", P_SPAM);
					continue;
					// don't export anything
				}
				data_to_export.insert(
					data_to_export.end(),
					(uint8_t*)(*vector)[c].data(),
					(uint8_t*)(*vector)[c].data()+trans_size_tmp);
			}
		}else{
			P_V(data_vector[i].get_length(), P_SPAM);
			data_to_export =
				std::vector<uint8_t>(
					(uint8_t*)data_vector[i].get_ptr(),
					(uint8_t*)data_vector[i].get_ptr()+
					data_vector[i].get_length());
		}
		if(data_to_export.size() == 0){
			continue;
		}
		transport_i_t trans_i = i; // size fixing
		transport_size_t trans_size = data_to_export.size();
		//P_V(trans_i, P_SPAM);
		P_V(trans_size, P_SPAM);
		ID_EXPORT(trans_i, retval);
		ID_EXPORT(trans_size, retval);
		id_export_raw(data_to_export, &retval);
	}
	ASSERT((0b11111100 & extra) == 0, P_ERR);
	P_V(extra, P_SPAM);
	if(extra & ID_EXTRA_COMPRESS){
		retval = id_api::raw::compress(retval);
	}
	if(extra & ID_EXTRA_ENCRYPT){
		retval = id_api::raw::encrypt(retval);
	}
	id_api::cache::add_data(
		retval);
	return retval;
}
