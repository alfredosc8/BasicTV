#include "id.h"
#include "id_set.h"

/*
  TODO: allow for individual ID lookups without expanding
 */

const std::vector<uint8_t> seperator = {0, 0, 0, 0, 0, 0, 0, 0};

std::vector<uint8_t> compact_id_set(std::vector<id_t_> id_set){
	std::vector<std::pair<std::vector<uint64_t>, std::array<uint8_t, 32> > > id_set_expand;
	for(uint64_t i = 0;i < id_set.size();i++){
		bool wrote = false;
		const uint64_t uuid =
			get_id_uuid(id_set[i]);
		const std::array<uint8_t, 32> hash =
			get_id_hash(id_set[i]);
		for(uint64_t c = 0;c < id_set_expand.size();c++){
			if(id_set_expand[c].second == hash){
				id_set_expand[c].first.push_back(
					uuid);
				wrote = true;
				break;
			}
		}
		if(!wrote){
			id_set_expand.push_back(
				std::make_pair(
					std::vector<uint64_t>(uuid),
					hash));
		}
	}
	/*
	  SPEC:
	  UUIDs are back to back, and then eight zeroes denotes the start of the
	  corresponding hash. Lengths are fixed, so this is pretty efficient
	  Because of statistics and other nonsense, the number of occurances
	  is encoded as well

	  TODO: It might be more effective to force all individual bytes of the
	  UUID to be zero, and only have one zero denote the start of the hash
	  (although re-computes would be more common...)
	 */
	std::vector<uint8_t> retval;
	for(uint64_t i = 0;i < id_set_expand.size();i++){
		if(unlikely(id_set_expand[i].first.size() == 0)){
			print("zero sized id_set_expand, weird", P_WARN);
			continue;
		}
		retval.insert(
			retval.end(),
			(uint8_t*)id_set_expand[i].first.data(),
			((uint8_t*)id_set_expand[i].first.data())+(id_set_expand[i].first.size()*8));
		retval.insert(
			retval.end(),
			seperator.begin(),
			seperator.end());
		retval.insert(
			retval.end(),
			id_set_expand[i].second.begin(),
			id_set_expand[i].second.end());
	}
	P_V(retval.size(), P_NOTE);
	return retval;
}

std::vector<id_t_> expand_id_set(std::vector<uint8_t> id_set){
	std::vector<id_t_> retval;
	while(likely(id_set.size() > 0)){
		std::vector<uint64_t> uuid_vector;
		while(likely(id_set.size()-(uuid_vector.size()*8) >= 8 &&
			     memcmp(seperator.data(), id_set.data()+uuid_vector.size()*8, 8) != 0)){
			uuid_vector.push_back(
				*(((uint64_t*)id_set.data()))+uuid_vector.size());
		}
		id_set.erase(
			id_set.begin(),
			id_set.begin()+(uuid_vector.size()*8));
		if(likely(id_set.size() >= 8)){
			id_set.erase(
				id_set.begin(),
				id_set.begin()+8);
		}else{
			print("corruption, this is bad", P_ERR);
		}
		if(likely(id_set.size() >= 32)){
			std::array<uint8_t, 32> hash;
			memcpy(&(hash[0]), id_set.data(), 32);
			id_set.erase(
				id_set.begin(),
				id_set.begin()+32);
			for(uint64_t i = 0;i < uuid_vector.size();i++){
				id_t_ tmp_id;
				set_id_hash(&tmp_id, hash);
				set_id_uuid(&tmp_id, uuid_vector[i]);
				retval.push_back(tmp_id);
			}
		}else{
			print("invalid compact id set", P_WARN);
		}
	}
	return retval;
}
