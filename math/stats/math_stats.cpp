#include "../math.h"
#include "math_stats.h"

#include "../../id/id_api.h"

/*
  ALL return values that are not P-values are variable length numbers
  (have to be decoded through math::number::get...). Even though most
  statistics functions' outputs don't use units, I think that larger sample sets
  would benefit (namely in intermediate operations).
 */

void math::stat::set::add::throughput_number_set(
	id_t_ set_id,
	uint64_t volume,
	uint64_t timestamp_micro_s){
	math_number_set_t *number_set_ptr =
		PTR_DATA(set_id,
			 math_number_set_t);
	// throwing would be nice here
	PRINT_IF_NULL(number_set_ptr, P_ERR);
	number_set_ptr->add_raw_data(
		std::vector<std::vector<uint8_t> >(
			{
				math::number::create(
					volume, 
					UNIT(MATH_NUMBER_USE_SI,
					     MATH_NUMBER_BASE_BYTE,
					     0)),
					math::number::create(
						timestamp_micro_s,
						UNIT(MATH_NUMBER_USE_SI,
						     MATH_NUMBER_BASE_SECOND,
						     MATH_NUMBER_PREFIX_MICRO))
					}));
}		
