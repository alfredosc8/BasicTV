#include "id.h"
#include "id_api.h"
#include "../main.h"
#include "../util.h"
#include "../lock.h"
#include "../convert.h"

// just for all of the data types
#include "../net/net.h"
#include "../net/net_socket.h"
#include "../net/proto/net_proto.h"
#include "../tv/tv.h"
#include "../tv/tv_frame_standard.h"
#include "../tv/tv_frame_video.h"
#include "../tv/tv_frame_audio.h"
#include "../tv/tv_channel.h"
#include "../tv/tv_window.h"
#include "../encrypt/encrypt.h"
#include "../compress/compress.h"

/*
  Because of encryption and compression overheads, making this multithreadable
  isn't a bad idea. However, implementing locks shouldn't be hard later on.
 */

/*
  TODO: GIVE THIS A REALLY GOOD CLEANING

  I mean seriously
 */

static std::array<std::string, 2> encrypt_blacklist = {
	"encrypt_pub_key_t",
	"encrypt_priv_key_t"
};

static bool encrypt_blacklist_type(std::string type_){
	const bool retval =
		std::find(
			encrypt_blacklist.begin(),
			encrypt_blacklist.end(),
			type_) != encrypt_blacklist.end();
	if(retval){
		print("blacklisted " + type_, P_NOTE);
	}else{
		print("whitelisted " + type_, P_NOTE);
	}
	return retval;
}

std::array<uint8_t, 32> get_id_hash(id_t_ id){
	std::array<uint8_t, 32> retval;
	memcpy(&(retval[0]), &(id[8]), 32);
	return retval;
}

void set_id_hash(id_t_ *id, std::array<uint8_t, 32> hash){
	memcpy(&((*id)[8]), &(hash[0]), 32);
}

uint64_t get_id_uuid(id_t_ id){
	return *((uint64_t*)&(id[0]));
}

void set_id_uuid(id_t_ *id, uint64_t uuid){
	memcpy(&((*id)[0]), &uuid, 8);
}

type_t_ get_id_type(id_t_ id){
	return id[40];
}

void set_id_type(id_t_ *id, type_t_ type){
	(*id)[40] = type;
}

void data_id_t::init_list_all_data(){
	add_data(&id,
		 {sizeof(id_t_)},
		 ID_DATA_ID);
	add_data_id_vector(
		&(linked_list.first),
		{ID_MAX_LINKED_LIST_SIZE});
	add_data_id_vector(
		&(linked_list.second),
		{ID_MAX_LINKED_LIST_SIZE});
}

/*
  production_priv_key_id is the private key used in the encryption of all of the
  files. This should be changable, but no interface exists to do that yet (and I
  don't really see a need for one, assuming no broadcasted net_peer_ts share
  that key).

  If the key can't be found, then zero out the ID. Every time the ID is
  referenced, check to see if the hash is zero and generate the hash the
  first time production_priv_key_id is valid (throw an exception when
  get_id is called without a valid hash)
 */

void data_id_t::init_gen_id(type_t_ type_){
	set_id_uuid(&id, true_rand(1, ~(uint64_t)0));
	set_id_type(&id, type_);
	encrypt_priv_key_t *priv_key =
		PTR_DATA(production_priv_key_id,
			 encrypt_priv_key_t);
	if(priv_key == nullptr){
		print("production_priv_key_id is a nullptr",
		      (running) ? P_WARN : P_NOTE);
		return;
	}
	encrypt_pub_key_t *pub_key =
		PTR_DATA(priv_key->get_pub_key_id(),
			 encrypt_pub_key_t);
	if(pub_key == nullptr){
		print("production_priv_key_id's public key is a nullptr",
		      (running) ? P_WARN : P_NOTE);
		return;
	}
	set_id_hash(&id,
		    encrypt_api::hash::sha256::gen_raw(
			    pub_key->get_encrypt_key().second));
}


data_id_t::data_id_t(void *ptr_, type_t_ type_){
	ptr = ptr_;
	init_gen_id(type_);
	init_list_all_data();
	id_api::array::add(this);
	id_api::cache::add(id, type_);
}

data_id_t::~data_id_t(){
	id_api::array::del(id);
	try{
		id_api::cache::del(id, get_id_type(id));
	}catch(...){}
}

id_t_ data_id_t::get_id(bool skip){
	// even with unlikely, this seems pretty slow
	if(!skip && !id_throw_exception && get_id_hash(id) == ID_BLANK_HASH){
		encrypt_priv_key_t *priv_key =
			PTR_DATA(production_priv_key_id,
				 encrypt_priv_key_t);
		if(priv_key == nullptr){
			print("do not have a hash yet, aborting", P_ERR);
		}
		encrypt_pub_key_t *pub_key =
			PTR_DATA(priv_key->get_pub_key_id(),
				 encrypt_pub_key_t);
		if(pub_key == nullptr){
			print("do not have a hash yet, aborting", P_ERR);
		}
		id_t_ old_id = id;
		set_id_hash(&id,
			    encrypt_api::hash::sha256::gen_raw(
				    pub_key->get_encrypt_key().second));
		// ID list is raw pointers, type list is an ID vector (fast)
		id_api::cache::del(old_id, get_id_type(id));
		id_api::cache::add(id, get_id_type(id));
	}
	return id;
}

void data_id_t::set_id(id_t_ id_){
	try{
		id_api::cache::del(id, get_id_type(id));
	}catch(...){} // shouldn't run anyways
	set_id_uuid(&id, get_id_uuid(id_));
	set_id_hash(&id, get_id_hash(id_));
	// type HAS to be preserved
	id_api::cache::add(id, get_id_type(id));
}

std::string data_id_t::get_type(){
	return convert::type::from(get_id_type(id));
}

/*
  Even though function can pass information to each other through IDs,
  get_id is used in too many searches to be a healthy benchmark
 */

void *data_id_t::get_ptr(){
	last_access_timestamp_micro_s =
		get_time_microseconds();
	return ptr;
}

void data_id_t::add_data(void *ptr_, std::vector<uint32_t> size_, uint64_t flags){
	if(ptr_ == nullptr){
		print("ptr_ is a nullptr", P_ERR);
	}
	data_vector.push_back(
		data_id_ptr_t(
			ptr_,
			size_,
			flags));
}

id_t_ data_id_t::get_encrypt_pub_key_id(){
	return encrypt_pub_key_id;
}

uint64_t data_id_t::get_data_index_size(){
	return data_vector.size();
}

/*
  Only currently used for exporting rules, so there is no need for any fine
  grain getters
 */

std::vector<uint8_t> data_id_t::get_ptr_flags(){
	std::vector<uint8_t> retval;
	for(uint64_t i = 0;i < data_vector.size();i++){
		retval.push_back(
			data_vector[i].get_flags());
	}
	return retval;
}

/*
  Anything being directly passed to ID_EXPORT_RAW is assumed to be inserted in
  network byte order. I don't see any exception to this, and whatever niche
  function it serves can be created with another function macro

  ^
  |
  |
  Why would I make that assumption? I forget...

 */

static void id_export_raw(std::vector<uint8_t> tmp, std::vector<uint8_t> *vector){
	if(tmp.size() == 0){
		print("attempted to export completely blank data set", P_WARN);
		return;
	}
	tmp = convert::nbo::to(tmp);
	vector->insert(vector->end(), tmp.begin(), tmp.end());
}

//#define ID_EXPORT(var, list) id_export_raw((uint8_t*)&var, sizeof(var), &list)
#define ID_EXPORT(var, list) id_export_raw(std::vector<uint8_t>((uint8_t*)&var, (uint8_t*)&var+sizeof(var)), &list)

/*
  flags will be what types to EXCLUDE
 */

/*
  Don't export on the following conditions:
  1. flags bit is flipped when param bit is not or
  2. flags bit is one
 */

static bool export_datum_check_type(uint8_t flags, uint8_t param){
	const bool data_nonet = flags & ID_DATA_NONET;
	const bool data_noexp = flags & ID_DATA_NOEXP;
	const bool param_nonet = param & ID_DATA_NONET;
	const bool param_noexp = param & ID_DATA_NOEXP;

	const bool export_nonet =
		(data_nonet == param_nonet) || (data_nonet == 0);
	const bool export_noexp =
		(data_noexp == param_noexp) || (data_noexp == 0);
	return export_nonet && export_noexp;
}

std::vector<uint8_t> data_id_t::export_data(uint8_t flags_, uint8_t extra){
	std::vector<uint8_t> retval;
	/*
	  0 & 0: include, no preference in second
	  1 & 0: include, preference doesn't apply here
	  0 & 1: exclude, preference in second
	  1 & 1: include, preference is met
	 */
	if(encrypt_blacklist_type(
		   convert::type::from(get_id_type(id)))){
		print("forcing no encryption on basis of encryption blacklist", P_DEBUG);
		extra &= ~ID_EXTRA_ENCRYPT;
	}
	if(!export_datum_check_type(global_flags, flags_)){
		print("global flags don't allow exporting, skipping", P_DEBUG);
		return {};
	}
	if(is_owner() == false){
		return imported_data;
	}
	uint8_t current_extra = 0;
	ID_EXPORT(current_extra, retval); // current extra is nothing
	ID_EXPORT(id, retval);
	ID_EXPORT(modification_incrementor, retval);
	for(uint64_t i = 0;i < data_vector.size();i++){
		/*
		  TODO: ENFORCE EXPORTING RULES
		*/
		if(!export_datum_check_type(
			   data_vector[i].get_flags(),
			   flags_)){
			print("skipping, individual datum incompatiable with flags", P_SPAM);
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
			for(uint64_t i = 0;i < vector->size();i++){
				transport_size_t trans_size =
					(*vector)[i].size();
				data_to_export.insert(
					data_to_export.end(),
					&trans_size,
					&trans_size+1);
				if((*vector)[i].data() == nullptr){
					//print("vector is empty, skipping", P_SPAM);
					continue;
					// don't export anything
				}
				data_to_export.insert(
					data_to_export.end(),
					(uint8_t*)(*vector)[i].data(),
					(uint8_t*)(*vector)[i].data()+trans_size);
			}

		}else{
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
		//P_V(trans_size, P_SPAM);
		ID_EXPORT(trans_i, retval);
		ID_EXPORT(trans_size, retval);
		id_export_raw(data_to_export, &retval);
	}
	if(extra & ID_EXTRA_COMPRESS){
		retval = id_api::raw::compress(retval);
	}
	if(extra & ID_EXTRA_ENCRYPT){
		retval = id_api::raw::encrypt(retval);
	}
	return retval;
}

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
	try{
		data = id_api::raw::decrypt(data);
		data = id_api::raw::decompress(data);
	}catch(...){
		// this code isn't koscher at all, but it should work fine
	}
	id_t_ trans_id = ID_BLANK_ID;
	uint8_t extra = 0;
	ID_IMPORT(extra);
	ID_IMPORT(trans_id);
	ID_IMPORT(modification_incrementor);
	// P_V_S(convert::array::id::to_hex(trans_id), P_SPAM);
	// P_V_S(convert::type::from(get_id_type(trans_id)), P_SPAM);
	// P_V_B(extra, P_SPAM);
	if(get_id_type(trans_id) != get_id_type(id)){
		P_V(get_id_type(trans_id), P_SPAM);
		P_V(get_id_type(id), P_SPAM);
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
			// P_V(trans_i, P_SPAM);
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
	imported_data = data;
}

void data_id_t::rsa_decrypt_backlog(){
	// The whole strucute isn't guaranteed to come over, and TCP
	// gives us lossless connections, so apply them from oldest to
	// newest
	for(uint64_t i = 0;i < rsa_backlog.size();i++){
		import_data(rsa_backlog[i]);
	}
	rsa_backlog.clear();
}

id_t_ data_id_t::get_prev_linked_list(){
	if(linked_list.first.size() > 0){
		return linked_list.first[0];
	}
	return ID_BLANK_ID;
}

id_t_ data_id_t::get_next_linked_list(){
	if(linked_list.second.size() > 0){
		return linked_list.second[0];
	}
	return ID_BLANK_ID;
}

void data_id_t::set_prev_linked_list(id_t_ data){
	linked_list.first.clear();
	linked_list.first.push_back(data);
}

void data_id_t::set_next_linked_list(id_t_ data){
	linked_list.second.clear();
	linked_list.second.push_back(data);
}

bool data_id_t::is_owner(){
	return (get_id_hash(id) ==
		get_id_hash(production_priv_key_id));
}

void data_id_t::noexport_all_data(){
	global_flags |= ID_DATA_NOEXP;
}

void data_id_t::nonet_all_data(){
	global_flags |= ID_DATA_NONET;
}

data_id_ptr_t::data_id_ptr_t(void *ptr_,
			     std::vector<uint32_t> length_,
			     uint8_t flags_){
	ptr = ptr_;
	length = length_;
	flags = flags_;
}

data_id_ptr_t::~data_id_ptr_t(){
}

void *data_id_ptr_t::get_ptr(){
	return ptr;
}

uint32_t data_id_ptr_t::get_length(){
	if(length.size() == 1){
		return length[0];
	}else{
		print("invalid size of length", P_ERR);
		return 0; // redunant
	}
}

std::vector<uint32_t> data_id_ptr_t::get_length_vector(){
	return length;
}

uint8_t data_id_ptr_t::get_flags(){
	return flags;
}

/*
  Data coming in has to be relatively sane
 */

std::vector<uint8_t> id_transport::get_entry(std::vector<uint8_t> data, transport_i_t trans_i){
	/*
	  The following entries should be readable, even if it is encrypted:
	  0: ID
	  1: type (32-byte string)
	  2: public key for decryption

	  Anything beyond this is originally decrypted, and once decrypted, can
	  be fetched with this function
	 */
	uint64_t data_pos = 0;
	while(data_pos < data.size()){
		const uint32_t preamble_size =
			sizeof(transport_i_t)+sizeof(transport_size_t);
		if(data.size()-data_pos < preamble_size){
			break;
		}
		transport_i_t tmp_trans_i = 0;
		memcpy(&tmp_trans_i,
		       data.data()+data_pos,
		       sizeof(tmp_trans_i)); 
		transport_size_t tmp_trans_size = 0;
		memcpy(&tmp_trans_size,
		       data.data()+data_pos+sizeof(transport_i_t),
		       sizeof(transport_size_t));
		if(tmp_trans_i == trans_i){
			return std::vector<uint8_t>(
				data.begin()+data_pos+preamble_size,
				data.begin()+data_pos+preamble_size+tmp_trans_size);
		}else{
			data_pos += tmp_trans_size+preamble_size;
		}
	}
	return {};
}

std::vector<uint8_t> id_transport::set_entry(std::vector<uint8_t> entry){
	print("implement individual entry setting somehow", P_CRIT);
}
