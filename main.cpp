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
#include "id/id_api.h"

#include "test.h" // includes benchmarking code too
#include "init.h"
#include "loop.h"
#include "close.h"

int argc = 0;
char **argv = nullptr;

bool running = false;
bool closing = false;

id_t_ production_priv_key_id = ID_BLANK_ID;
bool id_throw_exception = true;

static uint64_t avg_iter_time = 0;
static uint64_t iter_count = 0;

int main(int argc_, char **argv_){
	argc = argc_;
	argv = argv_;
	init();
	// if(settings::get_setting("run_tests") == "true"){
	// 	const uint64_t old_id_count =
	// 		id_api::array::get_id_count();
	// 	test();
	// 	if(old_id_count != id_api::array::get_id_count()){
	// 		P_V(id_api::array::get_id_count()-old_id_count, P_WARN);
	// 		print("tests are leaking possibly invalid data, fix this", P_CRIT);
	// 	}
	// }
	try{
		running =
			settings::get_setting("init_close_only") == "false";
	}catch(...){
		running = true;
	}
	print("formally starting BasicTV, entering loop", P_NOTE);
	uint64_t start_time = get_time_microseconds();
	uint64_t working_iter_time = start_time;
	while(running){
		try{
			tv_loop();
			input_loop();
			net_proto_loop();
			console_loop();
		}catch(...){}

		// main loop specific stuff
		check_finite_execution_modes(
			iter_count,
			start_time);
		check_iteration_modifiers();
		check_print_modifiers(
			avg_iter_time);
		update_iteration_data(
			&avg_iter_time,
			&iter_count,
			&working_iter_time);
	}
	close();
	std::cout << "[FIN] Program formally returning zero" << std::endl;
	return 0;
}
