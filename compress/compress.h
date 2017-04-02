#ifndef COMPRESS_H
#define COMPRESS_H
#include "../main.h"
#include "../util.h"
#include <vector>
#include <zstd.h>
#include <zstd_errors.h>
#include <zlib.h>
namespace compressor{
	namespace xz{
		std::vector<uint8_t> to(std::vector<uint8_t> input,
					   uint8_t compression_level);
		std::vector<uint8_t> from(std::vector<uint8_t> input);
	};
	namespace zstd{
		std::vector<uint8_t> to(std::vector<uint8_t> data,
					uint8_t compression_level,
					uint8_t dictionary = 0);
		std::vector<uint8_t> from(std::vector<uint8_t> data);
	};
	std::vector<uint8_t> compress(std::vector<uint8_t> data,
				      uint8_t compression_level,
				      type_t_ type);
	std::vector<uint8_t> decompress(std::vector<uint8_t> data);
};
#endif
