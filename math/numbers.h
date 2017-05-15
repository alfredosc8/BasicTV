#ifndef NUMBERS_H
#define NUMBERS_H
/*
  numbers

  Numbers can have units associated with it, and the arithmetic functions
  supplied take those into account and warn accordingly (multiplying
  
  For space and relevancy concerns, numbers only have their unit and 
  value associated with them. Originally (when directly copied from
  tv_frame), they had devices and timestamps, but those aren't needed
  here (but are still pulled from and added to those).
 */

// base SI unit or complex SI unit, doesn't matter
#define MATH_NUMBER_USE_SI 1
// use as a GPS coordinate (major and minor change value)
#define MATH_NUMBER_USE_COORD 2
// no formal units
#define MATH_NUMBER_USE_NONE 3

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

// alternative/imperial units IDs
// only distances for now, will finish later
#define MATH_NUMBER_ALT_INCH 1
#define MATH_NUMBER_ALT_FOOT 2
#define MATH_NUMBER_ALT_YARD 3
#define MATH_NUMBER_ALT_MILE 4
//#define MATH_NUMBER_ALT_KNOT 5

// base to alt conversion table (of sorts)

/*
  Doesn't have to be a set per-se, just is an expandable size
 */

typedef uint64_t math_number_unit_t;
typedef uint32_t math_number_len_t;

struct math_number_set_t{
private:
	std::vector<std::vector<uint8_t> > raw_number_data;
public:
	data_id_t id;
	math_number_set_t();
	~math_number_set_t();
	void add_raw_data(
		std::vector<uint8_t> data);
	std::vector<std::vector<uint8_t> > get_raw_data();
};

#endif
