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
public:
	data_id_t id;
	wallet_set_t();
	~wallet_set_t();
	std::array<std::pair<uint16_t, std::vector<uint8_t> >, 256> wallet_set;
	
};
#endif
