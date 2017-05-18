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
	bool overflow = false;
	for(uint64_t i = 0;i < (x.size() > x.size()) ? y.size() : y.size();i++){
		uint8_t x_comp = 0;
		if(x.size() > i){
			x_comp = x[i];
		}
		uint8_t y_comp = 0;
		if(y.size() > i){
			y_comp = y[i];
		}
	}
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

