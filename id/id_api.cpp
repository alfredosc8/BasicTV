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
#include "../net/net_cache.h"
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

/*
  TODO (SERIOUS):
  Even though there is no cryptographic proof of ownership, make sure that
  the hash of any requests come from a socket whose bound peer has the same
  hash, since there is no need (right now, for these simple types
  specifically) to forward them. There might be a case in the future where
  another type is allowed to be forwarded to find really obscure information,
  but that's pretty far off.

  (Also forwardable types are probably prone to DoS, unless we do something
  crafty with rules and rule enforcement with public key hashes)
 */

std::vector<type_t_> encrypt_blacklist = {
	TYPE_ENCRYPT_PUB_KEY_T,
	TYPE_ENCRYPT_PRIV_KEY_T,
	TYPE_NET_PROTO_CON_REQ_T,
	TYPE_NET_PROTO_TYPE_REQUEST_T,
	TYPE_NET_PROTO_LINKED_LIST_REQUEST_T,
	TYPE_NET_PROTO_ID_REQUEST_T
};

uint64_t id_api::array::get_id_count(){
	return id_list.size();
}

static data_id_t *id_find(id_t_ id){
	for(uint64_t i = 0;i < id_list.size();i++){
		if(unlikely(id_list[i]->get_id(true) == id)){
			// print("found ID " + convert::array::id::to_hex(id), P_SPAM);
			return id_list[i];
		}
	}
	// print("couldn't find ID " + convert::array::id::to_hex(id), P_SPAM);
	return nullptr;
}

/*
  TODO: make seperate functions for fast lookups only (ideally for use with the
  stats library, since speed is more important than actually having the data)
 */

#define LOOKUP_AND_RETURN()			\
	retval = id_find(id);			\
	id_lookup.erase(id_lookup.end()-1);	\
	if(retval != nullptr){			\
		return retval;			\
	}					\

// might be more optimized data types
std::vector<id_t_> id_lookup;

/*
  Should probably change around to optimize somehow
 */
 
data_id_t *id_api::array::ptr_id(id_t_ id,
				 std::string type,
				 uint8_t flags){
	if(id == ID_BLANK_ID){
		return nullptr;
	}
	if(convert::type::from(get_id_type(id)) != type && type != ""){
		print("type mis-match in ptr_id", P_WARN);
		return nullptr;
	}
	for(uint64_t i = 0;i < id_lookup.size();i++){
		if(id_lookup[i] == id){
			print("preventing dead-lock with ID request, pretending we don't have it" + id_breakdown(id), P_WARN);
			return nullptr;
		}
	}
	id_lookup.push_back(id);
	data_id_t *retval = nullptr;
	LOOKUP_AND_RETURN();
	id_lookup.push_back(id);
	id_api::cache::load_id(id);
	LOOKUP_AND_RETURN();
	id_lookup.push_back(id);
	id_disk_api::load(id);
	LOOKUP_AND_RETURN();
	if(!(flags & ID_LOOKUP_FAST)){
		net_proto::request::add_id(id);
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
	print("cannot find ID in list", P_WARN);
}
	
#define CHECK_TYPE(a)					\
	if(convert::type::from(type) == #a){		\
		print("importing data", P_SPAM);	\
		a* tmp = nullptr;			\
		try{					\
			tmp = new a;			\
			tmp->id.import_data(data_);	\
			return tmp->id.get_id();	\
		}catch(...){				\
			if(tmp != nullptr){		\
				delete tmp;		\
				tmp = nullptr;		\
				return ID_BLANK_ID;	\
			}				\
		}					\
	}						\

/*
  General purpose reader, returns the ID of the new information.
  The only current use of the return value is for associating sockets with
  data requests.
 */

/*
  We need a second function, otherwise deadlocks happens
 */

bool id_api::array::exists_in_array(id_t_ id){
	for(uint64_t i = 0;i < id_list.size();i++){
		if(unlikely(id_list[i]->get_id() == id)){
			return true;
		}
	}
	return false;
}

id_t_ id_api::array::add_data(std::vector<uint8_t> data_, bool raw){
	id_t_ id = ID_BLANK_ID;
	type_t_ type = 0;
	try{
		id = id_api::raw::fetch_id(data_);
		type = get_id_type(id_api::raw::fetch_id(data_));
		P_V_S(convert::array::id::to_hex(id), P_VAR);
		P_V_S(convert::type::from(type), P_VAR);
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
			if(exists_in_array(id)){
				PTR_ID(id, )->import_data(data_);
				return tmp_type_cache[i];
			}
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
	CHECK_TYPE(wallet_set_t);
	CHECK_TYPE(net_socket_t);
	CHECK_TYPE(math_number_set_t);
	CHECK_TYPE(net_interface_ip_address_t);
	print("type " + convert::type::from(type) + " needs a loader", P_CRIT);
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
		PTR_ID(vector[0], )->set_linked_list(
			std::make_pair(
				std::vector<id_t_>(),
				std::vector<id_t_>({vector[1]})));
		PTR_ID(vector[1], )->set_linked_list(
			std::make_pair(
				std::vector<id_t_>({vector[0]}),
				std::vector<id_t_>()));
		return;
	default:
		break;
	}
	data_id_t *first = PTR_ID(vector[0], );
	if(first != nullptr){
		first->set_linked_list(
			std::make_pair(
				std::vector<id_t_>({}),
				std::vector<id_t_>({vector[1]})));
	}else{
		print("first entry is a nullptr, this can be fixed, but I didn't bother and this shouldn't happen anyways", P_ERR);
	}
	for(uint64_t i = 1;i < vector.size()-1;i++){
		data_id_t *id = PTR_ID(vector[i], );
		if(id == nullptr){
			print("can't link against an ID that doesn't exist", P_ERR);
		}
		id->set_linked_list(
			std::make_pair(
				std::vector<id_t_>({vector[i-1]}),
				std::vector<id_t_>({vector[i+1]})));
	}
	PTR_ID(vector[vector.size()-1], )->set_linked_list(
		std::make_pair(
			std::vector<id_t_>({vector[vector.size()-2]}),
			std::vector<id_t_>({})));
}

/*
  These functions only operate in one direction per call, but can be
  made to run in both (call backwards in vector A, call forwards
  in vector B, insert A, then start_id, then B).
 */

#pragma message("id_api::linked_list::list::by_distance can be optimized pretty easily, just haven't done that yet")

std::vector<id_t_> id_api::linked_list::list::by_distance(id_t_ start_id,
							  int64_t pos){
	ASSERT(pos != 0, P_ERR);
	ASSERT(start_id != ID_BLANK_ID, P_ERR);
	std::vector<id_t_> retval;
	bool forwards = pos > 0;
	int64_t cur_pos = 0;
	data_id_t *id_ptr =
		PTR_ID(start_id, );
	try{
		while(id_ptr != nullptr &&
		      abs(pos) >= abs(cur_pos)){
			const id_t_ next_id =
				(forwards) ?
				id_ptr->get_linked_list().second.at(0) :
				id_ptr->get_linked_list().first.at(0);
			id_ptr =
				PTR_ID(next_id, );
			retval.push_back(
				next_id);
			if(forwards){
				cur_pos++;
			}else{
				cur_pos--;
			}
		}
	}catch(...){}
	return retval;
}

std::vector<id_t_> id_api::linked_list::list::by_distance_until_match(id_t_ start_id,
								      int64_t pos,
								      id_t_ target_id){
	ASSERT(pos != 0, P_ERR);
	ASSERT(start_id != ID_BLANK_ID, P_ERR);
	ASSERT(target_id != ID_BLANK_ID, P_ERR);
	std::vector<id_t_> retval;
	bool forwards = pos > 0;
	int64_t cur_pos = 0;
	data_id_t *id_ptr =
		PTR_ID(start_id, );
	try{
		while(id_ptr != nullptr &&
		      id_ptr->get_id() != target_id &&
		      abs(pos) >= abs(cur_pos)){
			const id_t_ next_id =
				(forwards) ?
				id_ptr->get_linked_list().second.at(0) :
				id_ptr->get_linked_list().first.at(0);
			id_ptr =
				PTR_ID(next_id, );
			retval.push_back(
				next_id);
			if(forwards){
				cur_pos++;
			}else{
				cur_pos--;
			}
		}
	}catch(...){}
	if(!forwards){
		std::reverse(
			retval.begin(),
			retval.end());
	}
	return retval;
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

#define DELETE_TYPE_2(a) if(ptr->get_type() == #a){print("deleting " + (std::string)(#a), P_DEBUG);delete (a*)ptr->get_ptr();ptr = nullptr;return;}

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
	P_V_S(convert::array::id::to_hex(id_), P_VAR);
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

static bool id_api_should_write_to_disk_export_flags(id_t_ id){
	data_id_t *id_ptr =
		PTR_ID(id, );
	if(id_ptr != nullptr){
		uint8_t export_rules = 0;
		id_ptr->get_highest_global_flag_level(
			nullptr,
			&export_rules,
			nullptr);
		return export_rules > ID_DATA_EXPORT_RULE_NEVER;
	}
	// can't export to disk if we can't access it now anyways...
	return false;
}

// TODO: would make more sense to pass by pointer here

static bool id_api_should_write_to_disk(id_t_ id){
	return id_api_should_write_to_disk_mod_inc(id) &&
		id_api_should_write_to_disk_export_flags(id);
}

void id_api::destroy(id_t_ id){	
	if(id_api_should_write_to_disk_mod_inc(id) &&
	   id_api_should_write_to_disk_export_flags(id) &&
	   settings::get_setting("export_data") == "true"){
		id_disk_api::save(id);
	}
	// TV subsystem
	data_id_t *ptr =
		PTR_ID(id, );
	if(ptr == nullptr){
		print("already destroying a destroyed type " + id_breakdown(id), P_WARN);
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

	DELETE_TYPE_2(id_disk_index_t);
	
	// Count de Monet
       	DELETE_TYPE_2(wallet_set_t);

	// Math
	DELETE_TYPE_2(math_number_set_t);

	DELETE_TYPE_2(net_interface_ip_address_t);
	
	print("No proper type was found for clean deleting, cutting losses "
	      "and delisting it, memory leak occuring: " + ptr->get_type(), P_ERR);

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
	std::vector<id_t_> list_tmp =
		id_api::get_all();
	for(uint64_t i = 0;i < list_tmp.size();i++){
		data_id_t *tmp =
			PTR_ID(list_tmp[i], );
		if(tmp == nullptr){
			// type destructors can delete other types
			continue;
		}
		if(tmp->get_type_byte() == TYPE_ENCRYPT_PRIV_KEY_T ||
		   tmp->get_type_byte() == TYPE_ENCRYPT_PUB_KEY_T ||
		   tmp->get_type_byte() == TYPE_ID_DISK_INDEX_T){
			continue;
		}
		P_V(tmp->get_type_byte(), P_VAR);
		destroy(tmp->get_id());
		tmp = nullptr;
	}
	/*
	  TODO: This works for now, but I need to create a system that allows
	  exporting of disk information (shouldn't be hard)
	 */
	for(uint64_t i = 0;i < list_tmp.size();i++){
		data_id_t *tmp =
			PTR_ID(list_tmp[i], );
		if(tmp == nullptr){
			// type destructors can delete other types
			continue;
		}
		if(tmp->get_type_byte() == TYPE_ID_DISK_INDEX_T ||
		   tmp->get_id() == production_priv_key_id){
			continue;
		}
		destroy(tmp->get_id());
		tmp = nullptr;
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
	P_V(resident*sysconf(_SC_PAGE_SIZE)/1024, P_VAR);
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
		print("freeing memory by deleting " +
		      convert::array::id::to_hex(lowest_timestamp_id), P_SPAM);
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
			P_V_S(find_out[i], P_VAR);
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
	P_V_S(convert::array::id::to_hex(id), P_VAR);
	if(get_id_type(id) == TYPE_ENCRYPT_PUB_KEY_T){
		print("can't encrypt public key", P_WARN);
	}else if(get_id_type(id) == TYPE_ENCRYPT_PRIV_KEY_T){
		print("can't encrypt private key", P_WARN);
	}else{
		try{
			id_t_ priv_key_id =
				encrypt_api::search::priv_key_from_hash(
					get_id_hash(id));
			P_V_S(convert::array::id::to_hex(priv_key_id), P_VAR);
			std::vector<uint8_t> unencrypt_chunk =
				std::vector<uint8_t>(
					data.begin()+ID_PREAMBLE_SIZE,
					data.end());
			P_V(unencrypt_chunk.size(), P_VAR);
			if(unencrypt_chunk.size() == 0){
				// having no actual payload still works
				print("unencrypt_chunk is empty for " + id_breakdown(id) + "(function call for supplied data probably had exporting restrictions of some sort)", P_SPAM);
				return data;
			}
			std::vector<uint8_t> encrypt_chunk =
				encrypt_api::encrypt(
					unencrypt_chunk,
					priv_key_id);
			P_V(encrypt_chunk.size(), P_VAR);
			if(encrypt_chunk.size() != 0){
				data.erase(
					data.begin()+ID_PREAMBLE_SIZE,
					data.end());
				data.insert(
					data.end(),
					encrypt_chunk.begin(),
					encrypt_chunk.end());
				data[0] |= ID_EXTRA_ENCRYPT;
			}else{
				print("can't encrypt empty chunk", P_WARN);
				// should be checked before call, but it's no
				// biggie right now
			}
		}catch(...){
			P_V_S(convert::type::from(get_id_type(id)), P_VAR);
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
		id_t_ pub_key_id =
			encrypt_api::search::pub_key_from_hash(
				get_id_hash(id));
		if(pub_key_id == ID_BLANK_ID){
			print("couldn't find public key for ID decryption" + id_breakdown(id), P_ERR);
		}else{
			std::vector<uint8_t> decrypt_chunk =
				encrypt_api::decrypt(
					std::vector<uint8_t>(
						data.begin()+ID_PREAMBLE_SIZE,
						data.end()),
					pub_key_id);
			data.erase(
				data.begin()+ID_PREAMBLE_SIZE,
				data.end());
			data.insert(
				data.end(),
				decrypt_chunk.begin(),
				decrypt_chunk.end());
		}
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
					data.begin()+ID_PREAMBLE_SIZE,
					data.end()),
				9,
				0); // type isn't used currently
		data.erase(
			data.begin()+ID_PREAMBLE_SIZE,
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
		std::vector<uint8_t> raw_chunk =
			std::vector<uint8_t>(
				data.begin()+ID_PREAMBLE_SIZE,
				data.end());
		std::vector<uint8_t> compress_chunk =
			compressor::decompress(
				raw_chunk);
		data.erase(
			data.begin()+ID_PREAMBLE_SIZE,
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
		P_V(needed_size, P_WARN);
		P_V(vector_size, P_WARN);
		print("vector is not large enough to contain requested information", P_ERR);
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

void id_api::add_data(std::vector<uint8_t> data){
	// just add to cache, it's fine...
	id_api::array::add_data(data);
	id_api::cache::add_data(data);
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
	P_V_B(retval, P_VAR);
	return retval;
}


/*
  Helper function I wrote that return all current data about any ID loaded in
  memory.
 */

std::vector<std::string> id_gdb_lookup(const char* hex_id){
	id_t_ lookup_id =
		convert::array::id::from_hex(
			(std::string)hex_id);
	if(lookup_id == ID_BLANK_ID){
		return {"couldn't parse"};
	}else{
		for(uint64_t i = 0;i < id_list.size();i++){
			if(id_list[i]->get_id() == lookup_id){
				std::stringstream ss;
				ss << id_list[i];
				std::string pointer =
					ss.str();
				const std::vector<std::string> retval =
					{
						hex_id,
						"type: " + id_list[i]->get_type(),
						"mod inc:" + std::to_string(id_list[i]->get_mod_inc()),
						"pointer: " + pointer
					};
				return retval;
			}
		}
	}
	return {"ID not found"};
}

int64_t id_api::linked_list::pos_in_linked_list(id_t_ ref_id, id_t_ goal_id, uint64_t max_search_radius){
	std::pair<id_t_, int64_t> lower_bound =
		std::make_pair(
			ref_id, 0);
	std::pair<id_t_, int64_t> upper_bound =
		std::make_pair(
			ref_id, 0);
	while((uint64_t)abs(lower_bound.second) < max_search_radius && lower_bound.first != ID_BLANK_ID){
		data_id_t *tmp_id_ptr =
			PTR_ID(lower_bound.first, );
		if(tmp_id_ptr == nullptr){
			break;
		}
		std::pair<std::vector<id_t_>, std::vector<id_t_> > linked_list =
			tmp_id_ptr->get_linked_list();
		if(linked_list.first.size() == 0){
			lower_bound.first = ID_BLANK_ID;
		}else{
			lower_bound.first =
				linked_list.first[0];
			lower_bound.second--;
		}
	}
	if(lower_bound.first == goal_id){
		return lower_bound.second;
	}
	while((uint64_t)abs(upper_bound.second) < max_search_radius && upper_bound.first != ID_BLANK_ID){
		data_id_t *tmp_id_ptr =
			PTR_ID(upper_bound.first, );
		if(tmp_id_ptr == nullptr){
			break;
		}
		std::pair<std::vector<id_t_>, std::vector<id_t_> > linked_list =
			tmp_id_ptr->get_linked_list();
		if(linked_list.first.size() == 0){
			upper_bound.first = ID_BLANK_ID;
		}else{
			upper_bound.first =
				linked_list.first[0];
			upper_bound.second++;
		}
	}
	if(upper_bound.first == goal_id){
		return upper_bound.second;
	}
	print("unable to find ID in linked list", P_ERR);
	return 0;
}


std::vector<uint8_t> id_api::export_id(
	id_t_ id_,
	uint8_t flags,
	uint8_t extra, 
	uint8_t network_flags,
	uint8_t export_flags,
	uint8_t peer_flags){
	/*
	  Because of space conerns with memory, IDs in memory don't store
	  exported versions of themselves, meaning that direct exports from
	  the ID only work if we are the owner
	 */
	const uint8_t cache_extra =
		extra & (~ID_EXTRA_COMPRESS); // anything encrypted should be compressed
	std::vector<uint8_t> retval =
		id_api::cache::get_id(
			id_,
			cache_extra);
	if(retval.size() == 0){
		print("ID not found in cache, loading from disk" + id_breakdown(id_), P_NOTE);
		id_disk_api::load(id_);
		retval =
			id_api::cache::get_id(
				id_,
				cache_extra);
	} // assuming faster reads than encryption, should be accurate
	if(retval.size() == 0 && get_id_hash(id_) == get_id_hash(production_priv_key_id)){
		print("id isn't in cache, exporting whole" + id_breakdown(id_), P_WARN);
		data_id_t *id =
			PTR_ID(id_, );
		if(id != nullptr){
			return id->export_data(
				flags,
				extra,
				network_flags,
				export_flags,
				peer_flags);
		}
	}
	if(retval.size() == 0){
		print("unable to load ID", P_NOTE);
	}
	return retval;
}

bool encrypt_blacklist_type(type_t_ type_){
	return std::find(
		encrypt_blacklist.begin(),
		encrypt_blacklist.end(),
		type_) != encrypt_blacklist.end();
}

/*
  Printable string containing the sizes of each type in an array or loaded
  into the program cache
 */

std::string id_api::cache::breakdown(){
	std::string retval;
	for(uint64_t i = 0;i < type_cache.size();i++){
		try{
			retval +=
				convert::type::from(
					type_cache[i].second) +
				" " +
				std::to_string(type_cache[i].first.size()) +
				"\n";
		}catch(...){
			print("error in computing cache breakdown", P_WARN);
		}
	}
	return retval;
}

#define ID_SHIFT(x) vector_pos += sizeof(x)
#define ID_IMPORT(x) memcpy(&x, data.data()+vector_pos, sizeof(x));vector_pos += sizeof(x)

#pragma warning("strip_to_lowest_rules removed a lot of sanity checks for clarity, should REALLY re-add");

std::vector<uint8_t> id_api::raw::strip_to_lowest_rules(
	std::vector<uint8_t> data,
	uint8_t network_rules,
	uint8_t export_rules,
	uint8_t peer_rules){

	uint32_t vector_pos = 0;
	id_t_ trans_id = ID_BLANK_ID;
	uint8_t extra =
		data[0];
	ASSERT((0b11111100 & extra) == 0, P_ERR);
	if(extra & ID_EXTRA_ENCRYPT){
		print("try and optimize code so strip_to_rules doesn't have to handle encryption", P_WARN);
		data = id_api::raw::decrypt(data);
	}
	if(extra & ID_EXTRA_COMPRESS){
		print("try and optimize code so strip_to_rules doesn't have to handle encryption", P_WARN);
		data = id_api::raw::decompress(data);
	}
	mod_inc_t_ modification_incrementor = 0;
	ID_SHIFT(extra); // just to remove it
	ID_SHIFT(trans_id);
	ID_SHIFT(modification_incrementor);

	transport_i_t trans_i = 0;
	transport_size_t trans_size = 0;
	uint8_t network_rules_tmp = 0;
	uint8_t export_rules_tmp = 0;
	uint8_t peer_rules_tmp = 0;
	while(data.size() > sizeof(transport_i_t) + sizeof(transport_size_t)){
		ID_SHIFT(trans_i);
		ID_IMPORT(network_rules_tmp);
		ID_IMPORT(export_rules_tmp);
		ID_IMPORT(peer_rules_tmp);
		ID_IMPORT(trans_size);
		const bool network_allows = (network_rules_tmp >= network_rules || network_rules == ID_DATA_RULE_UNDEF);
		const bool export_allows = (export_rules_tmp >= export_rules || export_rules == ID_DATA_RULE_UNDEF);
		const bool peer_allows = (peer_rules_tmp >= peer_rules || peer_rules == ID_DATA_RULE_UNDEF);
		if(!(network_allows &&
		     export_allows &&
		     peer_allows)){
			data.erase(
				data.begin()+vector_pos,
				data.begin()+vector_pos+trans_size);
			vector_pos -= trans_size;
		}
	}
	return data;
}

void id_api::print_id_vector(std::vector<id_t_> id_vector, uint32_t p_l){
	for(uint64_t i = 0;i < id_vector.size();i++){
		print(id_breakdown(id_vector[i]), p_l);
	}
}
