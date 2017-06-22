#ifndef MATH_NUMBERS_H
#define MATH_NUMBERS_H

#include "../../util.h"
#include "../../id/id_api.h"

/*
  Math API functions for getting and setting variable length numbers and the
  math_number_set_t datatype
 */

// base SI unit or complex SI unit, doesn't matter
#define MATH_NUMBER_USE_SI 1
// use as a GPS coordinate (major and minor change value)
#define MATH_NUMBER_USE_COORD 2
// no formal units, still a number tho
#define MATH_NUMBER_USE_NONE 3
// categorical information (don't use me in numerical analysis!!!)
#define MATH_NUMBER_USE_CAT 4

#define MATH_NUMBER_OPERATOR_BLANK 0
#define MATH_NUMBER_OPERATOR_MULTIPLY 1
#define MATH_NUMBER_OPERATOR_DIVIDE 2
#define MATH_NUMBER_OPERATOR_EXPONENT 3
#define MATH_NUMBER_OPERATOR_NEGATE 4

// SI base units, fit nicely into 3-bits

#define MATH_NUMBER_BASE_BLANK 0
#define MATH_NUMBER_BASE_METER 1
#define MATH_NUMBER_BASE_KILOGRAM 2
#define MATH_NUMBER_BASE_SECOND 3
#define MATH_NUMBER_BASE_AMPERE 4
#define MATH_NUMBER_BASE_KELVIN 5
#define MATH_NUMBER_BASE_MOLE 6
#define MATH_NUMBER_BASE_CANDELA 7
#define MATH_NUMBER_BASE_BYTE 8

#define MATH_NUMBER_PREFIX_MICRO 1

// alternative/imperial units IDs
// only distances for now, will finish later
#define MATH_NUMBER_ALT_INCH 1
#define MATH_NUMBER_ALT_FOOT 2
#define MATH_NUMBER_ALT_YARD 3
#define MATH_NUMBER_ALT_MILE 4
//#define MATH_NUMBER_ALT_KNOT 5

/*
  TODO: actually allow for compound units

  The units, as they stand now, are just SI units with metric 
  prefixes. This is OK (I guess) for internal use, but I would
  like to expand it out (shouldn't be hard, since the UNIT() macro
  should still work, but an oversimplification of the internal
  workings).
 */


#define UNIT(use, base, prefix) (use | (base << 8) | (prefix << 16))

// base to alt conversion table (of sorts)

typedef uint64_t math_number_unit_t;
typedef uint32_t math_number_len_t;

/*
  Doesn't have to strictly be numbers, IDs can be added as well.
  However, the IDs that are added have to be interpreted as numbers
  through math::number::create (should work fine).
 */

// assume undefined is numerical
#define MATH_NUMBER_DIM_UNDEF 0
#define MATH_NUMBER_DIM_NUM 1
#define MATH_NUMBER_DIM_CAT 2

struct math_number_set_t{
private:
	uint16_t dim_count = 0;
	/*
	  dim_data is only used to differentiate between dimensions
	  with categorical and numerical data

	  The only categorical data that's being used right now is IDs.
	  The use or format of the categorical data shouldn't matter, as
	  matches are found and would use internal abstraction magic to 
	  run Chi-Squared (or Chi-Cubed) tests

	  Any and all categorical data can be directly added to the vector.

	  TODO: It might make sense to, for sufficiently large pieces of 
	  categorical data, to instead do the categories based on the
	  SHA-256 hash (or something shorter, since preimage attacks aren't
	  a concern here). Maybe that could be a memory saving measure as
	  time goes on (but the data that is passed to the number set, like
	  IDs, would need the unique and random section cut off, which shouldn't
	  be needed in the first place, at least not for anything I'm doing
	  right now). I like the idea...
	 */
	std::vector<uint8_t> dim_data;
	std::vector<std::vector<uint8_t> > raw_number_data;
public:
	data_id_t id;
	math_number_set_t();
	~math_number_set_t();
	void set_dim_count(uint16_t dim_count_, std::vector<uint8_t> dim_data_);
	uint16_t get_dim_count();
	std::vector<uint8_t> get_dim_data();
	void add_raw_data(
		std::vector<std::vector<uint8_t> > data);
	std::vector<std::vector<uint8_t> > get_raw_data();
};

#endif
