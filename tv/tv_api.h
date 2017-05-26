#include "../encrypt/encrypt.h"
#include "../encrypt/encrypt_rsa.h"
#ifndef TV_API_H
#define TV_API_H
// flags are the following: TV_CHAN_STREAMING, TV_CHAN_AUDIO, TV_CHAN_VIDEO
// STREAMING is known by how old the associated ID is (should be updated often)

#define TV_ITEM_STREAMING (1 << 0)
#define TV_ITEM_AUDIO (1 << 1)
#define TV_ITEM_VIDEO (1 << 2)
#define TV_ITEM_TEXT (1 << 3)
#define TV_ITEM_NUMBER (1 << 4)

namespace tv{
	namespace chan{
		id_t_ next_id(id_t_ id, uint64_t flags = 0);
		id_t_ prev_id(id_t_ id, uint64_t flags = 0);
		id_t_ rand_id(uint64_t flags = 0);
	};
	// layout is defined through tv_window_t
};

#endif
