#include "math_numbers.h"
#include "../math.h"

/*
  Simple operations that access the number directly through
  the vector

  It is PRETTY DARN IMPORTANT that nothing here actually refers
  to another function, and that this entirely
 */

/*
  Might be a bit verbose and much, but it should work fine
 */

std::vector<uint8_t> math::number::simple::set_sign(
	std::vector<uint8_t> data,
	uint8_t sign){

	// units + size + one payload byte
	data[8+4+1] &= 1 << 7;
	data[8+4+1] |= (sign == MATH_NUMBER_SIGN_NEGATIVE) << 7;
	return data;
}

uint8_t math::number::simple::get_sign(
	std::vector<uint8_t> data){
	ASSERT(data.size() >= 8+4+1, P_ERR);
	return (data[8+4+1]&0b10000000) == MATH_NUMBER_SIGN_NEGATIVE;
}

std::vector<uint8_t> math::number::simple::flip_sign(
	std::vector<uint8_t> data){

	ASSERT(data.size() >= 8+4+1, P_ERR);
	return math::number::simple::set_sign(
		data,
		!math::number::simple::get_sign(data));
	
}
