#ifndef CRYPTOCURRENCY_H
#define CRYPTOCURRENCY_H

#include "id/id.h"
#include "id/id_api.h"
#include <tuple>

/*
  General interface for all cryptocurrencies

  Ideally, this can support generating QR codes to every type of currency
 */
#define WALLET_UNDEFINED (0)
#define WALLET_CRYPTO_BITCOIN (1)
#define WALLET_CRYPTO_MONERO (2)
#define WALLET_CRYPTO_LITECOIN (3)
#define WALLET_CRYPTO_ETHEREUM (4)

struct wallet_set_t{
private:
	// BTC, ETH, XMR, etc.
	std::vector<std::vector<uint8_t> > prefixes;
	std::vector<std::vector<uint8_t> > wallets;
public:
	data_id_t id;
	wallet_set_t();
	~wallet_set_t();
	void add_wallet(std::vector<uint8_t> prefix_,
			std::vector<uint8_t> wallet_);
	
	std::vector<std::pair<std::vector<uint8_t>, std::vector<uint8_t> > > get_wallet_set();
};
#endif
