#include "../main.h"
#include "../util.h"
#include "compress.h"

/*
  TODO: get rid of COMPRESS_TMP_SANE_SIZE and actually use something that makes
  more sense.
 */

static void compressor_zlib_error_checker(int32_t retval){
	switch(retval){
	case Z_OK:
		break;
	case Z_MEM_ERROR:
		print("can't allocate memory", P_ERR);
		break;
	case Z_BUF_ERROR:
		print("output buffer wasn't large enough", P_ERR);
		break; // this is fixable
	default:
		print("unknown error", P_ERR);
		break;
	}
}

// TODO: actually use compression_level

std::vector<uint8_t> compressor::xz::to(std::vector<uint8_t> input,
				       uint8_t compression_level){
#ifdef __arm__
	long unsigned int retval_size =
		(input.size()*1.1)+12;
#else
	uint64_t retval_size =
		(input.size()*1.1)+12;
#endif
	if(compression_level > 9){
		compression_level = Z_BEST_COMPRESSION; // defined as 9
	}
	uint8_t *retval_char = new uint8_t[retval_size];
	compressor_zlib_error_checker(
		::compress2(
			retval_char,
			&retval_size,
			&(input[0]),
			input.size(),
			compression_level));
	std::vector<uint8_t> retval(
		retval_char,
		retval_char+retval_size);
	delete[] retval_char;
	retval_char = nullptr;
	return retval;
}

std::vector<uint8_t> compressor::xz::from(std::vector<uint8_t> input){
#ifdef __arm__
	long unsigned int retval_size =
		input.size()*3;
#else
	uint64_t retval_size =
		input.size()*3;
#endif
	uint8_t *retval_char = new uint8_t[retval_size];
	compressor_zlib_error_checker(
		uncompress(
			retval_char,
			&retval_size,
			&(input[0]),
			input.size()));
	std::vector<uint8_t> retval;
	for(uint64_t i = 0;i < retval_size;i++){
		retval.push_back(retval_char[i]);
	}
	delete[] retval_char;
	retval_char = nullptr;
	return retval;
}
