#include "net.h"

#include "../util.h"
#include "../id/id_api.h"

/*
  Okay, here is how this is going to work

  ALL networking statistics of each category (latency and throughput) will be
  inside of one non-exportable and non-networkable math_number_set_t.

  Latency will have two parameters:
  Response delay
  Start timestamp in microseconds

  Throughput will have three parameters:
  Size of data
  Start timestamp in microseconds
  End timestamp in microseconds

  HOWEVER, each of those two types will have two IDs attached to the end
  of them. The first ID will be for the net_socket_t used for transport,
  the second being for the net_proxy_t for transport.

  To get specific stats, net::stats::pull_net_stats_from_global is called
  with the ID in question (can be either net_proxy_t or net_socket_t). 
  A new math_number_set_t is created from all of the data held globally,
  set as NON-EXPORTABLE and NON-NETWORKABLE, and the ID is returned.

  I trust that I would safely destroy these math_number_set_t's, as I could
  (somewhat easily) see the number of IDs of each type globally.

  net_socket_t is responsible for all calls, since it is the gateway to all
  network connections.
*/

static id_t_ net_stats_throughput_global_set_id = ID_BLANK_ID;
static id_t_ net_stats_latency_global_set_id = ID_BLANK_ID;

void net_stats_init(){
	if(PTR_ID(net_stats_throughput_global_set_id, math_number_set_t) == nullptr){
		math_number_set_t *throughput_global_set_ptr =
			new math_number_set_t;
		ID_MAKE_TMP(
			throughput_global_set_ptr->id.get_id());
		throughput_global_set_ptr->set_dim_count(
			5,
			{MATH_NUMBER_DIM_NUM, // Payload size
			 MATH_NUMBER_DIM_NUM, // start timestamp
			 MATH_NUMBER_DIM_NUM, // end timestamp
			 MATH_NUMBER_DIM_CAT, // socket id
			 MATH_NUMBER_DIM_CAT}); // proxy id
	}
	if(PTR_ID(net_stats_latency_global_set_id, math_number_set_t) == nullptr){
		math_number_set_t *latency_global_set_ptr =
			new math_number_set_t;
		ID_MAKE_TMP(
			latency_global_set_ptr->id.get_id());
		latency_global_set_ptr->set_dim_count(
			4,
			{MATH_NUMBER_DIM_NUM, // response delay
			 MATH_NUMBER_DIM_NUM, // start timestamp
			 MATH_NUMBER_DIM_CAT,  // socket id
			 MATH_NUMBER_DIM_CAT}); // proxy id
	}
}

void net_stats_close(){
	id_api::destroy(net_stats_throughput_global_set_id);
	id_api::destroy(net_stats_latency_global_set_id);
}

void net::stats::add_throughput_datum(
	uint64_t byte_volume,
	uint64_t start_time_micro_s,
	uint64_t end_time_micro_s,
	id_t_ socket_id,
	id_t_ proxy_id){
	std::vector<std::vector<uint8_t> > datum({
			math::number::create(
				byte_volume,
				UNIT(MATH_NUMBER_USE_SI,
				     MATH_NUMBER_BASE_BYTE,
				     0)),
			math::number::create(
				start_time_micro_s,
				UNIT(MATH_NUMBER_USE_SI,
				     MATH_NUMBER_BASE_SECOND,
				     MATH_NUMBER_PREFIX_MICRO)),
			math::number::create(
				end_time_micro_s,
				UNIT(MATH_NUMBER_USE_SI,
				     MATH_NUMBER_BASE_SECOND,
				     MATH_NUMBER_PREFIX_MICRO)),
			std::vector<uint8_t>(
				(uint8_t*)(&socket_id[0]),
			        ((uint8_t*)&socket_id[0])+sizeof(id_t_)),
			std::vector<uint8_t>(
				(uint8_t*)(&proxy_id[0]),
			        ((uint8_t*)&proxy_id[0])+sizeof(id_t_)),
		});
	math_number_set_t *throughput_global_set_ptr =
		PTR_DATA(net_stats_throughput_global_set_id,
			 math_number_set_t);
	if(throughput_global_set_ptr == nullptr){
		print("throughput_global_set_ptr is a nullptr", P_WARN);
		return;
	}
	throughput_global_set_ptr->add_raw_data(
		datum);
}
void net::stats::add_latency_datum(
	uint64_t start_time_micro_s,
	uint64_t end_time_micro_s,
	id_t_ socket_id,
	id_t_ proxy_id){
	std::vector<std::vector<uint8_t> > datum({
			math::number::create(
				start_time_micro_s,
				UNIT(MATH_NUMBER_USE_SI,
				     MATH_NUMBER_BASE_SECOND,
				     MATH_NUMBER_PREFIX_MICRO)),
			math::number::create(
				end_time_micro_s,
				UNIT(MATH_NUMBER_USE_SI,
				     MATH_NUMBER_BASE_SECOND,
				     MATH_NUMBER_PREFIX_MICRO)),
			std::vector<uint8_t>(
				(uint8_t*)(&socket_id[0]),
			        ((uint8_t*)&socket_id[0])+sizeof(id_t_)),
			std::vector<uint8_t>(
				(uint8_t*)(&proxy_id[0]),
			        ((uint8_t*)&proxy_id[0])+sizeof(id_t_)),
		});
	math_number_set_t *throughput_global_set_ptr =
		PTR_DATA(net_stats_throughput_global_set_id,
			 math_number_set_t);
	if(throughput_global_set_ptr == nullptr){
		print("throughput_global_set_ptr is a nullptr", P_WARN);
		return;
	}
	throughput_global_set_ptr->add_raw_data(
		datum);
}
