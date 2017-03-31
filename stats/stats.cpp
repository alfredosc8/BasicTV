#include "stats.h"

stat_sample_set_t::stat_sample_set_t() : id(this, __FUNCTION__){
	id.add_data((std::vector<uint8_t>*)&size_set, ~(uint32_t)0);
	id.add_data((std::vector<uint8_t>*)&set, ~(uint32_t)0);
	id.add_data(&entry_size, 4);
}

stat_sample_set_t::~stat_sample_set_t(){}

void stat_sample_set_t::reset(std::vector<uint8_t> size_set_){
	// also used to derive dimension count
	size_set = size_set_;
	entry_size = 0;
	for(uint64_t i = 0;i < size_set.size();i++){
		entry_size += size_set[i];
	}
}

// require all dimensions at one time

void stat_sample_set_t::add(std::vector<std::vector<uint8_t> > datum){
	if(unlikely(datum.size() != size_set.size())){
		print("dimensional size mismatch in stat add", P_ERR);
	}
	for(uint64_t i = 0;i < size_set.size();i++){
		if(likely(datum[i].size() == size_set[i])){
			set.insert(
				set.end(),
				datum[i].begin(),
				datum[i].end());
		}else{
			print("entry size mismatch in stat add", P_ERR);
		}
	}
}

void stat_sample_set_t::truncate_to_size(uint64_t size_){
	const int64_t entries_to_remove =
		(set.size()/entry_size)-size_;
	if(entries_to_remove > 0){
		set.erase(
			set.begin(),
			set.begin()+(entries_to_remove*entry_size));
	}
}
