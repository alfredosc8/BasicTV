#include "../math.h"
#include "../../util.h"
#include "../../id/id_api.h"

/*
  These are functions that operate on math_number_set_t's, and
  NOT continuous distributions. All continuous distributions
  are handled inside of the 'dist' namespace (math::stats::dist)
*/

#pragma message("math::functions::sum_inclusive doesn't assume X is ordered, should order at least one dimension, and it should probably be the X")

std::vector<uint8_t> math::functions::sum_inclusive(
	id_t_ math_number_set_id,
	uint64_t interval_dim,
	uint64_t sum_dim,
	std::vector<uint8_t> start_sum,
	std::vector<uint8_t> end_sum){

	std::vector<uint8_t> retval;
	math_number_set_t *number_set_ptr =
		PTR_DATA(math_number_set_id,
			 math_number_set_t);
	PRINT_IF_NULL(number_set_ptr, P_ERR);

	std::vector<std::vector<uint8_t> > raw_data =
		number_set_ptr->get_raw_data();
	// should define retvals as -1, 0, and 1 for less than, equal to
	// or greather than, instead of doing this monstrocity
	for(uint64_t i = 0;i < raw_data.size();i += number_set_ptr->get_dim_count()){
		if((math::number::cmp::greater_than(raw_data[i+interval_dim], start_sum) ||
		    math::number::cmp::equal_to(raw_data[i+interval_dim], start_sum)) &&
		   (math::number::cmp::less_than(raw_data[i+interval_dim], end_sum) ||
		    math::number::cmp::equal_to(raw_data[i+interval_dim], end_sum))){
			retval = MATH_ADD(retval, raw_data[i+sum_dim]);
		}
	}
	return retval;
}
