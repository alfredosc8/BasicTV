#include "tv_frame_numbers.h"

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

uint16_t number_api::get::device(std::vector<uint8_t> data){
	uint16_t retval;
	number_sanity_fetch(&retval, 0, sizeof(retval), &data);
	return retval;
}

uint64_t number_api::get::unit(std::vector<uint8_t> data){
	uint64_t retval;
	number_sanity_fetch(&retval, sizeof(uint16_t), sizeof(retval), &data);
	return retval;
}

uint64_t number_api::get::timestamp(std::vector<uint8_t> data){
	uint64_t retval;
	number_sanity_fetch(&retval, sizeof(uint16_t)+sizeof(uint64_t), sizeof(retval), &data);
	return retval;
}

long double number_api::get::number(std::vector<uint8_t> data){
	long double retval;
	const uint64_t min_start =
		sizeof(uint16_t)+sizeof(uint64_t)+sizeof(uint64_t);
	std::vector<uint8_t> major =
		number_sanity_fetch(
			min_start,
			&data);
	std::vector<uint8_t> minor =
		number_sanity_fetch(
			min_start+4+major.size(),
			&data);
	P_V(major.size(), P_SPAM);
	P_V(minor.size(), P_SPAM);
	if(major.size() > 8 || minor.size() > 8){
		print("I need to expand this beyond 64-bits", P_ERR);
	}
	uint64_t major_int = 0, minor_int = 0;
	memcpy(&major_int, major.data(), 8);
	memcpy(&minor_int, minor.data(), 8);
	retval = (long double)(major_int) + (long double)(minor_int/LONG_MAX);
	P_V(retval, P_SPAM);
	return retval;
}

// log10((2^64)-1)

#define MINOR_SPECIES_MULTIPLIER 20

#define NUMBER_CREATE_ADD(x) retval.insert(retval.end(), (uint8_t*)&x, (uint8_t*)&x+sizeof(x))

std::vector<uint8_t> number_api::create(uint16_t device,
					long double number,
					uint64_t unit,
					uint64_t timestamp){
	std::vector<uint8_t> retval;
	device = NBO_16(device);
	unit = NBO_64(unit);
	timestamp = NBO_64(timestamp);
	uint64_t major_int = NBO_64((uint64_t)(long double)(number));
	uint32_t major_size = NBO_32(8);
	uint64_t minor_int = NBO_64((uint64_t)(((long double)((uint64_t)(number)-number)*MINOR_SPECIES_MULTIPLIER)+(long double)0.5));
	uint32_t minor_size = NBO_32(8);
	// doesn't bother with endian stuff, assumed to have been done
	NUMBER_CREATE_ADD(device);
	NUMBER_CREATE_ADD(unit);
	NUMBER_CREATE_ADD(timestamp);
	NUMBER_CREATE_ADD(major_size);
	NUMBER_CREATE_ADD(major_int);
	NUMBER_CREATE_ADD(minor_size);
	NUMBER_CREATE_ADD(minor_int);
	P_V(retval.size(), P_SPAM);
	return retval;
}

tv_frame_number_device_t::tv_frame_number_device_t() : id(this, TYPE_TV_FRAME_NUMBER_DEVICE_T){
}

tv_frame_number_device_t::~tv_frame_number_device_t(){
}

void tv_frame_number_device_t::add_raw_data(std::vector<uint8_t> data){
	raw_number_data.push_back(
		data);
}

std::vector<std::vector<uint8_t> >  tv_frame_number_device_t::get_raw_data(){
	return raw_number_data;
}
