#include "id_cache.h"
#include "id_disk.h"
#include "id_api.h"
#include "id.h"
#include "../encrypt/encrypt.h"
#include "../compress/compress.h"
/*
  TODO: when threading becomes a thing, just offset everything
  to a vector to increment and decrement and periodically let
  up on the locks to allow reading and writing.
 */

// state of any exported ID is the first byte (the 'extra' byte)
// default state of exported data (from export_data) is entirely
// blank, and needs to be encrypted and stored before being sent
// back out (to prevent possibly un-needed encryptions and
// decryptions for information that never leaves disk, as well as
// various soon-to-be implemented exporting rules).

/*
  TODO: store this better for faster lookups
 */

static std::vector<std::vector<uint8_t> > cache_state;

void id_api::cache::hint_increment_id(id_t_ id){
	if(PTR_ID(id, ) != nullptr){
		return;
	}
	for(uint64_t i = 0;i < cache_state.size();i++){
		if(id_api::raw::fetch_id(cache_state[i]) == id){
			switch(id_api::raw::fetch_extra(cache_state[i])){
			case ID_CACHE_STATE_DECRYPT_DECOMP:
				id_api::array::add_data(cache_state[i]);
				// no need to change extra byte, as a search for
				// pre-imported data happens earlier
				break;
			case ID_CACHE_STATE_DECRYPT_COMP:
				cache_state[i] =
					id_api::raw::decompress(
						cache_state[i]);
				break;
			case ID_CACHE_STATE_ENCRYPT_COMP:
				cache_state[i] =
					id_api::raw::decrypt(
						cache_state[i]);
				break;
			default:
				print("cache entry is in an invalid state", P_WARN);
			
			}
		}
	}
	std::vector<id_t_> disk_indexes =
		id_api::cache::get(
			TYPE_ID_DISK_INDEX_T);
	for(uint64_t i = 0;i < disk_indexes.size();i++){
		id_disk_index_t *disk_index =
			PTR_DATA(disk_indexes[i],
				 id_disk_index_t);
		if(disk_index == nullptr){
			print("disk_index is a nullptr", P_WARN);
			continue;
		}
		if(disk_index->id_on_disk(id)){
			
		}
	}
}

/*
  Allows storing data we can't decrypt yet safely
*/

void id_api::cache::add_data(std::vector<uint8_t> data){
	cache_state.push_back(
		data);
	id_t_ raw_id =
		id_api::raw::fetch_id(data);
	id_api::cache::add(
		raw_id,
		get_id_type(raw_id));
}

void id_api::cache::load_id(id_t_ id){
	// we can assume we only have one ID safely (only harm is not
	// reading data, it would be self-destructive to recreate UUID)
	for(uint64_t i = 0;i < cache_state.size();i++){
		const id_t_ cache_state_id =
			id_api::raw::fetch_id(cache_state[i]);
		P_V_S(convert::array::id::to_hex(cache_state_id), P_VAR);
		P_V_S(convert::array::id::to_hex(id), P_VAR);
		if(unlikely(id_api::raw::fetch_id(cache_state[i]) == id)){
			print("found ID in cache_state vector", P_SPAM);
			id_api::array::add_data(cache_state[i]);
			cache_state.erase(
				cache_state.begin()+i);
		}else{
			P_V_S(convert::array::id::to_hex(cache_state_id), P_VAR);
			P_V_S(convert::array::id::to_hex(id), P_VAR);
		}
	}
}
