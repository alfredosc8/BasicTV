#include "tv_transcode_raw.h"
#include "tv_transcode.h"

std::vector<uint8_t> transcode::audio::raw::signed_to_unsigned(
	std::vector<uint8_t> signed_payload,
	uint8_t bit_depth){
	if(bit_depth != 16){
		P_V(bit_depth, P_WARN);
		print("only 16-bit PCM is allowed", P_ERR);
	}
	if((bit_depth%8) != 0){
		print("not using a divisor-of-eight bit depth, not yet supported", P_ERR);
	}
	if(bit_depth > 64){
		// pretty suspect
		P_V(bit_depth, P_WARN);
		print("bit depths above 64 aren't supported", P_ERR);
	}
	const uint64_t half = (1 << bit_depth)/2;
	P_V(half, P_NOTE);
	for(uint64_t i = 0;i < signed_payload.size();i += bit_depth/8){
		int16_t *ptr =
			(int16_t*)(&signed_payload[i]);
		(*ptr) += half;
	}
	return signed_payload;
}


std::vector<uint8_t> transcode::audio::raw::unsigned_to_signed(
	std::vector<uint8_t> unsigned_payload,
	uint8_t bit_depth){
	if((bit_depth%8) != 0){
		print("not using a divisor-of-eight bit depth, not yet supported", P_ERR);
	}
	if(bit_depth > 64){
		// pretty suspect
		P_V(bit_depth, P_WARN);
		print("bit depths above 64 aren't supported", P_ERR);
	}
	const uint64_t half = (1 << bit_depth)/2;
	P_V(half, P_NOTE);
	for(uint64_t i = 0;i < unsigned_payload.size();i += bit_depth/8){
		int16_t *ptr =
			(int16_t*)(&unsigned_payload[i]);
		(*ptr) -= half;
	}
	return unsigned_payload;
}
