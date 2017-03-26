#include "compress.h"
#include "../main.h"
#include "../util.h"

#define ZSTD_CON_REQ_DICT 1
#define ZSTD_PEER_DICT 2
#define ZSTD_PUB_KEY_DICT 3
#define ZSTD_WALLET_DICT 4

/*
  Any type being networked should have a ZSTD dictionary. Smaller types should
  use them constantly, but larger types can benefit from creating it on the spot
  if it means the total size is smaller.

  TODO: create a setting that compresses the file twice, using the dictionary
  and not using the dictionary, and using the one that uses less total space

  TODO: move from simple API to full API for preloading dictionaries and
  contexts

  TODO: sanity check everything to make sure data isn't 
 */

static const std::vector<std::vector<uint8_t> > zstd_dict = {
	{}
};

static void throw_on_wrong_dict(uint8_t dictionary){
	if(dictionary >= zstd_dict.size()){
		print("dictionary doesn't exist", P_ERR);
	}
}

std::vector<uint8_t> compressor::zstd::to(std::vector<uint8_t> data,
					  uint8_t compression_level,
					  uint8_t dictionary){
	std::vector<uint8_t> retval;
	try{
		throw_on_wrong_dict(dictionary);
	}catch(...){
		dictionary = 0;
	}
	const size_t out_size = ZSTD_compressBound(data.size());
	uint8_t *out_data = new uint8_t[out_size];
	size_t out_true_size = 0;
	compression_level = (uint8_t)((compression_level*22.0/9.0)+0.5);
	if(dictionary == 0){
		out_true_size =
			ZSTD_compress(
				out_data,
				out_size,
				data.data(),
				data.size(),
				compression_level);
	}else{
		ZSTD_CCtx *ctx = ZSTD_createCCtx();
		if(ctx == nullptr){
			print("couldn't allocate zstd ctx", P_ERR);
		}
		out_true_size =
			ZSTD_compress_usingDict(
				ctx,
				out_data, out_size,
				data.data(), data.size(),
				zstd_dict[dictionary].data(), zstd_dict[dictionary].size(),
				compression_level);
		ZSTD_freeCCtx(ctx);
		ctx = nullptr;
	}
	if(ZSTD_isError(out_true_size)){
		print("zstd cannot compress file:" + (std::string)ZSTD_getErrorName(out_true_size), P_ERR);
	}else{
		// ZSTD_compress should assert true_size < size
		retval = std::vector<uint8_t>(
			out_data,
			out_data+out_true_size);
		delete[] out_data;
		out_data = nullptr;
	}
	retval.insert(
		retval.begin(),
		dictionary);
	return retval;
}

std::vector<uint8_t> compressor::zstd::from(std::vector<uint8_t> data){
	std::vector<uint8_t> retval;
	uint8_t dictionary = data[0];
	data.erase(data.begin());
	throw_on_wrong_dict(dictionary);
	// can't seem to get ZSTD_findDecompressedSize to work fine
	const uint64_t out_size =
		10*data.size();
	uint8_t *out_data = new uint8_t[out_size];
	size_t out_true_size = 0;
	if(dictionary == 0){
		out_true_size =
			ZSTD_decompress(
				out_data, out_size,
				data.data(), data.size());
	}else{	
		ZSTD_DCtx *ctx = ZSTD_createDCtx();
		if(ctx == nullptr){
			print("couldn't allocate zstd ctx", P_ERR);
		}
		out_true_size =
			ZSTD_decompress_usingDict(
				ctx,
				&out_data, out_size,
				data.data(), data.size(),
				zstd_dict.at(dictionary).data(), zstd_dict.at(dictionary).size());
		ZSTD_freeDCtx(ctx);
		ctx = nullptr;
	}
	if(ZSTD_isError(out_true_size)){
		print("zstd cannot decompress file:" + (std::string)ZSTD_getErrorName(out_true_size), P_ERR);
	}else{
		// ZSTD_compress should assert true_size < size
		if(out_true_size > out_size){
			print("true size is larger than allocated size", P_ERR);
		}
		retval = std::vector<uint8_t>(
			out_data,
			out_data+out_true_size);
		delete[] out_data;
		out_data = nullptr;
	}
	return retval;
}
