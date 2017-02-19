#include "encrypt.h"
#include "encrypt_rsa.h"

/*
  DER is the standard for transporting RSA public keys

  IDs can use a hashing function.

  Private keys are exported in plain text for ease of use

  TODO: allow for importing of encrypted RSA keys
*/

static void rsa_load_key(RSA **rsa, std::vector<uint8_t> key, uint8_t type){
	const uint8_t *key_start = &(key[0]);
	if(type == ENCRYPT_KEY_TYPE_PUB){
		d2i_RSAPublicKey(rsa, &key_start, key.size());
	}else if(type == ENCRYPT_KEY_TYPE_PRIV){
		d2i_RSAPrivateKey(rsa, &key_start, key.size());
	}else{
		print("invalid key type supplied", P_ERR);
	}
	if(rsa == nullptr){
		print("can't allocate RSA key:"+(std::string)ERR_error_string(ERR_get_error(), nullptr), P_ERR);
	}
}

static std::vector<uint8_t> rsa_encrypt_mod_len(RSA *rsa, std::vector<uint8_t> data, uint8_t type){
	std::vector<uint8_t> retval(RSA_size(rsa), 0);
	int32_t encrypt_retval = 0;
	if(type == ENCRYPT_KEY_TYPE_PRIV){
		encrypt_retval =
			RSA_private_encrypt(
				data.size(),
				data.data(),
				retval.data(),
				rsa,
				RSA_PKCS1_PADDING);
	}else if(type == ENCRYPT_KEY_TYPE_PUB){
		encrypt_retval =
			RSA_public_encrypt(
				data.size(),
				data.data(),
				retval.data(),
				rsa,
				RSA_PKCS1_PADDING);
	}else{
		print("invalid key type supplied", P_ERR);
	}
	if(encrypt_retval == -1){
		print("unable to decrypt RSA string:"+(std::string)ERR_error_string(ERR_get_error(), nullptr), P_ERR);
	}else{
		retval.insert(
			retval.begin(), ENCRYPT_RSA);
	}
	return retval;
}

static void rsa_encrypt_sanity_check(RSA *rsa, std::vector<uint8_t> data){
	if(rsa == nullptr){
		print("RSA key is a nullptr", P_ERR);
	}
	if(data.size() == 0){
		print("no data to encrypt", P_ERR);
	}
}


std::vector<uint8_t> rsa::encrypt(std::vector<uint8_t> data,
				  std::vector<uint8_t> key,
				  uint8_t type){
	if(key.size() == 0){
		print("key is blank, can't decode anything", P_ERR);
	}
	RSA *rsa = nullptr;
	rsa_load_key(&rsa, key, type);
	rsa_encrypt_sanity_check(rsa, data);
	const uint64_t mod_len = RSA_size(rsa);
	std::vector<uint8_t> retval;
	while(data.size() > 0){
		const uint64_t size =
			(data.size() > mod_len) ?
			(mod_len+1) : (data.size());
		std::vector<uint8_t> mod =
			rsa_encrypt_mod_len(
				rsa,
				std::vector<uint8_t>(
					data.begin(),
					data.begin()+size),
				type);
		data = std::vector<uint8_t>(
			data.begin()+size,
			data.end());
		retval.insert(
			retval.end(),
			mod.begin(),
			mod.end());
	}
	RSA_free(rsa);
	rsa = nullptr;
	return retval;
}

/*
  TODO: resize the output data to leave off any trailing memory
 */

static std::vector<uint8_t> rsa_decrypt_mod_len(RSA *rsa, std::vector<uint8_t> data, uint8_t type){
	std::vector<uint8_t> retval(RSA_size(rsa), 0);
	if(data.size() != RSA_size(rsa)+1){
		print("invalid size for mod_len decryption", P_ERR);
	}
	const uint8_t scheme = data[0];
	if(scheme != ENCRYPT_RSA){
		print("invalid scheme", P_ERR);
	}
	data.erase(data.begin());
	int32_t encrypt_retval = 0;
	if(type == ENCRYPT_KEY_TYPE_PRIV){
		encrypt_retval =
			RSA_private_decrypt(
				data.size(),
				data.data(),
				retval.data(),
				rsa,
				RSA_PKCS1_PADDING);
	}else if(type == ENCRYPT_KEY_TYPE_PUB){
		encrypt_retval =
			RSA_public_decrypt(
				data.size(),
				data.data(),
				retval.data(),
				rsa,
				RSA_PKCS1_PADDING);
	}else{
		print("invalid key type supplied", P_ERR);
	}
	if(encrypt_retval == -1){
		print("unable to decrypt RSA string:"+(std::string)ERR_error_string(ERR_get_error(), nullptr), P_ERR);
	}
	return retval;
}

static void rsa_decrypt_sanity_check(RSA *rsa, std::vector<uint8_t> data){
	if(rsa == nullptr){
		print("RSA key is a nullptr", P_ERR);
	}
	const uint64_t rsa_mod_len = RSA_size(rsa);
	if(data.size() % (rsa_mod_len+1) != 0){
		print("invalid length for RSA modulus", P_ERR);
	}
	if(data[0] != ENCRYPT_RSA){
		print("wrong encryption scheme", P_ERR);
	}
}

std::vector<uint8_t> rsa::decrypt(std::vector<uint8_t> data,
				  std::vector<uint8_t> key,
				  uint8_t type){
	if(key.size() == 0){
		print("key is blank, can't decode anything", P_ERR);
	}
	RSA *rsa = nullptr;
	rsa_load_key(&rsa, key, type);
	rsa_decrypt_sanity_check(rsa, data);
	const uint64_t mod_len = RSA_size(rsa); // bytes
	P_V(mod_len, P_NOTE);
	std::vector<uint8_t> retval;
	while(data.size() > 0){ // sanity check
		// checks encryption_scheme too (nonencrypted)
		std::vector<uint8_t> payload =
			rsa_decrypt_mod_len(
				rsa,
				std::vector<uint8_t>(
					data.begin(),
					data.begin()+mod_len+1),
				type);
		data = std::vector<uint8_t>(
			data.begin()+mod_len+1,
			data.end());
		retval.insert(
			retval.end(),
			payload.begin(),
			payload.end());
		P_V(data.size(), P_NOTE);
	}
	RSA_free(rsa);
	rsa = nullptr;
	return retval;
}

// TODO: convert this to templates for readability

std::pair<id_t_, id_t_> rsa::gen_key_pair(uint64_t bits){
	encrypt_priv_key_t *priv_key =
		new encrypt_priv_key_t;
	encrypt_pub_key_t *pub_key =
		new encrypt_pub_key_t;
	for(uint32_t i = 0;i < 2*(bits/64);i++){
		uint64_t rand_seed_tmp =
			true_rand(0, ~0);
		RAND_seed(&rand_seed_tmp, 8);
	}
	RSA *rsa_key =
		RSA_generate_key(
			bits,
			65537,
			nullptr,
			nullptr);
	if(rsa_key == nullptr){
		print("can't generate new RSA key:"+(std::string)ERR_error_string(ERR_get_error(), nullptr), P_ERR);
	}
	uint8_t *priv_buf = 0;
	int32_t priv_len =
		i2d_RSAPrivateKey(rsa_key, &priv_buf);
	priv_key->set_encrypt_key(
		std::vector<uint8_t>(
			priv_buf,
			priv_buf+priv_len),
		ENCRYPT_SCHEME_RSA);
	uint8_t *pub_buf = 0;
	int32_t pub_len =
		i2d_RSAPublicKey(rsa_key, &pub_buf);
	pub_key->set_encrypt_key(
		std::vector<uint8_t>(
			pub_buf,
			pub_buf+pub_len),
		ENCRYPT_SCHEME_RSA);
	delete[] priv_buf;
	priv_buf = nullptr;
	delete[] pub_buf;
	pub_buf = nullptr;
	RSA_free(rsa_key);
	rsa_key = nullptr;
	return std::make_pair(
		priv_key->id.get_id(),
		pub_key->id.get_id());
}
