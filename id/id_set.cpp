#include "id.h"
#include "id_set.h"

/*
  TODO: allow for individual ID lookups without expanding

  TODO: actually use NBO for this
 */

const std::vector<uint8_t> seperator = {0, 0, 0, 0, 0, 0, 0, 0};

std::vector<uint8_t> compact_id_set(std::vector<id_t_> id_set){
	std::vector<std::tuple<std::vector<std::pair<uint64_t, uint8_t> >, std::array<uint8_t, 32> > > id_set_expand;
	for(uint64_t i = 0;i < id_set.size();i++){
		bool wrote = false;
		const uint64_t uuid =
			get_id_uuid(id_set[i]);
		const uint8_t type =
			get_id_type(id_set[i]);
		const std::array<uint8_t, 32> hash =
			get_id_hash(id_set[i]);
		for(uint64_t c = 0;c < id_set_expand.size();c++){
			if(std::get<1>(id_set_expand[c]) == hash){
				std::get<0>(id_set_expand[c]).push_back(
					std::make_pair(
						uuid,
						type));
				wrote = true;
				break;
			}
		}
		if(!wrote){
			id_set_expand.push_back(
				std::make_tuple(
					std::vector<std::pair<uint64_t, uint8_t> >({
							std::make_pair(
								uuid,
								type)}),
					hash));
		}
	}
	std::vector<uint8_t> retval;
	uint64_t full_uuid_count = 0; // number of unique IDs
	for(uint64_t i = 0;i < id_set_expand.size();i++){
		if(unlikely(std::get<0>(id_set_expand[i]).size() == 0)){
			print("zero sized id_set_expand, weird", P_WARN);
			continue;
		}
		std::vector<std::pair<uint64_t, uint8_t> > uuid_type = std::get<0>(id_set_expand[i]);
		std::array<uint8_t, 32> hash = std::get<1>(id_set_expand[i]);
		if(uuid_type.size() != 0){
			for(uint64_t c = 0;c < uuid_type.size();c++){
				uint8_t *uuid_type_tmp =
					(uint8_t*)&(uuid_type[c].first);
				retval.insert(
					retval.end(),
					uuid_type_tmp,
					uuid_type_tmp+sizeof(uuid_type[c].first)); // 8-bytes
				retval.push_back(
					uuid_type[c].second);
				full_uuid_count++;
			}
			retval.insert(
				retval.end(),
				seperator.begin(),
				seperator.end());
			retval.insert(
				retval.end(),
				hash.begin(),
				hash.end());
		}else{
			// shouldn't happen, internal function error
			print("not exporting hash without UUID/types", P_ERR);
		}
	}
	// pretty effective
	// print("compression ratio:" + std::to_string(((long double)retval.size())/((long double)(id_set.size())+(long double)(sizeof(id_t_)))), P_SPAM);
	if(full_uuid_count != id_set.size()){
		print("something got lost in translation with compaction", P_ERR);
	}
	return retval;
}

#define ASSERT_LENGTH(vector, size_) if(vector.size() < size_){P_V(vector.size(), P_SPAM);P_V(size_, P_SPAM);print("invalid length, corrupt id_set", P_ERR);}

static uint64_t find_first_seperator(std::vector<uint8_t> data){
	for(int64_t i = 0;i < (int64_t)(data.size())-7;i++){
		if(data[i] == 0 &&
		   data[i+1] == 0 &&
		   data[i+2] == 0 &&
		   data[i+3] == 0 &&
		   data[i+4] == 0 &&
		   data[i+5] == 0 &&
		   data[i+6] == 0 &&
		   data[i+7] == 0){
			return i;
		}
	}
	return ~(uint64_t)0;
}

/*
  TODO: sanitize the inputs, make sure segfaulting isn't as easy as it is now
 */

std::vector<id_t_> expand_id_set(std::vector<uint8_t> id_set){
	std::vector<id_t_> retval;
	std::vector<std::pair<std::vector<uint8_t>, std::array<uint8_t, 32> > > raw_read;
	uint64_t first_seperator = 0;
	while((first_seperator = find_first_seperator(id_set)) != ~(uint64_t)0){
		if(first_seperator != 0){
			std::vector<uint8_t> raw_uuid_type =
				std::vector<uint8_t>(
					id_set.begin(),
					id_set.begin()+first_seperator);
			raw_read.push_back(
				std::make_pair(
					raw_uuid_type,
					std::array<uint8_t, 32>({})));
			id_set.erase(
				id_set.begin(),
				id_set.begin()+first_seperator);
		}else{
			if(raw_read.size() == 0){
				print("can't add hash before first ID, this is wrong", P_ERR);
				// shouldn't happen
			}
			id_set.erase(
				id_set.begin(),
				id_set.begin()+8); // seperator
			memcpy(&(raw_read[raw_read.size()-1].second[0]),
			       &(id_set[0]),
			       32);
			id_set.erase(
				id_set.begin(),
				id_set.begin()+32);
		}
	}
	for(uint64_t i = 0;i < raw_read.size();i++){
		while(raw_read[i].first.size() > 0){
			id_t_ tmp_id_ = ID_BLANK_ID;
			set_id_uuid(&tmp_id_, *((uint64_t*)&raw_read[i].first[0]));
			set_id_type(&tmp_id_, raw_read[i].first[8]);
			set_id_hash(&tmp_id_, raw_read[i].second);
			retval.push_back(tmp_id_);
			raw_read[i].first.erase(
				raw_read[i].first.begin(),
				raw_read[i].first.begin()+sizeof(uint64_t)+sizeof(uint8_t));
		}
	}
	if(id_set.size() > 0){
		P_V(id_set.size(), P_WARN);
		print("leaving id_set with some data still in it, reader is broken", P_ERR);
	}
	return retval;
}

#undef ASSERT_LENGTH
