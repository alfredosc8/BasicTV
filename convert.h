#ifndef CONVERT_H
#define CONVERT_H
#include <vector>
#include <string>
#include <array>
// includes type byte vector
#include "id/id.h"

/*
  convert: if the conversion is simple enough to be written in plain C/C++,
  it goes here. If it needs external libraries, or needs special access
  to static variables, then it shouldn't be here (unless, of course, they
  are here first)
 */

// TODO: implement some builtin functions for VC++ and other compilers
// before opting for the slowest method

// NBO: network byte order

// TODO: actually have a list for both instead of defaulting to LE

#ifdef __sparc__
#define IS_BIG_ENDIAN
#else
#define IS_LITTLE_ENDIAN
#endif

#ifdef IS_LITTLE_ENDIAN

#ifdef __GNUC__

#define NBO_64 __builtin_bswap64
#define NBO_32 __builtin_bswap32
#define NBO_16 __builtin_bswap16

#define NBO_TO_NATIVE_16 __builtin_bswap16
#define NBO_TO_NATIVE_32 __builtin_bswap32
#define NBO_TO_NATIVE_64 __builtin_bswap64
#else

#error "no converting functions for NBO"

#endif

#endif

#ifdef IS_BIG_ENDIAN

#define NBO_64(a) (a)
#define NBO_32(a) (a)
#define NBO_16(a) (a)

#define NBO_TO_NATIVE_16(a) (a)
#define NBO_TO_NATIVE_32(a) (a)
#define NBO_TO_NATIVE_64(a) (a)
#endif

// mostly for time, but add the other ones
#define MILLI_PREFIX (0.001)
#define MICRO_PREFIX (0.000001)
#define MILLI MILLI_PREFIX
#define MICRO MICRO_PREFIX

#define METRIC(a, b) (a/b)

#define CROP_LSB(data, bits_out) (data & ~((~0) << bits_out))

namespace convert{
	namespace nbo{
		std::vector<uint8_t> to(std::vector<uint8_t>);
		std::vector<uint8_t> to(std::string);
		void to(uint8_t *, uint64_t);
		std::vector<uint8_t> from(std::vector<uint8_t>);
		std::vector<uint8_t> from(std::string);
		void from(uint8_t *, uint64_t);
	}
	namespace net{
		namespace ip{
			std::string to_string(std::string ip,
					      uint16_t port);
			std::pair<std::string, uint16_t> from_string(std::string);
		};
	}
	namespace array{
		namespace id{
			std::string to_hex(id_t_);
			id_t_ from_hex(std::string);
		}
	}
	namespace type{
		uint8_t to(std::string);
		std::string from(uint8_t);
	}
	namespace number{
		std::string to_binary(uint64_t);
		std::string to_hex(std::vector<uint8_t>);
		std::vector<uint8_t> from_hex(std::string);
	}
	/*
	  A color tuple is the RGB values plus the bytes per color. Bytes per 
	  color is used instead of bytes per pixel because, unlike SDL2 and
	  most other libaries, there is no native support for the alpha channel
	  (but that wouldn't be a bad idea for advanced menus)

	  TODO: move color into transcode namespace since the scope is limited
	  to the tv_* functions
	 */
	namespace vector{
		// currently only used for converting audio codecs from
		// a packetized system to a stream, but has wider use cases
		// as it progresses
		std::vector<uint8_t> collapse_2d_vector(
			std::vector<std::vector<uint8_t> > vector);
	};
	namespace color{
		uint64_t to(std::tuple<uint64_t, uint64_t, uint64_t, uint8_t> color);
		std::tuple<uint64_t, uint64_t, uint64_t, uint8_t> from(uint64_t color,
								       uint8_t bpc);
		std::tuple<uint64_t, uint64_t, uint64_t, uint8_t> bpc(std::tuple<uint64_t, uint64_t, uint64_t, uint8_t> color,
								      uint8_t new_bpp);
	}
	namespace string{
		std::vector<uint8_t> to_bytes(std::string str);
		std::string from_bytes(std::vector<uint8_t> bytes);
	}
};

#endif
