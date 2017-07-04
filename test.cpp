#include "main.h"
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
#include "math/math.h"
#include "lock.h"

#include "net/proto/net_proto_socket.h"
#include "net/net_socket.h"

#include "cryptocurrency.h"

#include "test.h"

#include "tv/transcode/tv_transcode.h"

/*
  Since these tests are being ran all of the time, there are a few formatting
  requriements I have to follow for my own sake:
  1. No printing on non-errors
  2. Ideally, make each test last under a second, but make sure they are well
  tested
  3. Run as many tests as possible (that make sense)
  4. All data created inside of these tests MUST be set as non-exportable and
  non-networkable, unless doing so goes against what is being tested (but
  needs to be set after testing is done, at a minimum)

  Any test that requires more complicated code (coordination with another
  running instance, user input, etc.) is prefixed with test_nc (non-compliant)

  ALSO, IF A TEST HAS SHORTCOMINGS, MAKE NOTE OF THEM AT THE TOP

  Keep each test as simple as possible, it is impossible to debug a problem when
  the test is faulting...
*/

/*
  Should be called and used whenever we are testing some integral part of
  the ID, or otherwise need just a generic ID (i'm making this a function
  to simplify the escape tests).
 */

static id_t_ test_create_generic_id(){
	wallet_set_t *wallet_set_ptr =
		new wallet_set_t;
	wallet_set_ptr->id.set_lowest_global_flag_level(
		ID_DATA_RULE_UNDEF,
		ID_DATA_EXPORT_RULE_NEVER,
		ID_DATA_RULE_UNDEF);
	std::string totally_legit_bitcoin_wallet_please_give_me_money =
		"13dfmkk84rXyHoiZQmuYfTxGYykug1mDEZ";
	wallet_set_ptr->add_wallet(
		std::vector<uint8_t>({'B', 'T', 'C'}),
		std::vector<uint8_t>(
			(uint8_t*)&(totally_legit_bitcoin_wallet_please_give_me_money[0]),
			(uint8_t*)&(totally_legit_bitcoin_wallet_please_give_me_money[0])+
			totally_legit_bitcoin_wallet_please_give_me_money.size()));
	return wallet_set_ptr->id.get_id();
}

/*
  NON-COMPLIANT TESTS

  Not actually ran inside of the main testing function for one of a few reaons
  1. They require more advanced coordination/another node (networking mostly)
  2. They are performance tests that export data, not a functionality test
  3. They require user input to work properly
  4. I said so
 */

static void test_socket(){
	net_socket_t *first_ptr =
		new net_socket_t;
	first_ptr->set_net_ip(
		"", 59050);
	first_ptr->connect();
	net_socket_t *second_ptr =
		new net_socket_t;
	second_ptr->set_net_ip(
		"127.0.0.1", 59050);
	second_ptr->connect();
	id_t_ new_socket_id =
		first_ptr->accept();
	ASSERT(new_socket_id != ID_BLANK_ID, P_ERR);
	net_socket_t *new_ptr =
		PTR_DATA(new_socket_id,
			 net_socket_t);
	ASSERT(new_ptr != nullptr, P_ERR);
	new_ptr->send(
		std::vector<uint8_t>({'A', 'A', 'A', 'A'}));
	second_ptr->recv(4, 0);
}

/*
  Technically this is fine, but it only runs with nc-dependent parameters
 */

static void test_nc_socket_array(std::vector<std::pair<id_t_, id_t_> > socket_array){
	for(uint64_t i = 0;i < socket_array.size();i++){
		net_socket_t *first =
			PTR_DATA(socket_array[i].first,
				 net_socket_t);
		net_socket_t *second =
			PTR_DATA(socket_array[i].second,
				 net_socket_t);
		if(first == nullptr || second == nullptr){
			P_V_S(convert::array::id::to_hex(socket_array[i].first), P_VAR);
			P_V_S(convert::array::id::to_hex(socket_array[i].second), P_VAR);
			print("SOCKETS STRUCTS ARE NULL", P_ERR);
		}
		first->send("aaaa");
		sleep_ms(1);
		if(second->recv(4, NET_SOCKET_RECV_NO_HANG).size() == 0){
			print("SOCKET HAS CLOSED", P_ERR);
		}else{
			print("received data for socket number " + std::to_string(i), P_NOTE);
		}
	}
}



/*
  COMPLIANT TESTS

  These tests are all ran automatically on startup, should be under one second
  each, and don't break the standrad rules
 */


static void test_compressor(){
	std::vector<uint8_t> input_data =
		true_rand_byte_vector(1024*1024);
	std::vector<uint8_t> output_data =
		compressor::xz::from(
			compressor::xz::to(
				input_data,
				0));
	if(!std::equal(input_data.begin(), input_data.end(), output_data.begin())){
		print("input isn't equal to output in compressor", P_ERR);
	}
}

/*
  BasicTV works best when there are many open TCP connections at one
  time, because there isn't an overhead for connecting to a new client.
  However, consumer grade routers are terrible at this, so this is an
  automatic test to see how much the router can actually handle.

  UPDATE: Linux's internal TCP socket limit was reached, and the following
  source code doesn't actually test the router's capabilities.

  TODO: check to see if this information makes it to the router, or if
  it is just stuck locally (which makes sense, but defeats the purpose
  of the test).
 */

/*
  This works up until 537 (stack smashing), and I can't find the problem. If
  you are stuck at a lower number, make sure you set the file descriptor limit
  high enough (ulimit -n 65536 works for me).

  Meant as a measure of local performance and API/Linux limits. A global one
  with tables has the test_nc distinction because of coordination with another
  node (when it is created).
 */

static void test_max_tcp_sockets_local(){
	// print("Local IP address:", P_NOTE);
	// std::string ip;
	// std::cin >> ip;
	// std::vector<std::pair<id_t_, id_t_> > socket_pair;
	// bool dropped = false;
	// net_socket_t *inbound =
	// 	new net_socket_t;
	// inbound->set_net_ip("", 50000);
	// inbound->connect();
	// while(!dropped){
	// 	for(uint64_t i = 0;i < 128;i++){
	// 		net_socket_t *first =
	// 			new net_socket_t;
	// 		first->set_net_ip(ip, 50000);
	// 		first->connect();
	// 		net_socket_t *second =
	// 			new net_socket_t;
	// 		sleep_ms(1); // probably isn't needed
	// 		TCPsocket tmp_socket =
	// 			SDLNet_TCP_Accept(inbound->get_tcp_socket());
	// 		if(tmp_socket != nullptr){
	// 			second->set_tcp_socket(tmp_socket);
	// 		}else{
	// 			print("unable to receive connection request", P_ERR);
	// 		}
	// 		socket_pair.push_back(
	// 			std::make_pair(
	// 				first->id.get_id(),
	// 				second->id.get_id()));
	// 	}
	// 	test_nc_socket_array(socket_pair);
	// }
	// id_api::destroy(inbound->id.get_id());
	// for(uint64_t i = 0;i < socket_pair.size();i++){
	// 	id_api::destroy(socket_pair[i].first);
	// 	id_api::destroy(socket_pair[i].second);
	// }
}

static void test_id_transport_print_exp(std::vector<uint8_t> exp){
	P_V(exp.size(), P_VAR);
	for(uint64_t i = 0;i < exp.size();i++){
		print(std::to_string(i) + "\t" + std::to_string((int)(exp[i])) + "\t" + std::string(1, exp[i]), P_VAR);
	}
}

/*
  TODO: rework this to use different types
 */

static void test_id_transport(){
	wallet_set_t *wallet_set_ptr =
		new wallet_set_t;
	wallet_set_ptr->id.set_lowest_global_flag_level(
		ID_DATA_RULE_UNDEF,
		ID_DATA_EXPORT_RULE_NEVER,
		ID_DATA_RULE_UNDEF);
	// noexp is overridden in the export function parameters
	std::string totally_legit_bitcoin_wallet_please_give_me_money =
		"13dfmkk84rXyHoiZQmuYfTxGYykug1mDEZ";
	wallet_set_ptr->add_wallet(
		std::vector<uint8_t>({'B', 'T', 'C'}),
		std::vector<uint8_t>(
			(uint8_t*)&(totally_legit_bitcoin_wallet_please_give_me_money[0]),
			(uint8_t*)&(totally_legit_bitcoin_wallet_please_give_me_money[0])+
			totally_legit_bitcoin_wallet_please_give_me_money.size()));
	std::vector<uint8_t> payload =
		wallet_set_ptr->id.export_data(
			0,
			ID_EXTRA_COMPRESS | ID_EXTRA_ENCRYPT,
			ID_DATA_RULE_UNDEF,
			ID_DATA_EXPORT_RULE_NEVER,
			ID_DATA_RULE_UNDEF);
	id_api::destroy(wallet_set_ptr->id.get_id());
	wallet_set_ptr = nullptr;
	wallet_set_ptr =
		PTR_DATA(id_api::array::add_data(payload),
			 wallet_set_t);
	if(wallet_set_ptr == nullptr){
		print("id transport failed", P_ERR);
	}
	id_api::destroy(wallet_set_ptr->id.get_id());
	wallet_set_ptr = nullptr;
}

/*
  Tests network byte order conversions. I have no doubt it works fine, but
  this is also here for benchmarking different approaches to arbitrarially
  large strings (although isn't being used for that right now)
 */

static void test_nbo_transport(){
	// 2, 4, and 8 are optimized, 7 isn't
	std::vector<uint8_t> test =
		{'T', 'E', 'S', 'T', 'I', 'N', 'G'};
	std::vector<uint8_t> test_2 = test;
	test_2 =
		convert::nbo::from(
			convert::nbo::to(
				test_2));
	if(test != test_2){
		for(uint64_t i = 0;i < test.size();i++){
			P_V_C(test[i], P_WARN);
			P_V_C(test_2[i], P_WARN);
		}
		print("Network byte order functions failed (embarassing, I know)", P_ERR);
	}
	running = false;
}

/*
  Just to see how it reacts
 */

static void test_break_id_transport(){
	while(true){
		std::vector<uint8_t> tmp =
			true_rand_byte_vector(1024);
		data_id_t tmp_id(nullptr, 255);
		tmp_id.import_data(tmp);
	}
	running = false;
}

// currently only does key generation

static void test_rsa_key_gen(){
	std::pair<id_t_, id_t_> rsa_key_pair =
		rsa::gen_key_pair(4096);
	encrypt_priv_key_t *priv =
		PTR_DATA(rsa_key_pair.first,
			 encrypt_priv_key_t);
	ID_MAKE_TMP(rsa_key_pair.first);
	ID_MAKE_TMP(rsa_key_pair.second);
	if(priv == nullptr){
		print("priv key is a nullptr", P_ERR);
	}
	/*
	  First is a macro for the encryption type
	  Second is the DER formatted vector
	 */
	// P_V(priv->get_encrypt_key().second.size(), P_NOTE);
	encrypt_pub_key_t *pub =
		PTR_DATA(rsa_key_pair.second,
			 encrypt_pub_key_t);
	if(pub == nullptr){
		print("pub key is a nullptr", P_ERR);
	}
	id_api::destroy(rsa_key_pair.first);
	id_api::destroy(rsa_key_pair.second);
	// P_V(pub->get_encrypt_key().second.size(), P_NOTE);
}

static void test_rsa_encryption(){
	uint64_t key_len = 4096;
	std::vector<uint8_t> test_data =
		true_rand_byte_vector(1024);
	std::pair<id_t_, id_t_> rsa_key_pair =
		rsa::gen_key_pair(key_len);
	ID_MAKE_TMP(rsa_key_pair.first);
	ID_MAKE_TMP(rsa_key_pair.second);
	std::vector<uint8_t> test_data_output =
		encrypt_api::encrypt(
			test_data,
			rsa_key_pair.first);
	test_data_output =
		encrypt_api::decrypt(
			test_data_output,
			rsa_key_pair.second);
	if(test_data != test_data_output){
		print("RSA encryption test failed", P_ERR);
	}
	id_api::destroy(rsa_key_pair.first);
	id_api::destroy(rsa_key_pair.second);
}

static void test_aes(){
	std::vector<uint8_t> key = {'f','f','f','f','f','f','f','f','f','f','f','f','f','f','f','f','f','f','f','f','f','f','f','f'};
	std::vector<uint8_t> data = {'T', 'E', 'S', 'T', 'I', 'N', 'G'};
	if(aes::decrypt(
		   aes::encrypt(
			   data, key),
		   key) != data){
		print("AES encryption doesn't work", P_ERR);
	}
}

/*
  TODO: make these tests so they can be more easily ran (all of them)
 */

static std::pair<uint64_t, std::vector<uint8_t> > benchmark_timed_encryption(std::vector<uint8_t> data, id_t_ key){
	std::pair<uint64_t, std::vector<uint8_t> > retval;
	const uint64_t start_time_micro_s =
		get_time_microseconds();
	retval.second = 
		encrypt_api::encrypt(data, key);
	retval.first =
		get_time_microseconds()-start_time_micro_s;
	return retval;
}

static uint64_t benchmark_timed_decryption(std::vector<uint8_t> data, id_t_ key){
	const uint64_t start_time_micro_s =
		get_time_microseconds();
	encrypt_api::decrypt(data, key);
	return get_time_microseconds()-start_time_micro_s;
}

/*
  benchmark stuff doesn't have to follow print rules (yet?)
 */

static std::pair<uint64_t, uint64_t> encryption_benchmark_datum(std::vector<uint8_t> data,
								std::pair<id_t_, id_t_> keys,
								std::string scheme){
	std::pair<uint64_t, uint64_t> retval;
	const uint64_t start_time_micro_s =
		get_time_microseconds();
	std::vector<uint8_t> encrypt_data;
	if(scheme == "rsa"){
		print("forcing RSA for benchmark datum", P_NOTE);
		encrypt_data =
			encrypt_api::encrypt(
				data, keys.first, ENCRYPT_RSA);
	}else if(scheme == "aes"){
		print("forcing AES-192 for benchmark datum", P_NOTE);
		encrypt_data =
			encrypt_api::encrypt(
				data, keys.first, ENCRYPT_AES192_SHA256);
	}else{
		print("letting encryption API choose scheme for benchmark datum", P_NOTE);
		encrypt_data =
			encrypt_api::encrypt(data, keys.first);
	}
	retval.first = get_time_microseconds()-start_time_micro_s;
	encrypt_api::decrypt(encrypt_data, keys.second);
	retval.second = get_time_microseconds()-(retval.first+start_time_micro_s);
	return retval;
}

static void benchmark_encryption(std::string method){
	// payload of data, encryption time, decryptiont ime
	std::vector<uint32_t> benchmark_data;
	for(uint64_t i = 1;i <= 1024;i++){
		benchmark_data.push_back(i);
	}
	std::pair<id_t_, id_t_> rsa_key_pair =
		rsa::gen_key_pair(4096); // TODO: modify encrypt API to not assume this
	ID_MAKE_TMP(rsa_key_pair.first);
	ID_MAKE_TMP(rsa_key_pair.second);
	std::ofstream out(method + ".bench");
	if(out.is_open() == false){
		print("can't open benchmark output file", P_ERR);
	}
	std::vector<uint8_t> payload;
	for(uint64_t i = 0;i < benchmark_data.size();i++){
		const uint64_t size_bytes = (benchmark_data[i]*1024*1024);
		if(payload.size() > size_bytes){
			payload.erase(
				payload.begin()+size_bytes,
				payload.end());
		}else if(payload.size() < size_bytes){
			payload.insert(
				payload.end(),
				size_bytes-payload.size(),
				0b10101010);
		}
		std::pair<uint64_t, uint64_t> datum =
			encryption_benchmark_datum(
				payload,
				rsa_key_pair,
				method);
		// P_V(size_bytes/(1024*1024), P_NOTE);
		// P_V_S(get_readable_time(datum.first), P_NOTE);
		// P_V_S(get_readable_time(datum.second), P_NOTE);
		out << size_bytes << " " << datum.first << " " << datum.second << std::endl;
		print(std::to_string(benchmark_data.size()-i-1) + " left to go", P_NOTE);
	}
	out.close();
	print("benchmark completed", P_NOTE);
}

static void test_escape_string(){
	const uint8_t escape_char = 0xFF;
	id_t_ wallet_set_id =
		test_create_generic_id();
	wallet_set_t *wallet_set_ptr =
		PTR_DATA(wallet_set_id, wallet_set_t);
	if(wallet_set_ptr == nullptr){
		print("wallet_set_ptr is a nullptr", P_ERR);
	}
	std::vector<uint8_t> payload =
		wallet_set_ptr->id.export_data(
			0,
			0,
			ID_DATA_RULE_UNDEF,
			ID_DATA_EXPORT_RULE_ALWAYS,
			ID_DATA_RULE_UNDEF);
	std::vector<uint8_t> escaped_data =
		escape_vector(
			payload,
			escape_char);
	print("complete chunk size (unescaped): " + std::to_string(payload.size()), P_VAR);
	print("complete chunk size (escaped): "  + std::to_string(escaped_data.size()), P_VAR);
	uint32_t cruft_size = escaped_data.size()/2;
	escaped_data.insert(
		escaped_data.end(),
		escaped_data.begin(),
		escaped_data.begin()+cruft_size); // intentionally create second half
	print("complete chunk size (fragmented half added): " + std::to_string(escaped_data.size()), P_VAR);
	std::pair<std::vector<std::vector<uint8_t> >, std::vector<uint8_t> > deconstructed =
		unescape_all_vectors(
			escaped_data,
			escape_char);
	print("escaped chunk count: " + std::to_string(deconstructed.first.size()), P_VAR);
	print("escaped chunk size: " + std::to_string(deconstructed.first[0].size()), P_VAR);
	if(deconstructed.first.size() != 1){
		P_V(deconstructed.first.size(), P_WARN);
		print("incorrect vector count from unescape_all_vectors", P_ERR);
	}
	if(deconstructed.first[0].size() != payload.size()){
		P_V(payload.size(), P_WARN);
		P_V(deconstructed.first[0].size(), P_WARN);
		print("incorrect vector size from unescape_all_vectors", P_ERR);
	}
	if(deconstructed.first[0] != payload){
		// not going to print, probably breaks the terminal...
		print("incorrect vector payload from unescape_all_vectors", P_ERR);
	}
	if(deconstructed.second.size() != cruft_size){
		P_V(cruft_size, P_WARN);
		P_V(deconstructed.second.size(), P_WARN);
		print("incorrect extra size from unescape_all_vectors", P_ERR);
	}
	id_api::destroy(wallet_set_id);
}

static void test_id_set_compression(){
	std::vector<id_t_> id_set;
	std::array<uint8_t, 32> hash;
	for(uint64_t i = 1;i < 1024;i++){
		// i is the UUID
		if(true_rand(0, 30) == 0){
			hash = encrypt_api::hash::sha256::gen_raw(
				true_rand_byte_vector(64));

		}
		id_t_ tmp_id;
		set_id_uuid(&tmp_id, i);
		set_id_hash(&tmp_id, hash);
		set_id_type(&tmp_id, TYPE_NET_PROTO_PEER_T);
		id_set.push_back(tmp_id);
	}
	std::vector<uint8_t> id_set_compact =
		compact_id_set(id_set);
	std::vector<id_t_> id_set_new =
		expand_id_set(id_set_compact);
	if(id_set_new == id_set){
		// const long double compression_ratio =
		// 	(id_set.size()*sizeof(id_t_))/(id_set_compact.size());
		// P_V(compression_ratio, P_NOTE);
	}else{
		// P_V(id_set.size(), P_NOTE);
		// P_V(id_set_new.size(), P_NOTE);
		uint64_t greater = id_set.size();
		if(id_set_new.size() > greater){
			greater = id_set_new.size();
		}
		for(uint64_t i = 0;i < greater;i++){
			if(id_set.size() > i){
				P_V_S(convert::array::id::to_hex(id_set[i]), P_NOTE);
			}
			if(id_set_new.size() > i){
				P_V_S(convert::array::id::to_hex(id_set_new[i]), P_NOTE);
			}
		}
		print("id_set compression failed (not matching)", P_ERR);
	}
	running = false;
}

static void test_number_cmp(){
	std::vector<uint8_t> number_one =
		math::number::create(
			true_rand(0, ~(uint64_t)0),
			0);
	std::vector<uint8_t> number_two =
		math::number::create(
			true_rand(0, ~(uint64_t)0),
			0);
        uint64_t number_one_real =
		math::number::get::number(number_one);
        uint64_t number_two_real =
		math::number::get::number(number_two);
	if((number_one_real > number_two_real) !=
	   math::number::cmp::greater_than(number_one, number_two)){
		print("greather than number comparison is wrong", P_ERR);
	}
	if((number_one_real == number_two_real) !=
	   math::number::cmp::equal_to(number_one, number_two)){
		print("equal to number comparison is wrong", P_ERR);
	}
	if((number_one_real < number_two_real) !=
	   math::number::cmp::less_than(number_one, number_two)){
		print("less than number comparison is wrong", P_ERR);
	}
}

static void benchmark_encryption(){
	try{
		if(settings::get_setting("benchmark_encryption") == "rsa"){
			benchmark_encryption("rsa");
		}else if(settings::get_setting("benchmark_encryption") == "aes"){
			benchmark_encryption("aes");
		}else if(settings::get_setting("benchmark_encryption") != ""){
			benchmark_encryption("");
		}
	}catch(...){} // don't expect it to be set unless it is true
}

/*
  tv_frame_number_device_t is just a dummy holder of std::vector, so there's no
  point in testing that right now (I might when there is a more sophisticated 
  setup).
 */

#define TV_NUMBER_CHECK_VALUE(x) if(!BETWEEN(x-0.001, math::number::get::x(device_sensor), x+0.001)){print((std::string)#x + " doesn't match", P_WARN);P_V(math::number::get::x(device_sensor), P_WARN);P_V(x, P_WARN);}

static void test_math_number_set(){
	for(uint64_t i = 0;i < 128;i++){
		const long double number = i*3.1415;
		const uint64_t unit = MATH_NUMBER_USE_NONE;
		std::vector<uint8_t> device_sensor =
			math::number::create(
				number,
				unit);
		TV_NUMBER_CHECK_VALUE(number);
		TV_NUMBER_CHECK_VALUE(unit);
	}
	for(uint64_t i = 0;i < 65536;i++){
		const uint64_t number = i;
		const uint64_t unit = MATH_NUMBER_USE_NONE;
		std::vector<uint8_t> device_sensor =
			math::number::create(
				number,
				unit);
		TV_NUMBER_CHECK_VALUE(number);
		TV_NUMBER_CHECK_VALUE(unit);
	}
}

#undef TV_NUMBER_CHECK_VALUE

/*
  This is going to be a mess
 */

void test_net_proto_socket_transcoding(){
	net_socket_t *intermediate_socket = new net_socket_t;
	uint16_t port = 59999;
	/*
	  One pretty cool side-effect of this code is it keeps looping until
	  one port opens and uses it, since ports fit nicely (with the exception
	  of 0) into a 16-bit variable
	 */
	while(intermediate_socket->get_socket_fd() == 0){
		port++; // 60000 is the typical
		intermediate_socket->set_net_ip("", port); // blank means accepts incoming
		intermediate_socket->connect();
	}
	std::vector<std::pair<net_proto_socket_t *, net_socket_t *> > socket_vector =
		{std::make_pair(new net_proto_socket_t,
				new net_socket_t),
		 std::make_pair(new net_proto_socket_t,
				nullptr)};
	socket_vector[0].first->id.set_lowest_global_flag_level(
		ID_DATA_RULE_UNDEF,
		ID_DATA_EXPORT_RULE_NEVER,
		ID_DATA_RULE_UNDEF);
	socket_vector[1].first->id.set_lowest_global_flag_level(
		ID_DATA_RULE_UNDEF,
		ID_DATA_EXPORT_RULE_NEVER,
		ID_DATA_RULE_UNDEF);
	// as of right now, there shouldn't be any problems with recycling my
	// peer, so long as we are just testing this. This wouldn't normally
	// fly in software since it (should) never try and send something
	// to itself
	socket_vector[0].first->set_peer_id(
		net_proto::peer::get_self_as_peer());
	socket_vector[1].first->set_peer_id(
		net_proto::peer::get_self_as_peer());

	socket_vector[0].first->set_socket_id(
		socket_vector[0].second->id.get_id());
	// 0 attempts a connect, intermediate_socket accepts, shifts ownership
	// to 1, and allows for transmission of data over the proto_socket
	socket_vector[0].second->set_net_ip("127.0.0.1", port);
	socket_vector[0].second->connect();
	// accept incoming
	// while((new_socket = SDLNet_TCP_Accept(intermediate_socket->get_tcp_socket())) == nullptr){
	while((socket_vector[1].second = PTR_DATA(intermediate_socket->accept(), net_socket_t)) == nullptr){
		sleep_ms(1);
	}
	socket_vector[1].first->set_socket_id(
		socket_vector[1].second->id.get_id());
	// load, export, delete, send, reload
	id_t_ wallet_set_id = test_create_generic_id();
	socket_vector[0].first->send_id(wallet_set_id);
	// checks for having it are done on read, not send, we're fine
	id_api::destroy(wallet_set_id);
	socket_vector[1].first->update();
	if(PTR_ID(wallet_set_id, ) == nullptr){
		print("net_proto_socket transcoding failed", P_ERR);
	}
	id_api::destroy(socket_vector[0].first->id.get_id());
	id_api::destroy(socket_vector[0].second->id.get_id());
	id_api::destroy(socket_vector[1].first->id.get_id());
	id_api::destroy(socket_vector[1].second->id.get_id());
	id_api::destroy(intermediate_socket->id.get_id());
	socket_vector[0].first = nullptr;
	socket_vector[0].second = nullptr;
	socket_vector[1].first = nullptr;
	socket_vector[1].second = nullptr;
	intermediate_socket = nullptr;
}

/*
  nc = non-compliant (requires some form of user intervention or breaks some
  other rule)
 */

// void test_nc(){
// 	test_nc_socket();
// }

/*
  Ran with --run_tests (enabled by default)
 */

/*
  Might be able to multithread (?)
 */

// #define RUN_TEST(test) try{test();print("TEST " + fix_to_length(((std::string)#test), 40) + " OK", P_NOTE);}catch(...){print((std::string)(#test) + " FAIL", P_ERR);throw std::runtime_error("failed test");}

#define RUN_TEST(test) test()

/*
  Golden Rule for Tests:

  If a problem is complicated enough that it requires re-writing a test to
  debug, more basic tests need to be ran more thoroughly. When in doubt,
  feed IDs and random data.
 */

void test_lock(){
	lock_t test_lock;
	for(uint64_t i = 0;i < 256;i++){
		test_lock.lock();
	}
	for(uint64_t i = 0;i < 256;i++){
		test_lock.unlock();
	}
}

void test_id_api_raw_fetch(){
	const id_t_ generic_id =
		test_create_generic_id();
	const extra_t_ extra =
		ID_EXTRA_ENCRYPT | ID_EXTRA_COMPRESS;
	data_id_t *data_id_ptr =
		PTR_ID(generic_id, );
	const mod_inc_t_ mod_inc =
		data_id_ptr->get_mod_inc();
	const type_t_ type =
		data_id_ptr->get_type_byte();
	const std::vector<uint8_t> export_payload =
		data_id_ptr->export_data(
			0,
			ID_EXTRA_ENCRYPT | ID_EXTRA_COMPRESS,
			ID_DATA_RULE_UNDEF,
			ID_DATA_RULE_UNDEF,
			ID_DATA_RULE_UNDEF);
	if(generic_id != id_api::raw::fetch_id(export_payload)){
		print("id mismatch", P_ERR);
	}
	if(extra != id_api::raw::fetch_extra(export_payload)){
		print("extra mismatch", P_ERR);
	}
	if(mod_inc != id_api::raw::fetch_mod_inc(export_payload)){
		P_V(mod_inc, P_WARN);
		P_V(id_api::raw::fetch_mod_inc(export_payload), P_WARN);
		print("mod_inc mismatch", P_ERR);
	}
	if(type != id_api::raw::fetch_type(export_payload)){
		print("type mismatch", P_ERR);
	}
}

static std::vector<uint8_t> get_standard_sine_wave_form(){
	std::vector<uint8_t> retval;
	for(uint64_t i = 0;i < 48000*10;i++){
		uint16_t tmp =
			(uint16_t)((long double)(sin(1000 * (2 * 3.1415) * i / 44100))*65535);
		retval.push_back((uint8_t)(tmp&0xFF));
		retval.push_back((uint8_t)((tmp>>8)&0xFF));
	}
	return retval;
}

/*
  Generates a sine wave, writes it to a file, encodes and decodes it with
  the specified format, and writes those raw samples to a file. I can't
  export to Opus yet since I haven't written the proper bindings to libopusfile
 */

void test_audio_format(uint8_t format){
	tv_audio_prop_t current_audio_prop;
	current_audio_prop.set_format(
		format);
	current_audio_prop.set_flags(
		TV_AUDIO_PROP_FORMAT_ONLY);

	tv_audio_prop_t output_audio_prop;
	output_audio_prop.set_format(
		format);
	output_audio_prop.set_flags(
		TV_AUDIO_PROP_FORMAT_ONLY);
	
	std::vector<uint8_t> samples =
		get_standard_sine_wave_form();
	std::vector<uint8_t> samples_buffer =
		samples;
	
	uint32_t sampling_freq = 48000;
	uint8_t bit_depth = 16;
	uint8_t channel_count = 1;
	
	std::vector<std::vector<uint8_t> > encoded_payload =
		transcode::audio::raw::to_codec(
			&samples_buffer,
			sampling_freq,
			bit_depth,
			channel_count,
			&current_audio_prop);

	encoded_payload =
		transcode::audio::frames::to_codec(
			transcode::audio::codec::to_frames(
				&encoded_payload,
				&current_audio_prop,
				&output_audio_prop,
				1000*1000),
			&output_audio_prop);
	std::vector<uint8_t> decoded_payload =
		transcode::audio::codec::to_raw(
			&encoded_payload,
			&current_audio_prop,
			&sampling_freq,
			&bit_depth,
			&channel_count);

	if(decoded_payload != samples){
		P_V(samples.size(), P_VAR);
		P_V(samples_buffer.size(), P_VAR);
		P_V(encoded_payload.size(), P_VAR);
		P_V(decoded_payload.size(), P_VAR);
		print("lossy conversion took place", P_WARN);
	}
	if(decoded_payload.size() != samples.size()){
		P_V(samples.size(), P_VAR);
		P_V(samples_buffer.size(), P_VAR);
		P_V(encoded_payload.size(), P_VAR);
		P_V(decoded_payload.size(), P_VAR);
		print("different number of samples", P_WARN);
	}
	tv_transcode_state_encode_codec_t encode_codec =
		encode_codec_lookup(
			TV_AUDIO_FORMAT_WAV);
	tv_audio_prop_t wave_file_audio_prop;
	wave_file_audio_prop.set_format(
		TV_AUDIO_FORMAT_WAV);
	wave_file_audio_prop.set_snippet_duration_micro_s(
		5*1000*1000); // force this for writing to files
	tv_transcode_encode_state_t *encode_state =
		encode_codec.encode_init_state(
			&wave_file_audio_prop);
	/*
	  There is no general file-writing API for exporting encoded data,
	  and in most cases, it's more complicated than just routing the
	  encoded data to a file (Opus uses OGG metadata). However, with
	  WAV being a non-state codec, and the frames being individual files,
	  we can set the frame size to be exactly the sample size and we
	  should be OK to write it to a file.

	  However, i'm not doing that yet
	 */
	file::write_file_vector(
		"raw.wav",
		encode_codec.encode_samples_to_snippets(
			encode_state,
			&samples,
			sampling_freq,
			bit_depth,
			channel_count).at(0));
	file::write_file_vector(
		"out.wav",
		encode_codec.encode_samples_to_snippets(
			encode_state,
			&decoded_payload,
			sampling_freq,
			bit_depth,
			channel_count).at(0));
}

static void test_number_calc(){
	const std::vector<uint8_t> one =
		math::number::create(
			(uint64_t)1,
			UNIT(MATH_NUMBER_USE_NONE,
			     MATH_NUMBER_BASE_BLANK,
			     0));
	// TODO: allow for passing standard ints to the calc functions
	std::vector<uint8_t> num = 
		math::number::create(
			(uint64_t)0,
			UNIT(MATH_NUMBER_USE_NONE,
			     MATH_NUMBER_BASE_BLANK,
			     0));
	uint64_t other = 0;
	for(uint64_t i = 0;i < 1024;i++){
		num = math::number::calc::add(
			{num, one});
		other++;
		P_V(math::number::get::number(num), P_NOTE);
		P_V(other, P_NOTE);
			
	}
}

static void test_audio_sign_unsign(){
	std::vector<uint8_t> sample_vector;
	for(uint64_t i = 0;i < 1024;i++){
		int16_t signed_sample =
			(int16_t)true_rand(-32768, 32767);
		P_V(signed_sample, P_SPAM);
		sample_vector.insert(
			sample_vector.end(),
			&signed_sample,
			&((&signed_sample)[1]));
	}
	std::vector<uint8_t> new_sample_vector =
		transcode::audio::raw::unsigned_to_signed(
			transcode::audio::raw::signed_to_unsigned(
				sample_vector,
				16),
			16);
	ASSERT(new_sample_vector == sample_vector, P_ERR);
}

void test(){
	std::vector<id_t_> full_id_set =
		id_api::get_all();
	RUN_TEST(test_socket);
	RUN_TEST(test_math_number_set);
	RUN_TEST(test_id_transport);
	RUN_TEST(test_escape_string);
	RUN_TEST(test_nbo_transport);
	RUN_TEST(test_rsa_key_gen);
	RUN_TEST(test_rsa_encryption);
	RUN_TEST(test_aes);
	RUN_TEST(test_id_set_compression);
	RUN_TEST(test_net_proto_socket_transcoding);
	RUN_TEST(test_lock);
	RUN_TEST(test_number_cmp);
	RUN_TEST(test_number_calc);
	RUN_TEST(test_id_api_raw_fetch);
	test_audio_format(TV_AUDIO_FORMAT_OPUS);
	RUN_TEST(test_audio_sign_unsign);
	std::vector<id_t_> extra_id_set =
		id_api::get_all();
	for(uint64_t i = 0;i < full_id_set.size();i++){
		for(uint64_t c = 0;c < extra_id_set.size();c++){
			if(full_id_set[i] == extra_id_set[c]){
				extra_id_set.erase(
					extra_id_set.begin()+c);
				c--;
				i = 0;
			}
		}
	}
	if(extra_id_set.size() > 0){
		print("not all tests cleaned up after themselves, should fix "
		      "this on a function basis soon", P_SPAM);
	}
	for(uint64_t i = 0;i < extra_id_set.size();i++){
		id_api::destroy(extra_id_set[i]);
	}
	print("All tests passed", P_NOTE);
}
