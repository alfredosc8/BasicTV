#include "../main.h"
#include "tv.h"
#ifndef TV_CHANNEL_H
#define TV_CHANNEL_H
/*
  tv_channel_t:
  TV channel is a description of what a channel is. The actual stream
  content has been offset to tv_item_t.

  It has a description of the channel, a reference to a wallet set, 
  official website, thumbnail image (?), and other stuff as well.

  TV window NO LONGER NEEDS to directly refer tv_channel_t. Instead, refer
  TV item directly to TV window, and load TV channel's data just inside of
  the 'info' button or description or whatever
 */

/*
  TODO: make sure this works with all Unicode stuff
 */

struct tv_channel_t{
private:
	std::vector<uint8_t> description;
	id_t_ wallet_set_id = ID_BLANK_ID;
public:
	data_id_t id;
	tv_channel_t();
	~tv_channel_t();
	id_t_ get_wallet_set_id(){return wallet_set_id;}
	void set_wallet_set_id(id_t_ wallet_set_id_){wallet_set_id = wallet_set_id_;}
	void set_desc(std::string desc);
	std::string get_desc();
};

#endif
