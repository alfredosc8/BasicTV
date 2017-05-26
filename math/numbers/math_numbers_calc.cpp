#include "../../main.h"
#include "../../util.h"
#include "math_numbers.h"
#include "math_numbers_calc.h"
#include "../math.h"


static void math_number_same_units(std::vector<std::vector<uint8_t> > data){
	uint64_t unit = 0;
	if(data.size() == 0){
		print("null set of numbers have compatiable units, but that's weird", P_NOTE);
		// probably not a warnable offense, but it might help
	}else{
		unit = math::number::get::unit(data[0]);
	}
	for(uint64_t i = 1;i < data.size();i++){
		if(unlikely(math::number::get::unit(data[i]) !=
			    unit)){
			print("incompat units, fail compat test", P_ERR);
		}
	}
	print("numbers appear to have sane units", P_DEBUG);
}

/*
  Maybe, when the pieces of data get beyond insanely large, we might be able to 
  have threads running on basic addition and subtraction of 1M+ items?
 */

static std::vector<uint8_t> math_simple_add(
	std::vector<uint8_t> x,
	std::vector<uint8_t> y){
	std::vector<uint8_t> retval;
	long double x_ =
		math::number::get::number(x);
	long double y_ =
		math::number::get::number(y);
	uint64_t x_unit =
		math::number::get::unit(x);
	uint64_t y_unit =
		math::number::get::unit(y);
	uint64_t real_unit = 0;
	if(x_unit == 0 && y_unit != 0){
		real_unit = y_unit;
	}else if(x_unit != 0 && y_unit == 0){
		real_unit = x_unit;
	}else if(x_unit != 0 && y_unit != 0){
		if(x_unit == y_unit){
			real_unit = x_unit; // or y_unit
		}else{
			print("unit mismatch", P_WARN);
		}
	}
	print("again, I really need to go over this code", P_WARN);
	return math::number::create(
		x_+y_,
		real_unit);
}

std::vector<uint8_t> math::number::calc::add(
	std::vector<std::vector<uint8_t> > data){
	math_number_same_units(data);
	if(unlikely(data.size() == 0)){
		return {};
	}
	std::vector<uint8_t> retval =
		math::number::create(
			(int64_t)0,
			math::number::get::unit(
				data[0]));
	for(uint64_t i = 0;i < data.size();i++){
		math_simple_add(
			retval,
			data[i]);
	}
	return retval;
}

