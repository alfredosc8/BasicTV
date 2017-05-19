#include "escape.h"
#include "util.h"

// just adds size for decoding

/*
  TODO: convert things to use these functions
 */

std::vector<uint8_t> escape_vector(
	std::vector<uint8_t> vector,
	char escape_char){
	for(uint64_t i = 0;i < vector.size();i++){
		if(unlikely(vector[i] == escape_char)){
			vector.insert(
				vector.begin()+i,
				escape_char);
			i++;
		}
	}
	// insert the escaped size as a prefix
	uint32_t escaped_length =
		NBO_32(vector.size());
	vector.insert(
		vector.begin(),
		((uint8_t*)&escaped_length),
		((uint8_t*)&escaped_length)+4);
	vector.insert(
		vector.begin(),
		escape_char);
	return vector;
}

static uint64_t pos_of_next_true_escape(std::vector<uint8_t> vector,
					char escape_char){
	for(uint64_t i = 1;i < vector.size()-1;i++){
		if(unlikely(vector[i-1] != escape_char &&
			    vector[i+0] == escape_char &&
			    vector[i+1] != escape_char)){
			return i;
		}
	}
	std::raise(SIGINT);
	return vector.size();
}

std::pair<std::vector<uint8_t>, std::vector<uint8_t> > unescape_vector(
	std::vector<uint8_t> vector,
	char escape_char){
	std::pair<std::vector<uint8_t>, std::vector<uint8_t> > retval;
	if(vector.size() <= 5){ // escape char + 32-bit length
		print("vector is too small to contain metadata", P_NOTE);
		return std::make_pair(
			std::vector<uint8_t>({}),
			vector);
	}
	vector.erase(vector.begin());
	uint32_t escaped_length = 0;
	memcpy(&escaped_length,
	       vector.data(),
	       4);
	vector.erase(vector.begin(),
		     vector.begin()+4);
	escaped_length = NBO_32(escaped_length);
	// P_V(escaped_length, P_SPAM);
	std::vector<uint8_t> payload;
	if(escaped_length > vector.size()){
		// P_V_B(escaped_length, P_NOTE);
		// P_V_B(vector.size(), P_NOTE);
		print("escaped_length is longer than actual data, not parsing", P_NOTE);
	}else{
		payload = std::vector<uint8_t>(
			vector.begin(),
			vector.begin()+escaped_length);
		for(uint64_t i = 0;i < payload.size();i++){
			/*
			  we can safely assume that escape chars
			  exist in pairs because of pos_of_next_true_escape
			*/
			if(payload[i] == escape_char){
				payload.erase(payload.begin()+i);
				i--;
			}
		}
	}
	return std::make_pair(
		payload,
		std::vector<uint8_t>(
			vector.begin()+payload.size(),
			vector.end()));
}

std::pair<std::vector<std::vector<uint8_t> >, std::vector<uint8_t> > unescape_all_vectors(
	std::vector<uint8_t> vector,
	char escape_char){
	// list of all exported vectors and the extra
	std::pair<std::vector<std::vector<uint8_t> >, std::vector<uint8_t> > retval;
	try{
		uint64_t old_size = 0;
		while(vector.size() != old_size){
			old_size = vector.size();
			std::pair<std::vector<uint8_t>, std::vector<uint8_t> > tmp =
				unescape_vector(vector, escape_char);
			if(tmp.first.size() != 0){
				retval.first.push_back(
					tmp.first);
			}
			vector = tmp.second;
		}
	}catch(...){}
	if(vector.size() != 0){
		print("non-zero vector size at end of bulk vector unescaping, returning as cruft", P_SPAM);
		// completely normal in typical use, but our tests shouldn't
		// have this kind of behavior
	}
	return retval;
}
