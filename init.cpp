#include "main.h"
#include "init.h"

#include "encrypt/encrypt.h"
#include "tv/tv.h"
#include "input/input.h"
#include "net/proto/net_proto.h"
#include "console/console.h"
#include "id/id_api.h"
#include "settings.h"
#include "id/id_disk.h"

/*
  All information imported has the SHA256 hash of the public key. It follows
  the following rules:

  1. The first item to be created is the owner's encrypt_priv_key_t, this
  doesn't need a SHA256 hash (as it shouldn't be networked and doesn't add
  any security). 
  2. The second item to be created is the corresponding public key.
  3. All information created will have the encryption fingerprint included
  in the ID, as well as being encrypted with that information for transport
  (not relevant here)

  Anything can be loaded perfectly fine, but it cannot request the ID of the
  public key to link it to the private key. id_throw_exception is ONLY used
  to make this connection.

  SHA256 keys aren't needed for array lookups, and no transporting should be
  done to keep this property for long enough (entire code is only around five
  lines), so this isn't as hacky as I make myself believe it is.
*/


/*
  The production private key is the private key that is associated with every
  data type created on this machine. If the information were to be imported, the
  hash would be applied to the ID, but it would be overridden by the ID
  associated with the imported data (logical, and it doesn't break the hashing)

  I made a write up on why this is needed above. In case that was deleted, then
  a tl;dr is that the public key can't reference itself because the hash needs
  to be known for get_id(), and that needs to be ran to link the private and
  public keys together (production_priv_key_id).
*/

static void bootstrap_production_priv_key_id(){
	id_api::import::load_all_of_type(
		"encrypt_pub_key_t",
		ID_API_IMPORT_FROM_DISK);
	id_api::import::load_all_of_type(
		"encrypt_priv_key_t",
		ID_API_IMPORT_FROM_DISK);
	std::vector<id_t_> all_public_keys =
		id_api::cache::get(
			"encrypt_pub_key_t");
	std::vector<id_t_> all_private_keys =
		id_api::cache::get(
			"encrypt_priv_key_t");
	encrypt_priv_key_t *priv_key = nullptr;
	encrypt_pub_key_t *pub_key = nullptr;
	if(all_private_keys.size() == 0){
		print("detected first boot, creating production id", P_NOTE);
		uint64_t bits_to_use = 4096;
		try{
			bits_to_use =
				std::stoi(
					settings::get_setting(
						"rsa_key_length"));
		}catch(...){}
		std::pair<id_t_, id_t_> key_pair =
			rsa::gen_key_pair(bits_to_use);
		priv_key =
			PTR_DATA(key_pair.first,
				 encrypt_priv_key_t);
		if(priv_key == nullptr){
			print("priv_key is a nullptr", P_ERR);
		}
		pub_key =
			PTR_DATA(key_pair.second,
				 encrypt_pub_key_t);
		if(pub_key == nullptr){
			print("pub_key is a nullptr", P_ERR);
		}
		priv_key->set_pub_key_id(key_pair.second);
		production_priv_key_id = priv_key->id.get_id();
	}else if(all_private_keys.size() == 1){
		if(all_public_keys.size() == 0){
			print("no public keys can possibly match private key, error in loading?", P_CRIT);
		}
		production_priv_key_id = all_private_keys[0];
		P_V_S(convert::array::id::to_hex(all_private_keys[0]), P_VAR);
		priv_key = PTR_DATA(all_private_keys[0], encrypt_priv_key_t);
		P_V_S(convert::array::id::to_hex(all_public_keys[0]), P_VAR);
		pub_key = PTR_DATA(all_public_keys[0], encrypt_pub_key_t);
	}else if(all_private_keys.size() > 1){
		print("I have more than one private key, make a prompt to choose one", P_ERR);
	}
	set_id_hash(&production_priv_key_id,
		    encrypt_api::hash::sha256::gen_raw(
			    pub_key->get_encrypt_key().second));
	priv_key->id.set_id(production_priv_key_id);
	id_throw_exception = false;
	priv_key->set_pub_key_id(pub_key->id.get_id());
	P_V_S(convert::array::id::to_hex(production_priv_key_id), P_NOTE);
	P_V_S(convert::array::id::to_hex(pub_key->id.get_id()), P_NOTE);
}


void init(){
	// debugging information for OpenSSL's error printing
	ERR_load_crypto_strings();
	// loads OpenSSL stuff (AES only)
	OpenSSL_add_all_algorithms();
	OPENSSL_config(nullptr);
	/*
	  settings_init() only reads from the file, it doesn't do anything
	  critical to setting default values
	*/
	// default port for ID networking
	settings::set_setting("net_port", "58486");
	settings::set_setting("net_hostname", "");
	settings::set_setting("net_open_tcp_port", "false");
	// console port
	settings::set_setting("console_port", "59000");
	// disable socks
	settings::set_setting("socks_enable", "false");
	// if SOCKS cannot be set up properly, then terminate
	settings::set_setting("socks_strict", "true");
	// SOCKS proxy ip address in ASCII
	settings::set_setting("socks_proxy_ip", "127.0.0.1");
	// SOCKS proxy port in ASCII
	settings::set_setting("socks_proxy_port", "9050");
	// enable subsystems in settings
	settings::set_setting("video", "true");
	settings::set_setting("audio", "true");
	// throw level
	settings::set_setting("throw_level", std::to_string(P_CRIT));
	// shouldn't need to disable other stuff
	settings::set_setting("run_tests", "true");
	settings::set_setting("data_folder", ((std::string)getenv("HOME"))+"/.BasicTV/");
	settings_init();

	/*
	  Using "~" doesn't work with C++, so get the information from getenv()

	  TODO: convert the input string from settings.cfg to this

	  TODO: use getuid and that stuff when getenv doesn't work (?)
	 */

	id_disk_index_t *disk_index =
		new id_disk_index_t;
	disk_index->id.noexp_all_data();
	disk_index->id.nonet_all_data();
		
	disk_index->set(
		ID_DISK_MEDIUM_HDD,
		2, // Tier 2 is lowest for non-RAM storage
		ID_DISK_TRANS_DIR,
		{ID_DISK_ENHANCE_UNDEF}, // macro to zero, here for verbosity
		settings::get_setting("data_folder"));

	bootstrap_production_priv_key_id();

	tv_init();
	input_init();
	net_proto_init();
	console_init();
}

