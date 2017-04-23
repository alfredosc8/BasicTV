#include "main.h"
#include "close.h"

#include "encrypt/encrypt.h"
#include "tv/tv.h"
#include "input/input.h"
#include "net/proto/net_proto.h"
#include "console/console.h"
#include "id/id_api.h"
#include "settings.h"
#include "id/id_disk.h"

void close(){
	closing = true;
	tv_close();
	input_close();
	net_proto_close();
	console_close();
	id_api::destroy_all_data();
	ENGINE_cleanup();
	EVP_cleanup();
	ERR_free_strings();
	CRYPTO_cleanup_all_ex_data();
	// SDL_Init implicitly calls subsystems, regardless of what
	// is being passed. This is meant to solve that only
	SDL_Quit();
}
