#include "cryptocurrency.h"

wallet_set_t::wallet_set_t() : id(this, TYPE_WALLET_SET_T){
	id.add_data_one_byte_vector_vector(&prefixes, ~0, ~0);
	id.add_data_one_byte_vector_vector(&wallets, ~0, ~0);
}

wallet_set_t::~wallet_set_t(){}

void wallet_set_t::add_wallet(std::vector<uint8_t> prefix_,
			      std::vector<uint8_t> wallet_){
	prefixes.push_back(prefix_);
	wallets.push_back(wallet_);
}

std::vector<std::pair<std::vector<uint8_t>, std::vector<uint8_t> > > wallet_set_t::get_wallet_set(){
	std::vector<std::pair<std::vector<uint8_t>, std::vector<uint8_t> > > retval;
	if(unlikely(prefixes.size() != wallets.size())){
		print("prefixes and wallets do not match", P_ERR);
	}
	for(uint64_t i = 0;i < prefixes.size();i++){
		retval.push_back(
			std::make_pair(
				prefixes[i],
				wallets[i]));
	}
	return retval;
}
