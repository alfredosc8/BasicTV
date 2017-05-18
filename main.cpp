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

int main(int argc_, char **argv_){
	argc = argc_;
	argv = argv_;
	init();
	if(settings::get_setting("run_tests") == "true"){
		test();
	}
	try{
		running =
			settings::get_setting("init_close_only") == "false";
	}catch(...){
		running = true;
	}
	while(running){
		tv_loop();
		input_loop();
		net_proto_loop();
		console_loop();
		try{
			if(settings::get_setting("slow_iterate") == "1"){
				sleep_ms(1000);
			}
		}catch(...){}
		try{
			if(settings::get_setting("prove_iterate") == "1"){
				std::cout << "iterated" << std::endl;
			}
		}catch(...){}
	}
	close();
	std::cout << "[FIN] Program formally returning zero" << std::endl;
	return 0;
}
