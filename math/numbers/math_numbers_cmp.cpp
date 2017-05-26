#include "../../main.h"
#include "../../util.h"
#include "math_numbers.h"
#include "math_numbers_cmp.h"
#include "../math.h"

/*
  TODO: REALLY SHOULD DO THIS RIGHT
 */

bool math::number::cmp::greater_than(
	std::vector<uint8_t> x,
	std::vector<uint8_t> y){
	long double x_ =
		math::number::get::number(x);
	long double y_ =
		math::number::get::number(y);
	print("this code should really be fixed", P_WARN);
	return x_ > y_;
}
