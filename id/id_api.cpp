#include "id.h"
#include "id_api.h"
#include "id_disk.h"

#include "../tv/tv_dev_audio.h"
#include "../tv/tv_dev_video.h"
#include "../tv/tv_frame_audio.h"
#include "../tv/tv_frame_video.h"
#include "../tv/tv_frame_caption.h"
#include "../tv/tv_channel.h"
#include "../tv/tv_window.h"
#include "../tv/tv_menu.h"
#include "../tv/tv_item.h"
#include "../net/proto/inbound/net_proto_inbound_data.h"
#include "../net/proto/outbound/net_proto_outbound_data.h"
#include "../net/proto/net_proto_con_req.h"
#include "../net/proto/net_proto_peer.h"
#include "../net/proto/net_proto_con_req.h"
#include "../net/proto/net_proto.h"
#include "../net/proto/net_proto_socket.h"
#include "../net/net.h"
#include "../input/input.h"
#include "../input/input_ir.h"
#include "../settings.h"
#include "../system.h"
#include "../cryptocurrency.h"
#include "../compress/compress.h"
#include "../encrypt/encrypt.h"

static std::vector<data_id_t*> id_list;
static std::vector<std::pair<std::vector<id_t_>, type_t_ > > type_cache;

static data_id_t *id_find(id_t_ id){
	for(uint64_t i = 0;i < id_list.size();i++){
		if(unlikely(id_list[i]->get_id(true) == id)){
			return id_list[i];
		}
	}
	print("Couldn't find ID", P_NOTE);
	return nullptr;
}

/*
  TODO: make seperate functions for fast lookups only (ideally for use with the
  stats library, since speed is more important than actually having the data)
 */

data_id_t *id_api::array::ptr_id(id_t_ id,
				 std::string type,
				 uint8_t flags){
	if(id == ID_BLANK_ID){
		return nullptr;
	}
	data_id_t *retval = id_find(id);
	if(retval == nullptr){
		if(flags & ID_LOOKUP_FAST){
			// TODO: add more levels?
			print("fast lookup, not querying disk", P_NOTE);
			return nullptr;
		}
		print("attempting import from disk", P_SPAM);
		try{
			id_disk_api::load(id);
		}catch(...){
			print("querying network for data", P_SPAM);
			net_proto::request::add_id(id);
		}
	}else if(retval->get_type() != type && type != ""){
		print("type-id mismatch", P_SPAM);
		return nullptr;
	}
	return retval;
}

data_id_t *id_api::array::ptr_id(id_t_ id,
				 type_t_ type,
				 uint8_t flags){
	return ptr_id(id, convert::type::from(type), flags);
}

void *id_api::array::ptr_data(id_t_ id,
			      std::string type,
			      uint8_t flags){
	data_id_t *id_ptr = ptr_id(id, type, flags);
	if(id_ptr == nullptr){
		return nullptr;
	}
	return id_ptr->get_ptr();
}

void *id_api::array::ptr_data(id_t_ id,
			      type_t_ type,
			      uint8_t flags){
	return ptr_data(id, convert::type::from(type), flags);
}

void id_api::array::add(data_id_t *ptr){
	id_list.push_back(ptr);
}

void id_api::array::del(id_t_ id){
	for(uint64_t i = 0;i < id_list.size();i++){
		if(id_list[i]->get_id() == id){
			id_list.erase(id_list.begin()+i);
			return;
		}
	}
	print("cannot find ID in list", P_ERR);
}

#define CHECK_TYPE(a) if(convert::type::from(type) == #a){print("importing data", P_NOTE);a *tmp = new a;tmp->id.import_data(data_);return tmp->id.get_id();}

/*
  General purpose reader, returns the ID of the new information.
  The only current use of the return value is for associating sockets with
  data requests.
 */

id_t_ id_api::array::add_data(std::vector<uint8_t> data_, bool raw){
	id_t_ id = ID_BLANK_ID;
	type_t_ type = 0;
	try{
		id = id_api::raw::fetch_id(data_);
		type = get_id_type(id_api::raw::fetch_id(data_));
		P_V_S(convert::array::id::to_hex(id), P_SPAM);
		P_V_S(convert::type::from(type), P_SPAM);
	}catch(std::exception &e){
		print("can't import id and type from raw data", P_ERR);
		throw e;
	}
	std::vector<id_t_> tmp_type_cache =
		id_api::cache::get(type);
	for(uint64_t i = 0;i < tmp_type_cache.size();i++){
		if(tmp_type_cache[i] == id){
			if(raw){
				data_ =
					encrypt_api::decrypt(
						data_,
						encrypt_api::search::pub_key_from_hash(
							get_id_hash(
								id)));
				data_ =
					compressor::decompress(
						data_);
			}
			PTR_ID(id, )->import_data(data_);
			return tmp_type_cache[i];
		}
	}
	CHECK_TYPE(tv_channel_t);
	CHECK_TYPE(tv_frame_audio_t);
	CHECK_TYPE(tv_frame_video_t);
	CHECK_TYPE(net_proto_peer_t);
	CHECK_TYPE(net_proto_id_request_t);
	CHECK_TYPE(net_proto_type_request_t);
	CHECK_TYPE(net_proto_linked_list_request_t);
	CHECK_TYPE(net_proto_con_req_t);
	CHECK_TYPE(encrypt_pub_key_t);
	CHECK_TYPE(encrypt_priv_key_t);
	print("type isn't valid", P_CRIT);
	return id;
}

#undef CHECK_TYPE

id_t_ id_api::array::fetch_one_from_hash(type_t_ type, std::array<uint8_t, 32> sha_hash){
	std::vector<id_t_> all_of_type =
		id_api::cache::get(type);
	for(uint64_t i = 0;i < all_of_type.size();i++){
		if(get_id_hash(all_of_type[i]) == sha_hash){
			return all_of_type[i];
		}
	}
	return ID_BLANK_ID;
}

std::vector<id_t_> id_api::sort::fingerprint(std::vector<id_t_> tmp){
	// TODO: actually get the finerprints
	return tmp;
}

// Type cache code

static std::vector<id_t_> *get_type_cache_ptr(type_t_ tmp){
	for(uint64_t i = 0;i < type_cache.size();i++){
		if(unlikely(type_cache[i].second == tmp)){
			return &type_cache[i].first;
		}
	}
	type_cache.push_back(std::make_pair(std::vector<id_t_>({}), tmp));
	print("type cache of " + convert::type::from(tmp) + " is " + std::to_string(type_cache.size()), P_SPAM);
	return &type_cache[type_cache.size()-1].first;
}

void id_api::cache::add(id_t_ id, type_t_ type){
	std::vector<id_t_> *vector =
		get_type_cache_ptr(type);
	vector->push_back(id);
	vector = nullptr;
}

void id_api::cache::add(id_t_ id, std::string type){
	add(id, convert::type::to(type));
}

void id_api::cache::del(id_t_ id, type_t_ type){
	std::vector<id_t_> *vector =
		get_type_cache_ptr(type);
	for(uint64_t i = 0;i < vector->size();i++){
		if((*vector)[i] == id){
			vector->erase(vector->begin()+i);
			vector = nullptr;
			return;
		}
	}
	vector = nullptr;
	print("couldn't find ID in type cache", P_ERR);
}

void id_api::cache::del(id_t_ id, std::string type){
	del(id, convert::type::to(type));
}

std::vector<id_t_> id_api::cache::get(type_t_ type){
	return *get_type_cache_ptr(type);
}

std::vector<id_t_> id_api::cache::get(std::string type){
	return get(convert::type::to(type));
}

// TODO: make a forwards and backwards function too

std::vector<id_t_> id_api::linked_list::get_forward_linked_list(id_t_ id){
	std::vector<id_t_> retval;
	while(id != ID_BLANK_ID){
		data_id_t *id_ptr = PTR_ID(id, );
		retval.push_back(id);
		id = id_ptr->get_next_linked_list();
	}
	return retval;
}

/*
  TODO: when more than the immediate neighbor can be stored, let this
  function do that and set the number as a parameter
 */

void id_api::linked_list::link_vector(std::vector<id_t_> vector){
	switch(vector.size()){
	case 0:
		print("vector is empty", P_NOTE);
		return;
	case 1:
		print("vector has one entry, can't link to anything" , P_NOTE);
		return;
	case 2:
		PTR_ID(vector[0], )->set_next_linked_list(vector[1]);
		PTR_ID(vector[1], )->set_next_linked_list(vector[0]);
		return;
	}
	data_id_t *first = PTR_ID(vector[0], );
	if(first != nullptr){
		first->set_next_linked_list(vector[1]);
	}else{
		print("first entry is a nullptr, this can be fixed, but I didn't bother and this shouldn't happen anyways", P_ERR);
	}
	for(uint64_t i = 1;i < vector.size()-1;i++){
		data_id_t *id = PTR_ID(vector[i], );
		id->set_next_linked_list(vector[i+1]);
		id->set_prev_linked_list(vector[i-1]);
	}
	PTR_ID(vector[vector.size()-1], )->set_prev_linked_list(
		vector[vector.size()-2]);
}

uint64_t id_api::linked_list::distance_fast(id_t_ linked_list_id, id_t_ target_id){
	if(unlikely(linked_list_id == target_id)){
		return 0;
	}
	data_id_t *tmp_ptr =
		PTR_ID_FAST(linked_list_id, );
	if(tmp_ptr == nullptr){
		print("linked list is invalid", P_ERR);
	}
	// Make this bi-direcitonal
	std::vector<id_t_> scanned_ids;
	while(std::find(scanned_ids.begin(),
			scanned_ids.end()-1,
			scanned_ids[scanned_ids.size()-1]) == scanned_ids.end()){
		tmp_ptr =
			PTR_ID_FAST(tmp_ptr->get_next_linked_list(), );
		if(tmp_ptr == nullptr){
			return 0;
		}
		
	}
	print("finish implementing this", P_CRIT);
	return 0;
}

std::vector<id_t_> id_api::get_all(){
	std::vector<id_t_> retval;
	for(uint64_t i = 0;i < id_list.size();i++){
		if(id_list[i] != nullptr){
			retval.push_back(id_list[i]->get_id());
		}
	}
	return retval;
}

// make this less stupid

#define DELETE_TYPE_2(a) if(ptr->get_type() == #a){print("deleting " + (std::string)(#a), P_SPAM);delete (a*)ptr->get_ptr();return;}

// refactor

/*
  Periodically update with rgrep
 */


/*
  TODO: when I start to care about Windows development, start using their
  inferior directory seperators

  TODO: add the modification incrementor to the exported information for
  faster lookups and sanity checks against existing data. Incrementor would
  go up by one every time the version in RAM changed, and will export it to 
  disk every time we were ahead, and imported it every time it was ahead,
  very nice for cluster computing, but that's not a high priority.
 */

static bool id_api_should_write_to_disk_mod_inc(id_t_ id_){
	// std::string directory =
	// 	id_disk_api::get_filename(id_);
	// directory =
	// 	directory.substr(
	// 		0,
	// 		directory.find_last_of('/'));
	// P_V_S(directory, P_SPAM);
	// std::vector<std::string> find_output =
	// 	system_handler::find_all_files(
	// 		directory,
	// 		convert::array::id::to_hex(id_));
	// // multiple files might exist for some stupid and very broken reason,
	// // so read the entire vector like this
	// uint64_t largest_mod_inc = 0;
	// for(uint64_t i = 0;i < find_output.size();i++){
	// 	try{
	// 		uint64_t mod_inc =
	// 			std::stoi(
	// 				find_output[i].substr(
	// 					find_output[i].find_last_of("_")+1,
	// 					find_output[i].size()));
	// 		if(mod_inc > largest_mod_inc){
	// 			largest_mod_inc = mod_inc;
	// 		}
	// 	}catch(...){}
	// }
	// data_id_t *id_tmp =
	// 	PTR_ID(id_, );
	// if(id_tmp == nullptr){
	// 	print("id_tmp is a nullptr, probably passing stale info", P_WARN);
	// 	return false;
	// 	// not a big deal, but shouldn't happen, probably passing around
	// 	// stale information at this point
	// }
	// return id_tmp->get_mod_inc() > largest_mod_inc || largest_mod_inc == 0;
	return true;
}

/*
  TODO: Make a setting to always export to disk every time a modification is
  made to a file (hopefully not too often). This is done so multiple different
  executions of BasicTV (prob. on multiple machines like an RPi) would be able
  to ensure with a fair amount of certainty that they have read the newest file.

  I'm not worried about the aforementioned TODO for now, but I would REALLY like
  to implement it in the future. This would need to keep track of the number of 
  times it was modified (always reference setters, and figure the setters to
  increment the number), so i'm going to wait for that to be implemented before
  I start worrying about spreading the load.
 */

// TODO: should probably keep this more updated

void id_api::destroy(id_t_ id){	
	if(id_api_should_write_to_disk_mod_inc(id) == true &&
	   settings::get_setting("export_data") == "true"){
		id_disk_api::save(id);
	}
	// TV subsystem
	data_id_t *ptr =
		PTR_ID(id, );
	if(ptr == nullptr){
		return;
	}
	DELETE_TYPE_2(tv_frame_video_t);
	DELETE_TYPE_2(tv_frame_audio_t);
	DELETE_TYPE_2(tv_frame_caption_t);
	DELETE_TYPE_2(tv_window_t);
	DELETE_TYPE_2(tv_channel_t);
	DELETE_TYPE_2(tv_menu_entry_t);
	DELETE_TYPE_2(tv_menu_t);
	DELETE_TYPE_2(tv_item_t);
	
	// net (proto and standard)
	DELETE_TYPE_2(net_socket_t);
	DELETE_TYPE_2(net_proto_peer_t);
	DELETE_TYPE_2(net_proto_socket_t);
	DELETE_TYPE_2(net_proto_type_request_t);
	DELETE_TYPE_2(net_proto_id_request_t);
	DELETE_TYPE_2(net_proto_linked_list_request_t);
	DELETE_TYPE_2(net_proto_con_req_t);
	DELETE_TYPE_2(net_cache_t); // ?

	// IR
	DELETE_TYPE_2(ir_remote_t);

	// input
	DELETE_TYPE_2(input_dev_standard_t);

	// cryptography
	DELETE_TYPE_2(encrypt_priv_key_t);
	DELETE_TYPE_2(encrypt_pub_key_t);

	// stats
	DELETE_TYPE_2(math_stat_sample_set_t);

	DELETE_TYPE_2(id_disk_index_t);
	
	// Count de Monet
       	DELETE_TYPE_2(wallet_set_t);

	print("No proper type was found for clean deleting, cutting losses "
	      "and delisting it, memory leak occuring: " + ptr->get_type(), P_WARN);

	/*
	  Shouldn't get this far, but if it does, delist it manually
	 */
	id_api::array::del(id);
	id_api::cache::del(id, ptr->get_type());
	
	for(uint64_t i = 0;i < id_list.size();i++){
		if(ptr == id_list[i]){
			id_list.erase(id_list.begin()+i);
		}
	}
}

void id_api::destroy_all_data(){
	std::vector<data_id_t*> list_tmp =
		id_list;
	for(uint64_t i = 0;i < list_tmp.size();i++){
		if(list_tmp[i]->get_type_byte() == TYPE_ENCRYPT_PRIV_KEY_T ||
		   list_tmp[i]->get_type_byte() == TYPE_ENCRYPT_PUB_KEY_T ||
		   list_tmp[i]->get_type_byte() == TYPE_ID_DISK_INDEX_T){
			continue;
		}
		P_V(list_tmp[i]->get_type_byte(), P_DEBUG);
		destroy(list_tmp[i]->get_id());
		list_tmp[i] = nullptr;
	}
	/*
	  TODO: This works for now, but I need to create a system that allows
	  exporting of disk information (shouldn't be hard)
	 */
	for(uint64_t i = 0;i < list_tmp.size();i++){
		if(list_tmp[i] == nullptr){
			continue;
		}
		if(list_tmp[i]->get_type_byte() == TYPE_ID_DISK_INDEX_T ||
		   list_tmp[i]->get_id() == production_priv_key_id){
			continue;
		}
		destroy(list_tmp[i]->get_id());
		list_tmp[i] = nullptr;
	}
	destroy(production_priv_key_id);
}

/*
  Goes through all of the IDs, and exports a select few to the disk.
  IDs with the lowest last_access_timestamp_micro_s are exported. Keep
  looping this over until enough memory has been freed. 
 */

static uint64_t get_mem_usage_kb(){
#ifdef __linux
	std::ifstream mem_buf("/proc/self/statm");
	uint32_t t_size = 0, resident = 0, share = 0;
	mem_buf >> t_size >> resident >> share;
	mem_buf.close();
	P_V(resident*sysconf(_SC_PAGE_SIZE)/1024, P_SPAM);
	return resident*sysconf(_SC_PAGE_SIZE)/1024; // RSS
#else
#error No memory function currently exists for non-Linux systems
#endif
}

void id_api::free_mem(){
	uint64_t max_mem_usage = 0;
	try{
		max_mem_usage =
			std::stoi(
				settings::get_setting(
					"max_mem_usage"));
	}catch(...){
		print("using max mem usage default of 1GB", P_NOTE);
		max_mem_usage = 1024*1024; // 1 GB, but should be lower 
	}
	while(get_mem_usage_kb() > max_mem_usage){
		id_t_ lowest_timestamp_id =
			ID_BLANK_ID;
		for(uint64_t i = 0;i < id_list.size();i++){
			data_id_t *lowest =
				PTR_ID(lowest_timestamp_id, );
			data_id_t *current =
				id_list[i];
			if(lowest == nullptr){
				// probably first entry
				lowest_timestamp_id = id_list[i]->get_id();
				lowest = current;
			}else{
				if(lowest->get_last_access_timestamp_micro_s() >
				   current->get_last_access_timestamp_micro_s()){
					lowest_timestamp_id =
						current->get_id();
				}
			}
		}
		id_api::destroy(
			lowest_timestamp_id);
		/*
		  id_api::destroy takes care of exporting to disk (if that makes
		  sense and is allowed per settings.cfg)
		 */
	}
}

/*
  TODO: refine this to filter by ownership and other easy to check items
 */

void id_api::import::load_all_of_type(std::string type, uint8_t flags){
	if(flags & ID_API_IMPORT_FROM_DISK){
		std::vector<std::string> find_out;
		std::vector<id_t_> all_disks = 
			id_api::cache::get(
				TYPE_ID_DISK_INDEX_T);
		for(uint64_t i = 0;i < all_disks.size();i++){
			id_disk_index_t *disk_index_ptr =
				PTR_DATA(all_disks[i],
					 id_disk_index_t);
			if(disk_index_ptr == nullptr){
				continue;
			}
			std::string path = disk_index_ptr->get_path();
			std::vector<std::string> new_ =
				system_handler::find_all_files(
					file::ensure_slash_at_end(
						path),
					type);
			find_out.insert(
				find_out.end(),
				new_.begin(),
				new_.end());
		}
		for(uint64_t i = 0;i < find_out.size();i++){
			P_V_S(find_out[i], P_SPAM);
			std::ifstream in(find_out[i], std::ios::binary);
			if(in.is_open() == false){
				print("can't open file I just searched for", P_ERR);
				continue;
			}
			std::vector<uint8_t> data;
			// TODO: make this efficient with vector's reserve
			char tmp;
			while(in.get(tmp)){
				data.push_back(tmp);
			}
			in.close();
			id_api::array::add_data(data);
		}
	}
	if(flags & ID_API_IMPORT_FROM_NET){
		net_proto_type_request_t *request_ptr =
			new net_proto_type_request_t;
		request_ptr->update_type(
			convert::type::to(
				type));
		// flags are not used anymore, remember?
		/*
		  TODO: include relevant information on a blacklist basis to 
		  decrease spam

		  With responses, I should be able to get a concrete response
		  and generate this list as a return value (with some
		  networking latency) assuming we are on multiple threads.

		  The major use case for this is not to get a list directly, but
		  instead to just query all resources to get it from the net at
		  least on the disk, and from on the disk to memory.
		 */
	}
}

std::vector<uint64_t> id_api::bulk_fetch::mod(std::vector<id_t_> vector){
	std::vector<uint64_t> retval;
	for(uint64_t i = 0;i < vector.size();i++){
		data_id_t *id_ptr =
			PTR_ID_FAST(vector[i], );
		if(id_ptr != nullptr){
			retval.push_back(
				id_ptr->get_mod_inc());
		}else{
			retval.push_back(0);
		}
	}
	return retval;
}

/*
  TODO: something here is broken, check over these functions and make sure there
  aren't any widespread errors
 */

std::vector<uint8_t> id_api::raw::encrypt(std::vector<uint8_t> data){
	id_t_ id = fetch_id(data);
	P_V_S(convert::array::id::to_hex(id), P_SPAM);
	if(get_id_type(id) == TYPE_ENCRYPT_PUB_KEY_T){
		print("can't encrypt public key", P_WARN);
	}else if(get_id_type(id) == TYPE_ENCRYPT_PRIV_KEY_T){
		print("can't encrypt private key", P_WARN);
	}else{
		try{
			id_t_ priv_key_id =
				encrypt_api::search::priv_key_from_hash(
					get_id_hash(id));
			P_V_S(convert::array::id::to_hex(priv_key_id), P_SPAM);
			std::vector<uint8_t> encrypt_chunk =
				encrypt_api::encrypt(
					std::vector<uint8_t>(
						data.begin()+1+sizeof(id_t_),
						data.end()),
					priv_key_id);
			data.erase(
				data.begin()+1+sizeof(id_t_),
				data.end());
			data.insert(
				data.end(),
				encrypt_chunk.begin(),
				encrypt_chunk.end());
			data[0] |= ID_EXTRA_ENCRYPT;
		}catch(...){
			print("can't encrypt exported id information, either not owner or a bug", P_ERR);
		}
	}
	return data;
}

std::vector<uint8_t> id_api::raw::decrypt(std::vector<uint8_t> data){
	id_t_ id = fetch_id(data);
	if(!(data[0] & ID_EXTRA_ENCRYPT)){
		print("can't decrypt pre-decrypted data", P_WARN);
	}else{
		data[0] &= ~ID_EXTRA_ENCRYPT;
		std::vector<uint8_t> decrypt_chunk =
			encrypt_api::decrypt(
				std::vector<uint8_t>(
					data.begin()+1+sizeof(id_t_),
					data.end()),
				encrypt_api::search::pub_key_from_hash(
					get_id_hash(id)));
		data.erase(
			data.begin()+1+sizeof(id_t_),
			data.end());
		data.insert(
			data.end(),
			decrypt_chunk.begin(),
			decrypt_chunk.end());
	}
	return data;
}

std::vector<uint8_t> id_api::raw::compress(std::vector<uint8_t> data){
	if(data[0] & ID_EXTRA_COMPRESS){
		print("can't compress pre-compressed data", P_WARN);
		print("TODO: compress pre-compressed data", P_NOTE);
	}else{
		data[0] |= ID_EXTRA_COMPRESS;
		std::vector<uint8_t> compress_chunk =
			compressor::compress(
				std::vector<uint8_t>(
					data.begin()+1+sizeof(id_t_),
					data.end()),
				9,
				0); // type isn't used currently
		data.erase(
			data.begin()+1+sizeof(id_t_),
			data.end());
		data.insert(
			data.end(),
			compress_chunk.begin(),
			compress_chunk.end());
	}
	return data;
}

std::vector<uint8_t> id_api::raw::decompress(std::vector<uint8_t> data){
	if(!(data[0] & ID_EXTRA_COMPRESS)){
		print("can't decompress uncompressed data", P_WARN);
	}else{
		data[0] &= ~ID_EXTRA_COMPRESS;
		std::vector<uint8_t> compress_chunk =
			compressor::decompress(
				std::vector<uint8_t>(
					data.begin()+1+sizeof(id_t_),
					data.end()));
		data.erase(
			data.begin()+1+sizeof(id_t_),
			data.end());
		data.insert(
			data.end(),
			compress_chunk.begin(),
			compress_chunk.end());
	}
	return data;
}

/*
  Following variables are guaranteed to be not encrypted
 */

static void fetch_size_sanity_check(uint64_t needed_size, uint64_t vector_size){
	if(needed_size > vector_size){
		print("vector is not large enough to contain requested information", P_ERR);
		P_V(needed_size, P_WARN);
		P_V(vector_size, P_WARN);
	}
}

static void generic_fetch(uint8_t *ptr, uint64_t start, uint64_t size, uint8_t *byte_vector){
	if(unlikely(byte_vector == nullptr)){
		// should have been seen in size sanity check
		print("byte_vector is a nullptr", P_ERR);
	}
	memcpy(ptr, byte_vector+start, size);
	convert::nbo::from(ptr, size); // checks for one byte length
}

id_t_ id_api::raw::fetch_id(std::vector<uint8_t> data){
	id_t_ retval = ID_BLANK_ID;
	const uint64_t start = sizeof(extra_t_);
	const uint64_t size = sizeof(retval);
	fetch_size_sanity_check(start+size, data.size());
	generic_fetch(&(retval[0]), start, size, data.data());
	return retval;
}

extra_t_ id_api::raw::fetch_extra(std::vector<uint8_t> data){
	extra_t_ retval = 0;
	const uint64_t start = 0;
	const uint64_t size = sizeof(retval);
	fetch_size_sanity_check(start+size, data.size());
	generic_fetch(&retval, start, size, data.data());
	return retval;
}

type_t_ id_api::raw::fetch_type(std::vector<uint8_t> data){
	return get_id_type(fetch_id(data));
}

mod_inc_t_ id_api::raw::fetch_mod_inc(std::vector<uint8_t> data){
	mod_inc_t_ retval = 0;
	const uint64_t start = sizeof(extra_t_)+sizeof(id_t_);
	const uint64_t size = sizeof(retval);
	fetch_size_sanity_check(start+size, data.size());
	generic_fetch((uint8_t*)&retval, start, size, data.data());
	return retval;
}
