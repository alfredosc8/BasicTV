#include "../main.h"
#ifndef ID_H
#define ID_H
#include <cstdint>
#include <fstream>
#include <array>
#include <vector>
#include <random>
#include <cstdlib>

/*
  id_t: ID and pointer system for the networking system
 */

#define ADD_DATA(x) id.add_data_raw((uint8_t*)&x, sizeof(x))

typedef std::array<uint8_t, 41> id_t_;
typedef uint8_t type_t_;
typedef std::array<uint8_t, 32> hash_t_;
typedef uint8_t extra_t_;
typedef uint64_t mod_inc_t_;

const id_t_ blank_id = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
const hash_t_ blank_hash = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

#define ID_BLANK_ID blank_id
#define ID_BLANK_TYPE (0)
#define ID_BLANK_HASH (blank_hash)

#define ID_DATA_NOEXP (1 << 0)
#define ID_DATA_NONET (1 << 1)
#define ID_DATA_ID (1 << 2)
#define ID_DATA_BYTE_VECTOR (1 << 3)
#define ID_DATA_EIGHT_BYTE_VECTOR (1 << 4)
#define ID_DATA_ID_VECTOR (1 << 5)
#define ID_DATA_BYTE_VECTOR_VECTOR (1 << 6)

#define ID_DATA_CACHE ID_DATA_NOEXPORT
 
// might want to be larger (?)
#define ID_MAX_LINKED_LIST_SIZE 64

#define ID_EXTRA_ENCRYPT (1 << 0)
#define ID_EXTRA_COMPRESS (1 << 1)

#define ID_PREAMBLE_SIZE (sizeof(extra_t_)+sizeof(id_t_)+sizeof(mod_inc_t_))

// pointer added through add_data
struct data_id_ptr_t{
private:
	void *ptr = nullptr;
	std::vector<uint32_t> length;
	uint8_t flags = 0;
public:
	data_id_ptr_t(void *ptr_,
		      std::vector<uint32_t> length_,
		      uint8_t flags_);
	~data_id_ptr_t();
	void *get_ptr();
	uint32_t get_length();
	std::vector<uint32_t> get_length_vector();
	uint8_t get_flags();
	void set_flags(uint8_t flags_){flags = flags_;}
};

/*
  TODO: rename add_data functions to more easily see how it is stored and used
  internally (raw pointer vs byte vector, namely)
 */

type_t_ get_id_type(id_t_ id); // tacky

struct data_id_t{
private:
	// first 8 bytes UUID, last 32-byte SHA-256 hash
	id_t_ id = ID_BLANK_ID;
	void *ptr = nullptr;
	id_t_ encrypt_pub_key_id = ID_BLANK_ID;
	std::vector<std::vector<uint8_t> > rsa_backlog;
	std::vector<data_id_ptr_t> data_vector;
	std::pair<std::vector<id_t_>, std::vector<id_t_> > linked_list;
	std::vector<uint8_t> imported_data; // encrypted data if imported
	void init_list_all_data();
	void init_gen_id(type_t_);
	void init_type_cache();
	// set at get_ptr(), used for selective exporting
	uint64_t last_access_timestamp_micro_s = 0;
	// incremented every time a getter or setter is called in either this
	// function or the parent data type (manually call mod_inc();
	mod_inc_t_ modification_incrementor = 0;
	uint8_t global_flags = 0;
	void add_data(void *ptr_, std::vector<uint32_t> size_, uint64_t flags_ = 0);
public:
	data_id_t(void *ptr_, uint8_t type);
	~data_id_t();
	// getters and setters
	// skip check for hash, only used internally
	id_t_ get_id(bool skip = false);
	/*
	  SHOULD ONLY BE USED TO BOOTSTRAP
	 */
	void set_id(id_t_ id_);
	std::string get_type();
	uint8_t get_type_byte(){return get_id_type(id);}
	void *get_ptr();
	void mod_inc(){modification_incrementor++;}
	mod_inc_t_ get_mod_inc(){return modification_incrementor;}
	id_t_ get_encrypt_pub_key_id();
	uint64_t get_data_index_size();
	id_t_ get_next_linked_list();
	id_t_ get_prev_linked_list();
	void set_next_linked_list(id_t_ data);
	void set_prev_linked_list(id_t_ data);
	// pointer list modififers
	/*
	  size of data is referring to the type size and the array size, whereas
	  the size of the ID is referring to just the array size, since the size
	  is assumed with the pointer type (8 bytes, but maybe more later?)
	 */
	// void add_data(
	// 	void *ptr_,
	// 	uint32_t size_,
	// 	uint64_t flags = 0);
	// void add_data(
	// 	id_t_ *ptr_,
	// 	uint32_t size_,
	// 	uint64_t flags = 0);
	// void add_data(
	// 	std::vector<uint8_t> *ptr_,
	// 	uint32_t size_,
	// 	uint64_t flags_ = 0);
	// void add_data(
	// 	std::vector<uint64_t> *ptr_,
	// 	uint32_t size_,
	// 	uint64_t flags = 0);
	// void add_data(
	// 	std::vector<id_t_> *ptr_,
	// 	uint32_t size_,
	// 	uint64_t flags = 0);
	// TODO: should enforce casting
	void add_data_one_byte_vector(
		std::vector<uint8_t> *ptr_,
		uint32_t max_size_elem_,
		uint64_t flags = 0){add_data(ptr_, {max_size_elem_}, flags | ID_DATA_BYTE_VECTOR);}
	void add_data_one_byte_vector_vector(
		std::vector<std::vector<uint8_t> > *ptr_,
		uint32_t max_size_elem_,
		uint32_t max_size_elem__,
		uint64_t flags = 0){add_data(ptr_, {max_size_elem_, max_size_elem__}, flags | ID_DATA_BYTE_VECTOR_VECTOR);}
	void add_data_eight_byte_vector(
		std::vector<uint64_t> *ptr_,
		uint32_t max_size_elem_,
		uint64_t flags = 0){add_data(ptr_, {max_size_elem_}, flags | ID_DATA_EIGHT_BYTE_VECTOR);}
	void add_data_id_vector(
		std::vector<id_t_> *ptr_,
		uint32_t max_size_elem_,
		uint64_t flags = 0){add_data(ptr_, {max_size_elem_}, flags | ID_DATA_ID_VECTOR);}
	void add_data_id(
		id_t_ *id_,
		uint32_t const_size_elem_,
		uint64_t flags = 0){add_data(id_, {const_size_elem_}, flags | ID_DATA_ID);}
	void add_data_raw(
		void *ptr_,
		uint32_t const_size_bytes_,
		uint64_t flags = 0){add_data(ptr_, {const_size_bytes_}, flags);}
	// export and import data
	// default on export is unencrypted and uncompressed, but is compressed
	// and encrypted when it is loaded into the cache (so always, currently,
	// but just not handled in this function)
	std::vector<uint8_t> export_data(uint8_t flags_, uint8_t extra);
	void import_data(std::vector<uint8_t> data);
	void rsa_decrypt_backlog();
	bool is_owner();
	std::vector<uint8_t> get_ptr_flags();
	void noexport_all_data();
	void noexp_all_data(){noexport_all_data();}
	void nonet_all_data();
	uint64_t get_last_access_timestamp_micro_s(){return last_access_timestamp_micro_s;}
};

typedef uint16_t transport_i_t;
typedef uint32_t transport_size_t;

// namespace id_transport{
// 	std::vector<uint8_t> get_entry(std::vector<uint8_t> data, transport_i_t trans_i);
// 	std::vector<uint8_t> set_entry(std::vector<uint8_t> entry, transport_i_t trans);
// };

extern std::array<uint8_t, 32> get_id_hash(id_t_ id);
extern void set_id_hash(id_t_ *id, std::array<uint8_t, 32> hash);
extern uint64_t get_id_uuid(id_t_ id);
extern void set_id_uuid(id_t_ *id, uint64_t uuid);
extern type_t_ get_id_type(id_t_ id);
extern void set_id_type(id_t_ *id, type_t_ type);

/*
  Currently unused type byte

  I want to move away from the 32-byte type ID (only really used for
  ease of debugging and creating new types), but since the speed of
  types created will be going down from here on out, I can spend the
  extra time to add them here

  This also allows me to add a type byte to the ID for faster sanity
  checking (assuming that is followed)
 */

#define TYPE_IR_REMOTE_T				1
#define TYPE_ENCRYPT_PRIV_KEY_T				2
#define TYPE_ENCRYPT_PUB_KEY_T				3
#define TYPE_CONSOLE_T					4
#define TYPE_WALLET_SET_T				5
#define TYPE_NET_PROTO_SOCKET_T				6
#define TYPE_NET_PROTO_PEER_T				7
#define TYPE_NET_PROTO_CON_REQ_T			8
#define TYPE_NET_PROTO_LINKED_LIST_REQUEST_T		9
#define TYPE_NET_PROTO_ID_REQUEST_T		      	10
#define TYPE_NET_PROTO_TYPE_REQUEST_T			11
#define TYPE_NET_SOCKET_T				12
#define TYPE_NET_PROXY_T				13
#define TYPE_TV_CHANNEL_T				14
#define TYPE_TV_WINDOW_T				15
#define TYPE_TV_MENU_ENTRY_T				16
#define TYPE_TV_MENU_T					17
#define TYPE_TV_DEV_AUDIO_T				18
#define TYPE_TV_DEV_VIDEO_T				19
#define TYPE_TV_FRAME_AUDIO_T				20
#define TYPE_TV_FRAME_VIDEO_T				21
#define TYPE_TV_FRAME_CAPTION_T				22
#define TYPE_INPUT_DEV_STANDARD_T			23
#define TYPE_ID_DISK_INDEX_T				24
#define TYPE_MATH_NUMBER_SET_T				25
#define TYPE_TV_ITEM_T					26

#define ID_NONET(x)				\
	if(true){				\
		data_id_t *tmp = PTR_ID(x, );	\
		if(tmp != nullptr){		\
			tmp->nonet_all_data();	\
		}				\
	}					

#define ID_NOEXP(x)				\
	if(true){				\
		data_id_t *tmp = PTR_ID(x, );	\
		if(tmp != nullptr){		\
			tmp->nonet_all_data();	\
		}				\
	}					

#define ID_NOEXP_NONET(x)				\
	if(true){					\
		data_id_t *tmp = PTR_ID(x, );		\
		if(tmp != nullptr){			\
			tmp->noexp_all_data();		\
			tmp->nonet_all_data();		\
		}					\
	}					

// this should be the only thing on the line, so we can
// add a ton of garbage to the end if we want
#define NEW(type, var, global_exp_rules) var = new type;if(global_exp_rules & ID_DATA_NONET){var->id.nonet_all_data();}if(gloabl_exp_rules & ID_DATA_NOEXP){var->id.noexp_all_data();}

#endif
