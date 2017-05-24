#include "escape.h"
#include "util.h"

// just adds size for decoding

/*
  TODO: convert things to use these functions
 */

std::vector<uint8_t> escape_vector(
	std::vector<uint8_t> vector,
	uint8_t escape_char){
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
		&(escape_char),
		&(escape_char)+1);
	if(vector[0] != escape_char){
		print("escape char isn't at beginning, I need to re-learn C++ vector rules", P_ERR);
	}
	return vector;
}

static uint64_t pos_of_next_true_escape(std::vector<uint8_t> vector,
					uint8_t escape_char){
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
	uint8_t escape_char){
	std::pair<std::vector<uint8_t>, std::vector<uint8_t> > retval;
	if(vector.size() <= 5){ // escape char + 32-bit length
		//print("vector is too small to contain metadata", P_SPAM);
		return std::make_pair(
			std::vector<uint8_t>({}),
			vector);
	}
	if(vector[0] != escape_char){
		// just skip to the first valid escape char (singular)
		print("data to unescape doesn't start with escape char, better ways to handle this", P_ERR);
	}
	uint32_t escaped_length = 0;
	memcpy(&escaped_length,
	       &(vector[1]), // escape char is first byte
	       4);
	escaped_length = NBO_32(escaped_length);
	std::vector<uint8_t> payload;
	if(escaped_length <= vector.size()){
		vector.erase(
			vector.begin(),
			vector.begin()+sizeof(uint32_t)+sizeof(uint8_t));
		// length of payload and escape char
		payload = std::vector<uint8_t>(
			vector.begin(),
			vector.begin()+escaped_length);
		vector.erase(
			vector.begin(),
			vector.begin()+escaped_length);
	}
	for(uint64_t i = 0;i < payload.size();i++){
		if(payload[i] == escape_char){
			payload.erase(payload.begin()+i);
			while(payload[i] == escape_char){
				i++;
			}
		}
	}
	return std::make_pair(
		payload, vector);
}

std::pair<std::vector<std::vector<uint8_t> >, std::vector<uint8_t> > unescape_all_vectors(
	std::vector<uint8_t> vector,
	uint8_t escape_char){
	// list of all exported vectors and the extra
	std::pair<std::vector<std::vector<uint8_t> >, std::vector<uint8_t> > retval;
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
	if(vector.size() != 0){
		print("non-zero vector size at end of bulk vector unescaping, returning as cruft", P_SPAM);
	}
	retval.second = vector;
	return retval;
}
