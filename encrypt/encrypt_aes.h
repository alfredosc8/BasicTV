#ifndef ENCRYPT_AES_H
#define ENCRYPT_AES_H
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <vector>

/*
  AES encryption is currently only used for verifying information sent
  over the network (where the decryption key is sent inside of RSA
  signed data), so there is no need to define a seperate type in encrypt.h
  to facilitate AES decryption (yet)
 */
namespace aes{
	std::vector<uint8_t> encrypt(std::vector<uint8_t> data,
				     std::vector<uint8_t> key);
	std::vector<uint8_t> decrypt(std::vector<uint8_t> data,
				     std::vector<uint8_t> key);
};
#endif
