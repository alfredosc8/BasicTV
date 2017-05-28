#ifndef ESCROW_H
#define ESCROW_H

/*
  Escrow system (Bitcoin only currently)

  Unlike myself, most people don't work for free. BasicTV allows for tv_item_t
  to be held in escrow until a set payment, sent by an address, is sent to a
  cryptocurrency wallet. Here is a more detailed writeup of how a typical
  usecase would work:

  A tv_item_t is sent out with a reference to a second encrypt_pub_key_t.

  When this second encrypt_pub_key_t is requested, a response is sent back
  telling the node that a certain volume of money is required to allow sending
  this ID (as defined in perm_rules_t_, which is encrypted with the rest of the
  ID information, meaning that it cannot be changed without changing the ID). 

  The hash of the tv_item_t ID must match the hash of the
  wallet_payment_request_t. If it does not, then we can assume that somebody has
  done something evil here (created their own wallet_payment_request_t and
  masquerading it), and we query another peer until we get a valid hash.

  Once the hashes are matched, we check the volume of the requested data,
  query the user to ask if they want to do this.

  Assuming they say yes, we create a wallet_payment_response_t. 

  [HERE IS WHERE THIS BECOMES BITCOIN ONLY, OR JUST COMPLICATED TO IMPLEMENT]

  The wallet_payment_response_t contains the transaction hash and a Bitcoin
  address-signed payload containing the following:
  ID of the wallet_payment_request_t
  ID of the wallet_payment_response_t
  ID of the payer's suggest peer ID

  wallet_payment_response_t is sent back to the peer who sent the request
  (only because we know they have it, no problem with multicasting if
  they are down), and any peer that has that ID can send it back to the
  network.

  PROBLEMS:

  Even though the encrypt_pub_key_t is restricted on how it can be sent,
  it can be copied over to a second type and sent out on the network.
  A malicious client could do an exhaustive search of all encrypt_pub_key_t's
  and brute-force things. 
  - The first possible solution is to not make encrypt_pub_key_t a type
    request. Instead, it has to be referred to by an ID. However, this
    just means that another data type needs to be created that refers
    to it on the network.
  - The second possible approach is to spam the network with bad
    encrypt_pub_key_t data, and make the key length for the double-encryption
    tv_item_t keys and the fake ones long enough so the decryption time
    (for the first block, uses AES scheme) makes a bruteforce attack impractical
    for most consumer hardware. Of course, faster hardware is coming out to
    make this a lot harder, but since only ~512 bytes of relevant data is
    being decrypted, we can afford to go crazy with RSA-64K

  If a malicous sender is requested, the data can be sent back without a
  payment request. The client can just disregard this information and query
  more peers until it gets a wallet_payment_request_t.

*/

struct 
#endif
