#include "stats.h"

stat_sample_set_standard_t::stat_sample_set_standard_t(){
}

stat_sample_set_standard_t::~stat_sample_set_standard_t(){
}

void stat_sample_set_standard_t::list_virtual_data(data_id_t *id){
	id->add_data((uint8_t*)&max_samples, 8);
}

stat_sample_set_t::stat_sample_set_t() : id(this, __FUNCTION__){
	list_virtual_data(&id);
	id.add_data(&x, ~0);
	id.add_data(&y, ~0);
	id.noexp_all_data(); // ideally keep them over long periods of time
	id.nonet_all_data();
}

stat_sample_set_t::~stat_sample_set_t(){
}

void stat_sample_set_t::add_sample(uint64_t x_, uint64_t y_){
	//sample_vector.push_back(std::make_pair(x, y));
	x.push_back(x_);
	y.push_back(y_);
}

std::vector<uint64_t> stat_sample_set_t::get_x(){
	return x;
}

std::vector<uint64_t> stat_sample_set_t::get_y(){
	return y;
}

void stat_sample_set_t::set_tables(std::vector<uint64_t> x_, std::vector<uint64_t> y_){
	x = x_;
	y = y_;
}

/*
  TODO: program stat_sample_set_id_t
 */

stat_sample_set_id_t::stat_sample_set_id_t() : id(this, __FUNCTION__){
	list_virtual_data(&id);
	id.add_data(&x, ~0);
	id.add_data(&y, ~0);
}

stat_sample_set_id_t::~stat_sample_set_id_t(){
}

void stat_sample_set_id_t::add_sample(uint64_t x_, id_t_ y_){
	x.push_back(x_);
	y.push_back(y_);
}

void stat_sample_set_id_t::set_tables(std::vector<uint64_t> x_, std::vector<id_t_> y_){
	x = x_;
	y = y_;
}

std::vector<uint64_t> stat_sample_set_id_t::get_x(){
	return x;
}

std::vector<id_t_> stat_sample_set_id_t::get_y(){
	return y;
}
