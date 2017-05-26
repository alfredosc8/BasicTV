#include "../../main.h"
#include "../../util.h"
#include "math_numbers.h"
#include "math_numbers_cmp.h"
#include "../math.h"

/*
  TODO: REALLY SHOULD DO THIS RIGHT
 */

#pragma message("math::number::cmp::greater_than, less_than, and equal to are not finished yet")

/*
  math::number::get::raw_species converts the numbers to NBO automatically.
  We would read it from right to left (high to low 'i'), if
 */

bool math::number::cmp::greater_than(
	std::vector<uint8_t> x,
	std::vector<uint8_t> y){
	std::pair<std::vector<uint8_t>,
		  std::vector<uint8_t> > x_species =
		math::number::get::raw_species(x);
	std::pair<std::vector<uint8_t>,
		  std::vector<uint8_t> > y_species =
		math::number::get::raw_species(y);
	for(int64_t i = (x_species.first.size() > y_species.first.size()) ?
		    x_species.first.size()-1 :
		    y_species.first.size()-1;
	    i >= 0;
	    i--){
		uint8_t x_comp = 0;
		if(x_species.first.size() > (uint64_t)i){
			x_comp = x_species.first[i];
		}
		uint8_t y_comp = 0;
		if(y_species.first.size() > (uint64_t)i){
			y_comp = y_species.first[i];
		}
		if(x_comp > y_comp){
			return true;
		}else if(x_comp < y_comp){
			return false;
		}
	}
	print("major species match, moving to minor for comparison", P_SPAM);
	for(uint64_t i = 0;
	    i < (x_species.second.size() > y_species.second.size()) ?
		    x_species.second.size() :
		    y_species.second.size();i++){
		uint8_t x_comp = 0;
		if(x_species.second.size() > i){
			x_comp = x_species.second[i];
		}
		uint8_t y_comp = 0;
		if(y_species.second.size() > i){
			y_comp = y_species.second[i];
		}
		if(x_comp > y_comp){
			return true;
		}else if(x_comp < y_comp){
			return false;
		}
	}
	return false; // exactly equal down the line, weird
}

bool math::number::cmp::equal_to(
	std::vector<uint8_t> x,
	std::vector<uint8_t> y){
	std::pair<std::vector<uint8_t>,
		  std::vector<uint8_t> > x_species =
		math::number::get::raw_species(x);
	std::pair<std::vector<uint8_t>,
		  std::vector<uint8_t> > y_species =
		math::number::get::raw_species(y);
	for(int64_t i = (x_species.first.size() > y_species.first.size()) ?
		    x_species.first.size()-1 :
		    y_species.first.size()-1;
	    i >= 0;
	    i--){
		uint8_t x_comp = 0;
		if(x_species.first.size() > (uint64_t)i){
			x_comp = x_species.first[i];
		}
		uint8_t y_comp = 0;
		if(y_species.first.size() > (uint64_t)i){
			y_comp = y_species.first[i];
		}
		if(x_comp != y_comp){
			return false;
		}
	}
	print("major species match, moving to minor for comparison", P_SPAM);
	for(uint64_t i = 0;
	    i < (x_species.second.size() > y_species.second.size()) ?
		    x_species.second.size() :
		    y_species.second.size();i++){
		uint8_t x_comp = 0;
		if(x_species.second.size() > i){
			x_comp = x_species.second[i];
		}
		uint8_t y_comp = 0;
		if(y_species.second.size() > i){
			y_comp = y_species.second[i];
		}
		if(x_comp != y_comp){
			return false;
		}
	}
	return true;
}

bool math::number::cmp::less_than(
	std::vector<uint8_t> x,
	std::vector<uint8_t> y){
	// of course this can be faster
	return !greater_than(x, y) && !equal_to(x, y);
}
