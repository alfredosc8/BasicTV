#include "encrypt.h"
#include "encrypt_rsa.h"
#include "encrypt_aes.h"

encrypt_key_t::encrypt_key_t(){}

encrypt_key_t::~encrypt_key_t(){}

void encrypt_key_t::list_virtual_data(data_id_t *id){
	id->add_data_one_byte_vector(&key,
				     65536);
	id->add_data_raw(&encryption_scheme,
			 1);
}

void encrypt_key_t::set_encrypt_key(std::vector<uint8_t> key_,
				    uint8_t encryption_scheme_){
	key = key_;
	encryption_scheme = encryption_scheme_;
}

std::pair<uint8_t, std::vector<uint8_t> >  encrypt_key_t::get_encrypt_key(){
	return std::make_pair(encryption_scheme, key);
}

encrypt_pub_key_t::encrypt_pub_key_t() : id(this, TYPE_ENCRYPT_PUB_KEY_T){
	list_virtual_data(&id);
}

encrypt_pub_key_t::~encrypt_pub_key_t(){}

encrypt_priv_key_t::encrypt_priv_key_t() : id(this, TYPE_ENCRYPT_PRIV_KEY_T){
	list_virtual_data(&id);
	id.nonet_all_data();
}

encrypt_priv_key_t::~encrypt_priv_key_t(){}

id_t_ encrypt_priv_key_t::get_pub_key_id(){
	return pub_key_id;
}

void encrypt_priv_key_t::set_pub_key_id(id_t_ pub_key_id_){
	pub_key_id = pub_key_id_;
}


/*
  This works on private and public keys in the same way. The only reason why
  there are two key types is so every instance of a private key is guaranteed
  to be non-networkable, but exportable (through the initializer)
 */
static void encrypt_pull_key_info(id_t_ id,
				  std::vector<uint8_t> *key,
				  uint8_t *encryption_scheme,
				  uint8_t *key_type){
	data_id_t *ptr = PTR_ID(id, );
	if(ptr == nullptr){
		P_V_I(id, P_SPAM)
		print("id is nullptr", P_ERR);
	}
	if(ptr->get_type() == "encrypt_pub_key_t"){
		encrypt_pub_key_t *pub_key =
			(encrypt_pub_key_t*)ptr->get_ptr();
		if(pub_key == nullptr){
			print("can't load public key", P_ERR);
		}
		std::pair<uint8_t, std::vector<uint8_t> > key_data =
			pub_key->get_encrypt_key();
		if(encryption_scheme != nullptr){
			*encryption_scheme = key_data.first;
		}
		*key = key_data.second;
		*key_type = ENCRYPT_KEY_TYPE_PUB;
 	}else if(ptr->get_type() == "encrypt_priv_key_t"){
		encrypt_priv_key_t *priv_key =
			(encrypt_priv_key_t*)ptr->get_ptr();
		if(priv_key == nullptr){
			print("can't load private key", P_ERR);
		}
		std::pair<uint8_t, std::vector<uint8_t> > key_data =
			priv_key->get_encrypt_key();
		if(encryption_scheme != nullptr){
			*encryption_scheme = key_data.first;
		}
		*key = key_data.second;
		*key_type = ENCRYPT_KEY_TYPE_PRIV;		
	}else{
		print("key ID is not a valid key", P_ERR);
		// redundant
		*key = {};
		*encryption_scheme = ENCRYPT_UNDEFINED;
	}
}

static std::vector<uint8_t> encrypt_aes192_sha256(std::vector<uint8_t> data,
						  std::vector<uint8_t> key,
						  uint8_t key_type){
	std::vector<uint8_t> retval;
	std::vector<uint8_t> aes_key;
	for(uint64_t i = 0;i < 192/8;i++){
		aes_key.push_back(
			(uint8_t)true_rand(0, 255));
	}
	std::array<uint8_t, 32> hash =
		encrypt_api::hash::sha256::gen_raw(
			data);
	retval.insert(
		retval.end(),
		aes_key.begin(),
		aes_key.end());
	retval.insert(
		retval.end(),
		hash.begin(),
		hash.end());
	retval =
		rsa::encrypt(
			retval, key, key_type);
	data = aes::encrypt(
		data, aes_key);
	retval.insert(
		retval.end(),
		data.begin(),
		data.end());
	return retval;
}

static std::vector<uint8_t> decrypt_aes192_sha256(std::vector<uint8_t> data,
						  std::vector<uint8_t> key,
						  uint8_t key_type){
	std::vector<uint8_t> retval;
	const uint16_t key_length_bytes = (4096/8)+1;
	std::vector<uint8_t> header =
		rsa::decrypt(
			std::vector<uint8_t>(
				data.begin(),
				data.begin()+key_length_bytes),
			key,
			key_type);
	const std::vector<uint8_t> aes_key(
		header.begin(),
		header.begin()+(192/8));
	std::array<uint8_t, 32> sha_hash;
	memcpy(sha_hash.data(),
	       header.data()+(192/8),
	       32);
	retval =
		aes::decrypt(
			std::vector<uint8_t>(
				data.begin()+key_length_bytes,
				data.end()),
			aes_key);
	if(encrypt_api::hash::sha256::gen_raw(retval) != sha_hash){
		print("incorrect SHA-256 hash, returning blank", P_WARN);
		retval = {};
	}
	return retval;
}

static uint8_t encrypt_gen_optimal_encrypt(std::vector<uint8_t> data,
					   std::vector<uint8_t> key){
	if(key.size()*8 < 2048+13){
		print("TODO: (possibly) implement larger than one block headers for unsafe keys", P_ERR);
	}
	if(data.size()+13 > key.size()){
		print("using AES-192 encryption", P_SPAM);
		return ENCRYPT_AES192_SHA256;
	}else{
		print("using plain RSA encryption (small payload)", P_SPAM);
		return ENCRYPT_RSA;
	}
}

std::vector<uint8_t> encrypt_api::encrypt(std::vector<uint8_t> data,
					  id_t_ key_id,
					  uint8_t encryption_scheme){
	std::vector<uint8_t> retval;
	std::vector<uint8_t> key;
	uint8_t key_encryption_scheme = 0;
	uint8_t key_type = 0;
	encrypt_pull_key_info(key_id,
			      &key,
			      &key_encryption_scheme,
			      &key_type);
	if(key_encryption_scheme != ENCRYPT_RSA){
		print("not bothering with non-RSA encryption keys for now", P_ERR);
	}
	/*
	  encryption scheme is just what the keys themselves can handle, meaning
	  I can encrypt with AES192_SHA256 perfectly fine
	 */
	if(encryption_scheme == ENCRYPT_UNDEFINED){
		encryption_scheme =
			encrypt_gen_optimal_encrypt(data, key);
	}
	P_V(encryption_scheme, P_VAR);
	switch(encryption_scheme){
	case ENCRYPT_AES192_SHA256:
		retval = encrypt_aes192_sha256(
			data, key, key_type);
		break;
	case ENCRYPT_RSA:
		// can be practical if payload is small enough
		retval = rsa::encrypt(data, key, key_type);
		break;
	case ENCRYPT_UNDEFINED:
		print("no encryption scheme is set", P_ERR);
		break;
	default:
		print("unknown encryption scheme is set", P_ERR);
		break;
	}
	// encryption scheme is ALWAYS the first byte
	retval.insert(
		retval.begin(),
		&encryption_scheme,
		&encryption_scheme+1);
	return retval;
}

std::vector<uint8_t> encrypt_api::decrypt(std::vector<uint8_t> data,
					  id_t_ key_id){
	std::vector<uint8_t> retval;
	uint8_t encryption_scheme = ENCRYPT_UNDEFINED;
	std::vector<uint8_t> key;
	uint8_t key_type = 0;
	const uint8_t message_encryption_scheme =
		data[0];
	data.erase(
		data.begin());
	encrypt_pull_key_info(
		key_id,
		&key,
		&encryption_scheme,
		&key_type);
	switch(message_encryption_scheme){
	case ENCRYPT_AES192_SHA256:
		retval = decrypt_aes192_sha256(
			data, key, key_type);
		break;
	case ENCRYPT_RSA:
		retval = rsa::decrypt(data, key, key_type);
		break;
	case ENCRYPT_UNDEFINED:
		print("no encryption scheme is set", P_ERR);
		break;
	default:
		print("unknown encryption scheme is set", P_ERR);
		break;
	}
	return retval;
}

std::array<uint8_t, 32> encrypt_api::hash::sha256::gen_raw(std::vector<uint8_t> data){
	std::array<uint8_t, 32> retval;
	SHA256_CTX sha256;
	if(SHA256_Init(&sha256) == 0){
		print("can't initialize SHA256_CTX", P_ERR);
	}
	if(SHA256_Update(&sha256, &(data[0]), data.size()) == 0){
		print("can't update SHA256_CTX", P_ERR);
	}
	if(SHA256_Final(&(retval[0]), &sha256) == 0){
		print("can't compute SHA256_CTX", P_ERR);
	}
	return retval;
}

std::string encrypt_api::hash::sha256::gen_str(std::vector<uint8_t> data){
	return gen_str_from_raw(gen_raw(data));
}

std::string encrypt_api::hash::sha256::gen_str_from_raw(std::array<uint8_t, 32> data){
	std::string retval;
	for(uint16_t i = 0;i < 32;i++){
		retval += to_hex(data[i]);
	}
	P_V_S(retval, P_VAR);
	return retval;
}

/*
  Can probably check the id_t_ first
 */

id_t_ encrypt_api::search::pub_key_from_hash(std::array<uint8_t, 32> hash){
	std::vector<id_t_> pub_key_vector =
		id_api::cache::get(
			"encrypt_pub_key_t");
	P_V(pub_key_vector.size(), P_VAR);
	for(uint64_t i = 0;i < pub_key_vector.size();i++){
		encrypt_pub_key_t *pub_key_ptr =
			PTR_DATA_PRE(
				pub_key_vector[i],
				encrypt_pub_key_t);
		if(pub_key_ptr == nullptr){
			continue;
		}
		std::array<uint8_t, 32> pub_key_hash =
			encrypt_api::hash::sha256::gen_raw(
				pub_key_ptr->get_encrypt_key().second);
		if(pub_key_hash == hash){
			P_V_S(convert::number::to_hex(
				      std::vector<uint8_t>(
					      &(hash[0]),
					      &(hash[0])+32)), P_VAR);
			P_V_S(convert::number::to_hex(
				      std::vector<uint8_t>(
					      &(pub_key_hash[0]),
					      &(pub_key_hash[0])+32)), P_VAR);
			return pub_key_vector[i];
		}
	}
	print("couldn't find public key, returning blank id", P_WARN);
	return ID_BLANK_ID;
}

id_t_ encrypt_api::search::priv_key_from_hash(std::array<uint8_t, 32> hash){
	id_t_ pub_key = pub_key_from_hash(hash);
	std::vector<id_t_> priv_keys =
		id_api::cache::get(
			TYPE_ENCRYPT_PRIV_KEY_T);
	/*
	  I'm assuming the search area is small enough for this not to be a
	  performance problem
	 */
	for(uint64_t i = 0;i < priv_keys.size();i++){
		encrypt_priv_key_t *priv_key_ptr =
			PTR_DATA(priv_keys[i],
				 encrypt_priv_key_t);
		if(priv_key_ptr == nullptr){
			continue;
		}
		P_V_S(convert::array::id::to_hex(pub_key), P_VAR);
		P_V_S(convert::array::id::to_hex(
			      priv_key_ptr->get_pub_key_id()), P_VAR);
		if(priv_key_ptr->get_pub_key_id() == pub_key){
			return priv_keys[i];
		}
	}
	print("couldn't find private key, returning blank id", P_WARN);
	return ID_BLANK_ID;
}

uint16_t encrypt_key_t::get_modulus(){
	print("implement modulus getter (differentiate between private and public)", P_CRIT);
	return 0;
}
