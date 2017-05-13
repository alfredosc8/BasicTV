#include "math.h"
#include "stats.h"
#include "numbers.h"

#define MINOR_SPECIES_MULTIPLIER (pow(10, 20))

/*
  reads and writes are directly to the frame, don't bother with loading
  to an intermediary right now
 */

static void number_sanity_fetch(void *ptr, uint64_t start, uint64_t size, std::vector<uint8_t> *data){
	if(data->size() < start+size){
		print("can't copy, not enough room", P_ERR);
	}
	print("reading in " + std::to_string(size) + " bytes, starting at " + std::to_string(start), P_SPAM);
	memcpy(ptr, data->data()+start, size);
	convert::nbo::from((uint8_t*)ptr, size);
}

static std::vector<uint8_t> number_sanity_fetch(uint64_t start, std::vector<uint8_t> *data){
	std::vector<uint8_t> retval;
	if(data->size() < 4){
		print("not enough room to possibly encode major/minor size", P_ERR);
	}
	uint32_t size;
	memcpy(&size, data->data()+start, 4);
	size = NBO_32(size);
	if(data->size() < start+4+size){
		P_V(start+4+size, P_SPAM);
		P_V(data->size(), P_SPAM);
		print("invalid size for current number chunk", P_ERR);
	}
	retval =
		convert::nbo::from(
			std::vector<uint8_t>(
				data->begin()+start+4,
				data->begin()+start+4+size));
	return retval;
}

uint64_t math::number::get::unit(std::vector<uint8_t> data){
	uint64_t retval;
	number_sanity_fetch(&retval, sizeof(uint16_t), sizeof(retval), &data);
	return retval;
}

/*
  For some reason the minor doesn't want to export right
 */

std::pair<std::vector<uint8_t>,
	  std::vector<uint8_t> > math::number::get::raw_species(
		  std::vector<uint8_t> data){
	uint64_t start =
		sizeof(uint16_t)+sizeof(uint64_t)+sizeof(uint64_t);
	std::pair<std::vector<uint8_t>, std::vector<uint8_t> > retval;
	retval.first =
		number_sanity_fetch(
			start,
			&data);
	retval.second =
		number_sanity_fetch(
			start+4+retval.first.size(),
			&data);
	return retval;
}

long double math::number::get::number(std::vector<uint8_t> data){
	long double retval;
	std::pair<std::vector<uint8_t>, std::vector<uint8_t> > species =
		math::number::get::raw_species(
			data);
	if(species.first.size() > 8 || species.second.size() > 8){
		print("I need to expand this beyond 64-bits", P_ERR);
	}
	uint64_t major_int = 0, minor_int = 0;
	memcpy(&major_int, species.first.data(), 8);
	memcpy(&minor_int, species.second.data(), 8);
	P_V(major_int, P_SPAM);
	P_V(minor_int, P_SPAM);
	retval = (long double)(major_int) + (long double)((long double)minor_int/(long double)MINOR_SPECIES_MULTIPLIER);
	P_V(retval, P_SPAM);
	return retval;
}

#define NUMBER_CREATE_ADD(x) retval.insert(retval.end(), (uint8_t*)&x, (uint8_t*)&x+sizeof(x))

std::vector<uint8_t> math::number::create(long double number,
					uint64_t unit){
	std::vector<uint8_t> retval;
	unit = NBO_64(unit);
	uint64_t major_int =
		NBO_64((uint64_t)(long double)(number));
	uint32_t major_size =
		NBO_32(8);
	uint64_t minor_int =
		NBO_64((((long double)number-(long double)major_int)*MINOR_SPECIES_MULTIPLIER));
	uint32_t minor_size =
		NBO_32(8);
	P_V(major_int, P_SPAM);
	P_V(minor_int, P_SPAM);
	// doesn't bother with endian stuff, assumed to have been done
	NUMBER_CREATE_ADD(unit);
	NUMBER_CREATE_ADD(major_size);
	NUMBER_CREATE_ADD(major_int);
	NUMBER_CREATE_ADD(minor_size);
	NUMBER_CREATE_ADD(minor_int);
	P_V(retval.size(), P_SPAM);
	return retval;
}

math_number_set_t::math_number_set_t() : id(this, TYPE_MATH_NUMBER_SET_T){
}

math_number_set_t::~math_number_set_t(){
}

void math_number_set_t::add_raw_data(std::vector<uint8_t> data){
	raw_number_data.push_back(
		data);
}

std::vector<std::vector<uint8_t> >  math_number_set_t::get_raw_data(){
	return raw_number_data;
}


// function definitions from math API in math.h

// only useful in addition and subtraction, multiplication and division
// create new units

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
			0,
			math::number::get::unit(
				data[0]));
	for(uint64_t i = 0;i < data.size();i++){
		math_simple_add(
			retval,
			data[i]);
	}
	return retval;
}


bool math::number::cmp::greater_than(
	std::vector<uint8_t> x,
	std::vector<uint8_t> y){
	// going from most sig to least sig, return false
	// on first less than. do some stuff
}
