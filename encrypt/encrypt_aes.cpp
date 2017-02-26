#include "../util.h"
#include "encrypt_aes.h"

static void aes_valid_key(std::vector<uint8_t> key){
	if(key.size() != 192/8){
		print("AES key is not 192 bit", P_ERR);
	}
}

static void handleErrors(){
	ERR_print_errors_fp(stderr);
}

/*
  copied from OpenSSL wiki page

  TODO: actually do this securely, possibly use either AES-NI or NaCl (when
  they support AES well enough)
 */


static int libcrypto_encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
  unsigned char *iv, unsigned char *ciphertext)
{
  EVP_CIPHER_CTX *ctx;

  int len;

  int ciphertext_len;

  /* Create and initialise the context */
  if(!(ctx = EVP_CIPHER_CTX_new())) handleErrors();

  /* Initialise the encryption operation. IMPORTANT - ensure you use a key
   * and IV size appropriate for your cipher
   * In this example we are using 256 bit AES (i.e. a 256 bit key). The
   * IV size for *most* modes is the same as the block size. For AES this
   * is 128 bits */
  if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_192_cbc(), NULL, key, iv))
    handleErrors();

  /* Provide the message to be encrypted, and obtain the encrypted output.
   * EVP_EncryptUpdate can be called multiple times if necessary
   */
  if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
    handleErrors();
  ciphertext_len = len;

  /* Finalise the encryption. Further ciphertext bytes may be written at
   * this stage.
   */
  if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) handleErrors();
  ciphertext_len += len;

  /* Clean up */
  EVP_CIPHER_CTX_free(ctx);

  return ciphertext_len;
}

static int libcrypto_decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
  unsigned char *iv, unsigned char *plaintext)
{
  EVP_CIPHER_CTX *ctx;

  int len;

  int plaintext_len;

  /* Create and initialise the context */
  if(!(ctx = EVP_CIPHER_CTX_new())) handleErrors();

  /* Initialise the decryption operation. IMPORTANT - ensure you use a key
   * and IV size appropriate for your cipher
   * In this example we are using 256 bit AES (i.e. a 256 bit key). The
   * IV size for *most* modes is the same as the block size. For AES this
   * is 128 bits */
  if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_192_cbc(), NULL, key, iv))
    handleErrors();

  /* Provide the message to be decrypted, and obtain the plaintext output.
   * EVP_DecryptUpdate can be called multiple times if necessary
   */
  if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
    handleErrors();
  plaintext_len = len;

  /* Finalise the decryption. Further plaintext bytes may be written at
   * this stage.
   */
  if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len)) handleErrors();
  plaintext_len += len;

  /* Clean up */
  EVP_CIPHER_CTX_free(ctx);

  return plaintext_len;
}

static std::vector<uint8_t> aes_raw_encrypt(std::vector<uint8_t> plaintext, std::vector<uint8_t> key, std::vector<uint8_t> iv){
	uint8_t *retval_raw = new uint8_t[plaintext.size()+128];
	uint32_t retval_size = 
		libcrypto_encrypt(plaintext.data(),
				  plaintext.size(),
				  key.data(),
				  iv.data(),
				  retval_raw);
	std::vector<uint8_t> retval(
		retval_raw,
		retval_raw+retval_size);
	delete[] retval_raw;
	retval_raw = nullptr;
	return retval;
}

static std::vector<uint8_t> aes_raw_decrypt(std::vector<uint8_t> ciphertext, std::vector<uint8_t> key, std::vector<uint8_t> iv){
	uint8_t *retval_raw = new uint8_t[ciphertext.size()+128];
	uint32_t retval_size = 
		libcrypto_decrypt(ciphertext.data(),
				  ciphertext.size(),
				  key.data(),
				  iv.data(),
				  retval_raw);
	std::vector<uint8_t> retval(
		retval_raw,
		retval_raw+retval_size);
	delete[] retval_raw;
	retval_raw = nullptr;
	return retval;
}

std::vector<uint8_t> aes::encrypt(std::vector<uint8_t> data,
				  std::vector<uint8_t> key){
	aes_valid_key(key);
	std::vector<uint8_t> retval; // enough
	std::vector<uint8_t> iv;
	for(uint64_t i = 0;i < key.size();i++){
		iv.push_back((uint8_t)true_rand(0, 255));
	}
	retval.insert(
		retval.end(),
		iv.begin(),
		iv.end());
	std::vector<uint8_t> ciphertext =
		aes_raw_encrypt(
			data,
			key,
			iv);
	retval.insert(
		retval.end(),
		ciphertext.begin(),
		ciphertext.end());
	return retval;
}

std::vector<uint8_t> aes::decrypt(std::vector<uint8_t> data,
				  std::vector<uint8_t> key){
	aes_valid_key(key);
	std::vector<uint8_t> retval;
	std::vector<uint8_t> iv(
		data.begin(),
		data.begin()+(key.size()));
	data = std::vector<uint8_t>(
		data.begin()+(key.size()),
		data.end());
	retval =
		aes_raw_decrypt(
			data,
			key,
			iv);
	return retval;
}
