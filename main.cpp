#include "settings.h"
#include "file.h"
#include "util.h"
#include "encrypt/encrypt.h"
#include "encrypt/encrypt_rsa.h"
#include "encrypt/encrypt_aes.h"
#include "tv/tv.h"
#include "tv/tv_frame_standard.h"
#include "tv/tv_frame_audio.h"
#include "tv/tv_frame_video.h"
#include "tv/tv_window.h"
#include "tv/tv_dev_video.h"
#include "tv/tv_dev_audio.h"
#include "tv/tv_frame_video.h"
#include "tv/tv_frame_audio.h"
#include "tv/tv_frame_caption.h"
#include "tv/tv_dev.h"
#include "tv/tv_channel.h"
#include "input/input.h"
#include "input/input_ir.h"
#include "net/proto/net_proto.h"
#include "net/net.h"
#include "id/id_api.h"
#include "compress/compress.h"
#include "convert.h"
#include "console/console.h"
#include "system.h"
#include "escape.h"
#include "id/id_set.h"
#include "id/id_disk.h"

#include "test.h" // includes benchmarking code too
#include "init.h"
#include "close.h"

int argc = 0;
char **argv = nullptr;

bool running = false;
bool closing = false;

id_t_ production_priv_key_id = ID_BLANK_ID;
bool id_throw_exception = true;

/*
  Printed every minute, just gives basic network information.
  Pretty useful (also helps determine if the software is responsive).
 */

#define NEW_TAB_LINE(the_payload) (std::string(P_V_LEV_LEN+8, ' ') + (std::string)the_payload + (std::string)"\n")

/*
  TODO: I REALLY want to use math_number_set_t for this, get a hold of those
  delicious statistics functions and visualizations...
 */

static uint64_t last_print_micro_s = 0;
static uint64_t avg_iter_time = 0;
static uint64_t iter_count = 0;

static void print_stats(){
	const uint64_t print_stat_freq =
		settings::get_setting_unsigned_def(
			"print_stat_freq", 60)*1000000;
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
		      NEW_TAB_LINE(network_socket_count) +
		      NEW_TAB_LINE(network_peer_count) +
		      NEW_TAB_LINE(channel_count) +
		      NEW_TAB_LINE(item_count) +
		      NEW_TAB_LINE(avg_iter_time_), P_NOTE);
		last_print_micro_s =
			cur_time_micro_s;
	}
}

#undef NEW_TAB_LINE

int main(int argc_, char **argv_){
	argc = argc_;
	argv = argv_;
	init();
	if(settings::get_setting("run_tests") == "true"){
		const uint64_t old_id_count =
			id_api::array::get_id_count();
		test();
		if(old_id_count != id_api::array::get_id_count()){
			P_V(id_api::array::get_id_count()-old_id_count, P_WARN);
			print("tests are leaking possibly invalid data, fix this", P_CRIT);
		}
	}
	try{
		running =
			settings::get_setting("init_close_only") == "false";
	}catch(...){
		running = true;
	}
	print("formally starting BasicTV, entering loop", P_NOTE);
	uint64_t working_iter_time = get_time_microseconds();
	while(running){
		tv_loop();
		input_loop();
		net_proto_loop();
		console_loop();
		try{
			// should standardize 1 and true
			if(settings::get_setting("slow_iterate") == "1"){
				sleep_ms(1000);
			}
		}catch(...){}
		try{
			if(settings::get_setting("prove_iterate") == "1"){
				std::cout << "iterated" << std::endl;
			}
		}catch(...){}
		try{
			if(settings::get_setting("print_stats") == "true"){
				print_stats();
			}
		}catch(...){}
		// averaging stuff
		iter_count++;
		const uint64_t cur_time_micro_s =
			get_time_microseconds();
		const uint64_t iter_time_micro_s =
			cur_time_micro_s-working_iter_time;
		// works well enough
		avg_iter_time =
			((avg_iter_time*iter_count)+iter_time_micro_s)/iter_count;
		working_iter_time =
			cur_time_micro_s;
	}
	close();
	std::cout << "[FIN] Program formally returning zero" << std::endl;
	return 0;
}
