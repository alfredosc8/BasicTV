#include "compress.h"
#include "../main.h"
#include "../util.h"

#define COMPRESS_XZ 1
#define COMPRESS_ZSTD 2

/*
  Regardless of the underlying architecture, the compression_level scale will
  still be from 0-9, but will be using a multiplier to convert to whatever
  scale is used (ZSTD uses 0-22)
 */

std::vector<uint8_t> compressor::compress(std::vector<uint8_t> data,
					  uint8_t compression_level, 
					  type_t_ type){
	if(type != 0){
		// the idea was to have multiple different dictionaries for
		// the types of data, we can work fine without that for now
		print("type variable passed, but not used", P_SPAM);
	}
	std::vector<uint8_t> retval = 
		compressor::zstd::to(data,
				     compression_level,
				     0);
	retval.insert(
		retval.begin(),
		COMPRESS_ZSTD);
	return retval;
}

std::vector<uint8_t> compressor::decompress(std::vector<uint8_t> data){
	if(data.size() < 3){
		print("data to decompress is too small", P_ERR);
	}
	std::vector<uint8_t> retval;
	switch(data[0]){
	case COMPRESS_XZ:
		retval = compressor::xz::from(
			std::vector<uint8_t>(data.begin()+1, data.end()));
		break;
	case COMPRESS_ZSTD:
		retval = compressor::zstd::from(
			std::vector<uint8_t>(data.begin()+1, data.end()));
		break;
	default:
		print("invalid compression scheme for compressed data", P_ERR);
		HANG();
	}
	return retval;
}

