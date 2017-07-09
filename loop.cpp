#include "loop.h"
#include "main.h"

#include "settings.h"
#include "util.h"
#include "id/id.h"
#include "id/id_api.h"

#define NEW_TAB_LINE(the_payload) (std::string(P_V_LEV_LEN+8, ' ') + (std::string)the_payload + (std::string)"\n")

static uint64_t last_print_micro_s = 0;

static void print_stats(uint64_t avg_iter_time){
	const uint64_t print_stat_freq =
		0;
	uint64_t cur_time_micro_s =
		get_time_microseconds();
	if(cur_time_micro_s-last_print_micro_s > print_stat_freq){
		std::string network_socket_count =
			"Proto Socket Count: " + std::to_string(id_api::cache::get(TYPE_NET_PROTO_SOCKET_T).size());
		std::string network_peer_count =
			"Peer Count: " + std::to_string(id_api::cache::get(TYPE_NET_PROTO_PEER_T).size());
		std::string channel_count =
			"Channel Count: " + std::to_string(id_api::cache::get(TYPE_TV_CHANNEL_T).size());
		std::string item_count =
			"Item Count: " + std::to_string(id_api::cache::get(TYPE_TV_ITEM_T).size());
		std::string avg_iter_time_ =
			"Average Iteration Frequency: " + std::to_string(1/((long double)((long double)avg_iter_time/(long double)1000000)));
		print("Routine Stats\n" +
		      id_api::cache::breakdown() + 
		      avg_iter_time_, P_NOTE);
		last_print_micro_s =
			cur_time_micro_s;
	}
}

#undef NEW_TAB_LINE


void check_finite_execution_modes(
	uint64_t iter_count,
	uint64_t start_time){
	try{
		if(settings::get_setting("finite_iter") == "true"){
			uint64_t finite_iter_count =
				std::stoi(
					settings::get_setting(
						"finite_iter_count"));
			if(iter_count > finite_iter_count){
				// not sure if this is really the
				// proper way, but it should work fine
				print("finished set execution iterations", P_NOTE);
				std::raise(SIGINT);
			}
		}
	}catch(...){}
	try{
		if(settings::get_setting("finite_time") == "true"){
			uint64_t finite_time =
				std::stoi(
					settings::get_setting(
						"finite_time_len"));
			if(get_time_microseconds()-start_time > finite_time){
				print("finished set execution time", P_NOTE);
				std::raise(SIGINT);
			}
		}
	}catch(...){}
}

void check_iteration_modifiers(){
	try{
		// should standardize 1 and true
		if(settings::get_setting("slow_iterate") == "true"){
			sleep_ms(1000);
		}
	}catch(...){}
	try{
		if(settings::get_setting("prove_iterate") == "true"){
			std::cout << "iterated" << std::endl;
		}
	}catch(...){}
}

void check_print_modifiers(
	uint64_t avg_iter_time){
	try{
		if(settings::get_setting("print_stats") == "true"){
			print_stats(avg_iter_time);
		}
	}catch(...){}
}

void update_iteration_data(
	uint64_t *avg_iter_time,
	uint64_t *iter_count, 
	uint64_t *working_iter_time){
	(*iter_count)++;
	const uint64_t cur_time_micro_s =
		get_time_microseconds();
	const uint64_t iter_time_micro_s =
		cur_time_micro_s-(*working_iter_time);
	// works well enough
	*avg_iter_time =
		(((*avg_iter_time)*(*iter_count))+iter_time_micro_s)/(*iter_count);
	*working_iter_time =
		cur_time_micro_s;
}
