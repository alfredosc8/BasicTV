#include "../../main.h"
#include "../../util.h"
#include "math_numbers.h"
#include "../math.h"

#define MINOR_SPECIES_MULTIPLIER (pow(2, 64)-1)

math_number_set_t::math_number_set_t() : id(this, TYPE_MATH_NUMBER_SET_T){
}

math_number_set_t::~math_number_set_t(){
}

void math_number_set_t::add_raw_data(std::vector<std::vector<uint8_t> > data){
	if(data.size() != dim_count){
		print("dim_count and parameter size mismatch, not adding", P_WARN);
	}else{
		for(uint64_t i = 0;i < dim_data.size();i++){
			if(dim_data[i] == MATH_NUMBER_DIM_CAT){
				data[i] = convert::nbo::to(data[i]);
			}
			// numbers are natively stored in NBO
			raw_number_data.push_back(
				data[i]);
		}
	}
}

std::vector<std::vector<uint8_t> >  math_number_set_t::get_raw_data(){
	return raw_number_data;
}

void math_number_set_t::set_dim_count(uint16_t dim_count_, std::vector<uint8_t> dim_data_){
	if(dim_data_.size() != dim_count_){
		print("dim_data and dim_count do not match", P_ERR);
		/*
		  We don't need both dim_count_ and dim_data_, but honestly it 
		  helps out with typos right now and I could care less to change
		  it over.
		*/
	}
	dim_count = dim_count_;
	dim_data = dim_data_;
}

uint16_t math_number_set_t::get_dim_count(){
	return dim_count;
}

std::vector<uint8_t> math_number_set_t::get_dim_data(){
	return dim_data;
}

static void number_sanity_fetch(void *ptr, uint64_t start, uint64_t size, std::vector<uint8_t> *data){
	if(data->size() < start+size){
		print("can't copy, not enough room", P_ERR);
	}
	memcpy(ptr, data->data()+start, size);
	convert::nbo::from((uint8_t*)ptr, size);
}

static std::vector<uint8_t> number_sanity_fetch(std::vector<uint8_t> *data){
	std::vector<uint8_t> retval;
	if(data->size() < 4){
		print("not enough room to possibly encode major/minor size", P_ERR);
	}
	uint32_t size;
	memcpy(&size, data->data(), 4);
	size = NBO_32(size);
	if(data->size() < 4+size){
		P_V(size, P_WARN);
		P_V(data->size(), P_WARN);
		print("invalid size for current number chunk", P_ERR);
	}
	retval =
		convert::nbo::from(
			std::vector<uint8_t>(
				data->begin()+4,
				data->begin()+4+size));
	/*
	  Read data needs to be truncated
	 */
	data->erase(
		data->begin(),
		data->begin()+4+size);
	return retval;
}

uint64_t math::number::get::unit(std::vector<uint8_t> data){
	uint64_t retval;
	number_sanity_fetch(&retval, 0, sizeof(retval), &data);
	return retval;
}

std::pair<std::vector<uint8_t>,
	  std::vector<uint8_t> > math::number::get::raw_species(
		  std::vector<uint8_t> data){
	uint64_t start =
		sizeof(math_number_unit_t);
	data.erase(
		data.begin(),
		data.begin()+start); // truncate unit
	std::pair<std::vector<uint8_t>, std::vector<uint8_t> > retval;
	retval.first =
		number_sanity_fetch(
			&data);
	retval.second =
		number_sanity_fetch(
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
	memcpy(&major_int, species.first.data(), species.first.size());
	memcpy(&minor_int, species.second.data(), species.second.size());
	retval = (long double)(major_int) + (long double)((long double)(minor_int/MINOR_SPECIES_MULTIPLIER));
	return retval;
}

#define NUMBER_CREATE_ADD(x) retval.insert(retval.end(), (uint8_t*)&x, (uint8_t*)&x+sizeof(x))

/*
  TODO: In order to really optimize the numbers, we need to calculate the min
  number of bytes needed to represent this (not even in powers of two, which
  is pretty nice). As of right now, it is stuck at the (somewhat reasonable)
  max of 8-bytes (64-bit), but slimming that down could help a lot if I choose
  to optimize math_number_set_t internally (specifically removing overheads
  with multiple vectors).
 */

std::vector<uint8_t> math::number::create(long double number,
					uint64_t unit){
	std::vector<uint8_t> retval;
	int64_t major_int =
		((uint64_t)(long double)(number));
	uint32_t major_size =
		(8);
	uint64_t minor_int =
		((((long double)number-(long double)major_int)*(long double)MINOR_SPECIES_MULTIPLIER));
	uint32_t minor_size =
		(8);
	unit = NBO_64(unit);
	major_int = NBO_64(major_int);
	major_size = NBO_32(major_size);
	minor_int = NBO_64(minor_int);
	minor_size = NBO_32(minor_size);
	// doesn't bother with endian stuff, assumed to have been done
	NUMBER_CREATE_ADD(unit);
	NUMBER_CREATE_ADD(major_size);
	NUMBER_CREATE_ADD(major_int);
	NUMBER_CREATE_ADD(minor_size);
	NUMBER_CREATE_ADD(minor_int);
	return retval;
}


std::vector<uint8_t> math::number::create(uint64_t number,
					  uint64_t unit){
	std::vector<uint8_t> retval;
	uint64_t major_int =
		number;
	uint32_t major_size =
		8;
	uint64_t minor_int =
		0;
	uint32_t minor_size =
		0;
	unit = NBO_64(unit);
	major_int = NBO_64(major_int);
	major_size = NBO_32(major_size);
	minor_int = NBO_64(minor_int);
	minor_size = NBO_32(minor_size);
	// doesn't bother with endian stuff, assumed to have been done
	NUMBER_CREATE_ADD(unit);
	NUMBER_CREATE_ADD(major_size);
	NUMBER_CREATE_ADD(major_int);
	NUMBER_CREATE_ADD(minor_size);
	NUMBER_CREATE_ADD(minor_int);
	return retval;
}


std::vector<uint8_t> math::number::create(int64_t number,
					  uint64_t unit){
	std::vector<uint8_t> retval;
	int64_t major_int =
		number;
	uint32_t major_size =
		8;
	uint64_t minor_int =
		0;
	uint32_t minor_size =
		0;
	unit = NBO_64(unit);
	major_int = NBO_64(major_int);
	major_size = NBO_32(major_size);
	minor_int = NBO_64(minor_int);
	minor_size = NBO_32(minor_size);
	// doesn't bother with endian stuff, assumed to have been done
	NUMBER_CREATE_ADD(unit);
	NUMBER_CREATE_ADD(major_size);
	NUMBER_CREATE_ADD(major_int);
	NUMBER_CREATE_ADD(minor_size);
	NUMBER_CREATE_ADD(minor_int);
	return retval;
}
