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
	wallet_set_ptr->id.noexp_all_data();
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

static void test_nc_socket(){
	/*
	  I cannot locally connect to this computer without using another IP
	  address (breaking 4-tuple), so just test this with laptop
	 */
	net_socket_t *test_socket_ = new net_socket_t;
	test_socket_->id.noexp_all_data();
	std::string ip;
	uint16_t port = 0;
	bool recv = false;
	try{
		recv = settings::get_setting(
			"test_recv") == "1";
	}catch(...){}
	if(recv){
		while(true){
			net_proto_loop();
		}
	}else{
		print("IP address to test", P_NOTE);
		std::cin >> ip;
		print("Port to test", P_NOTE);
		std::cin >> port;
		std::pair<std::string, uint16_t> laptop_conn =
			std::make_pair(ip, port);
		test_socket_->set_net_ip(ip, port);
		test_socket_->connect();
		test_socket_->send("AAAA");
		while(true){
			sleep_ms(1);
		}
	}
	id_api::destroy(test_socket_->id.get_id());
	test_socket_ = nullptr;
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
	std::vector<uint8_t> input_data;
	for(uint64_t i = 0;i < 1024*1024;i++){
		input_data.push_back(i&0xFF);
	}
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
	print("Local IP address:", P_NOTE);
	std::string ip;
	std::cin >> ip;
	std::vector<std::pair<id_t_, id_t_> > socket_pair;
	bool dropped = false;
	net_socket_t *inbound =
		new net_socket_t;
	inbound->set_net_ip("", 50000);
	inbound->connect();
	while(!dropped){
		for(uint64_t i = 0;i < 128;i++){
			net_socket_t *first =
				new net_socket_t;
			first->set_net_ip(ip, 50000);
			first->connect();
			net_socket_t *second =
				new net_socket_t;
			sleep_ms(1); // probably isn't needed
			TCPsocket tmp_socket =
				SDLNet_TCP_Accept(inbound->get_tcp_socket());
			if(tmp_socket != nullptr){
				second->set_tcp_socket(tmp_socket);
			}else{
				print("unable to receive connection request", P_ERR);
			}
			socket_pair.push_back(
				std::make_pair(
					first->id.get_id(),
					second->id.get_id()));
		}
		test_nc_socket_array(socket_pair);
	}
	id_api::destroy(inbound->id.get_id());
	for(uint64_t i = 0;i < socket_pair.size();i++){
		id_api::destroy(socket_pair[i].first);
		id_api::destroy(socket_pair[i].second);
	}
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
	wallet_set_ptr->id.noexp_all_data();
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
			ID_DATA_NOEXP,
			ID_EXTRA_COMPRESS | ID_EXTRA_ENCRYPT);
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
			// P_V_C(test[i], P_WARN);
			// P_V_C(test_2[i], P_WARN);
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
		std::vector<uint8_t> tmp;
		for(uint64_t i = 0;i < 1024;i++){
			tmp.push_back(true_rand(0, 255));
		}
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
	ID_NOEXP_NONET(rsa_key_pair.first);
	ID_NOEXP_NONET(rsa_key_pair.second);
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
	std::vector<uint8_t> test_data;
	std::pair<id_t_, id_t_> rsa_key_pair =
		rsa::gen_key_pair(key_len);
	ID_NOEXP_NONET(rsa_key_pair.first);
	ID_NOEXP_NONET(rsa_key_pair.second);
	for(uint64_t x = 0;x < 1024;x++){
		test_data.push_back(
			(uint8_t)true_rand(0, 255));
	}
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
	for(uint64_t i = 1;i <=1024;i++){
		benchmark_data.push_back(i);
	}
	std::pair<id_t_, id_t_> rsa_key_pair =
		rsa::gen_key_pair(4096); // TODO: modify encrypt API to not assume this
	ID_NOEXP_NONET(rsa_key_pair.first);
	ID_NOEXP_NONET(rsa_key_pair.second);
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
			ID_DATA_NOEXP, 0);
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
		const long double compression_ratio =
			(id_set.size()*sizeof(id_t_))/(id_set_compact.size());
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

#define TV_NUMBER_CHECK_VALUE(x) if(math::number::get::x(device_sensor) != x){print((std::string)#x + " doesn't match", P_WARN);P_V(math::number::get::x(device_sensor), P_WARN);P_V(x, P_WARN);}

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
	while(intermediate_socket->get_tcp_socket() == nullptr){
		port++; // 60000 is the typical
		intermediate_socket->set_net_ip("", port); // blank means accepts incoming
		intermediate_socket->connect();
	}
	std::vector<std::pair<net_proto_socket_t *, net_socket_t *> > socket_vector =
		{std::make_pair(new net_proto_socket_t,
				new net_socket_t),
		 std::make_pair(new net_proto_socket_t,
				new net_socket_t)};
	socket_vector[0].first->id.noexp_all_data();
	socket_vector[1].first->id.noexp_all_data();

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
	socket_vector[1].first->set_socket_id(
		socket_vector[1].second->id.get_id());
	// 0 attempts a connect, intermediate_socket accepts, shifts ownership
	// to 1, and allows for transmission of data over the proto_socket
	socket_vector[0].second->set_net_ip("127.0.0.1", port);
	socket_vector[0].second->connect();
	// accept incoming
	TCPsocket new_socket = nullptr;
	while((new_socket = SDLNet_TCP_Accept(intermediate_socket->get_tcp_socket())) == nullptr){
		sleep_ms(1);
	}
	socket_vector[1].second->set_tcp_socket(new_socket);
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

void test_nc(){
	test_nc_socket();
}

/*
  Ran with --run_tests (enabled by default)
 */

/*
  Might be able to multithread (?)
 */

#define RUN_TEST(test) try{test();print("TEST " + fix_to_length(((std::string)#test), 40) + " OK", P_NOTE);}catch(...){print((std::string)(#test) + " FAIL", P_ERR);throw std::runtime_error("failed test");}

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

void test(){
	std::vector<id_t_> full_id_set =
		id_api::get_all();
	RUN_TEST(test_math_number_set);
	RUN_TEST(test_escape_string);
	RUN_TEST(test_id_transport);
	RUN_TEST(test_nbo_transport);
	RUN_TEST(test_rsa_key_gen);
	RUN_TEST(test_rsa_encryption);
	RUN_TEST(test_aes);
	RUN_TEST(test_id_set_compression);
	RUN_TEST(test_net_proto_socket_transcoding);
	RUN_TEST(test_lock);
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
