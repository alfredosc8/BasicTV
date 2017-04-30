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

#include "test.h"

/*
  Run as many tests as possible (that make sense)
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
	if(std::equal(input_data.begin(), input_data.end(), output_data.begin())){
		print("Compressor works", P_NOTE);
	}else{
		print("input != output", P_ERR);
	}
}

static void test_socket(){
	/*
	  I cannot locally connect to this computer without using another IP
	  address (breaking 4-tuple), so just test this with laptop
	 */
	net_socket_t *test_socket_ = new net_socket_t;
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

static void test_socket_array(std::vector<std::pair<id_t_, id_t_> > socket_array){
	for(uint64_t i = 0;i < socket_array.size();i++){
		net_socket_t *first =
			PTR_DATA(socket_array[i].first,
				 net_socket_t);
		net_socket_t *second =
			PTR_DATA(socket_array[i].second,
				 net_socket_t);
		if(first == nullptr || second == nullptr){
			P_V_S(convert::array::id::to_hex(socket_array[i].first), P_SPAM);
			P_V_S(convert::array::id::to_hex(socket_array[i].second), P_SPAM);
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
  This works up until 537 (stack smashing), and I can't find the problem. If
  you are stuck at a lower number, make sure you set the file descriptor limit
  high enough (ulimit -n 65536 works for me).
*/

static void test_max_tcp_sockets(){
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
		test_socket_array(socket_pair);
	}
}

static void test_id_transport_print_exp(std::vector<uint8_t> exp){
	P_V(exp.size(), P_SPAM);
	for(uint64_t i = 0;i < exp.size();i++){
		print(std::to_string(i) + "\t" + std::to_string((int)(exp[i])) + "\t" + std::string(1, exp[i]), P_SPAM);
	}
}

/*
  TODO: rework this to use different types
 */

static void test_id_transport(){
	// not defined behavior at all
	settings::set_setting("export_data", "true");
	net_proto_peer_t *tmp =
		new net_proto_peer_t;
	tmp->set_net_ip("127.0.0.1", 58486);
	const std::vector<uint8_t> exp =
		tmp->id.export_data(ID_DATA_NOEXP | ID_DATA_NONET,
				    ID_EXTRA_COMPRESS | ID_EXTRA_ENCRYPT);
	//test_id_transport_print_exp(exp);
	net_proto_peer_t *tmp_2 =
		new net_proto_peer_t;
	tmp_2->id.import_data(exp);
	P_V_S(convert::array::id::to_hex(tmp->id.get_id()), P_NOTE);
	P_V_S(convert::array::id::to_hex(tmp_2->id.get_id()), P_NOTE);
	P_V(tmp_2->get_net_port(), P_NOTE);
	P_V_S(tmp_2->get_net_ip_str(), P_NOTE);
	running = false;
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
	if(test == test_2){
		print("it works", P_NOTE);
	}else{
		for(uint64_t i = 0;i < test.size();i++){
			P_V_C(test[i], P_NOTE);
			P_V_C(test_2[i], P_NOTE);
		}
		print("it doesn't work", P_ERR);
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
	if(priv == nullptr){
		print("priv key is a nullptr", P_ERR);
	}
	/*
	  First is a macro for the encryption type
	  Second is the DER formatted vector
	 */
	P_V(priv->get_encrypt_key().second.size(), P_NOTE);
	encrypt_pub_key_t *pub =
		PTR_DATA(rsa_key_pair.second,
			 encrypt_pub_key_t);
	if(pub == nullptr){
		print("pub key is a nullptr", P_ERR);
	}
	P_V(pub->get_encrypt_key().second.size(), P_NOTE);
}

static void test_rsa_encryption(){
	print("using an RSA key length of 4096", P_NOTE);
	uint64_t key_len = 4096;
	std::vector<uint8_t> test_data;
	std::pair<id_t_, id_t_> rsa_key_pair =
		rsa::gen_key_pair(key_len);
	print("generating RSA test data", P_NOTE);
	for(uint64_t x = 0;x < 1024;x++){
		test_data.push_back(
			(uint8_t)true_rand(0, 255));
	}
	print("encrypting RSA test data", P_NOTE);
	std::vector<uint8_t> test_data_output =
		encrypt_api::encrypt(
			test_data,
			rsa_key_pair.first);
	print("decrypting RSA test data", P_NOTE);
	test_data_output =
		encrypt_api::decrypt(
			test_data_output,
			rsa_key_pair.second);
	if(test_data == test_data_output){
		print("it worked with " + std::to_string((long double)test_data.size()/(1024.0*1024.0)) + " MB", P_NOTE);
	}else{
		print("FAILED", P_ERR);
	}
}

static void test_aes(){
	std::vector<uint8_t> key = {'f','f','f','f','f','f','f','f','f','f','f','f','f','f','f','f','f','f','f','f','f','f','f','f'};
	std::vector<uint8_t> data = {'T', 'E', 'S', 'T', 'I', 'N', 'G'};
	if(aes::decrypt(
		   aes::encrypt(
			   data, key),
		   key) == data){
		print("it works", P_NOTE);
	}else{
		print("it does not work", P_CRIT);
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
	data_id_t *priv_key = PTR_ID(rsa_key_pair.first, );
	if(priv_key != nullptr){
		priv_key->noexp_all_data();
	}
	data_id_t *pub_key = PTR_ID(rsa_key_pair.second, );
	if(pub_key != nullptr){
		pub_key->noexp_all_data();
	}
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
		P_V(size_bytes/(1024*1024), P_NOTE);
		P_V_S(get_readable_time(datum.first), P_NOTE);
		P_V_S(get_readable_time(datum.second), P_NOTE);
		out << size_bytes << " " << datum.first << " " << datum.second << std::endl;
		print(std::to_string(benchmark_data.size()-i-1) + " left to go", P_NOTE);
	}
	out.close();
	print("benchmark completed", P_NOTE);
}

static void test_escape_string(){
	const char escape_char = 0xFF;
	std::vector<uint8_t> all_escaped_stuff;
	for(uint64_t i = 0;i < 512;i++){
		std::vector<uint8_t> tmp =
			escape_vector(
				true_rand_byte_vector(
					8192),
				escape_char);
		all_escaped_stuff.insert(
			all_escaped_stuff.end(),
			tmp.begin(),
			tmp.end());
	}
	P_V(all_escaped_stuff.size(), P_NOTE);
	std::pair<std::vector<std::vector<uint8_t> >, std::vector<uint8_t> > deconstructed =
		unescape_all_vectors(
			all_escaped_stuff,
			escape_char);
	P_V(deconstructed.first.size(), P_NOTE);
	P_V(deconstructed.second.size(), P_NOTE);
}

static void test_id_set_compression(){
	std::vector<id_t_> id_set;
	std::array<uint8_t, 32> hash;
	for(uint64_t i = 0;i < 256;i++){
		// i is the UUID
		if(true_rand(0, 30) == 0){
			print("computing new hash at iteration " + std::to_string(i), P_NOTE);
			hash = encrypt_api::hash::sha256::gen_raw(
				true_rand_byte_vector(64));
		}
		id_t_ tmp_id;
		set_id_uuid(&tmp_id, i);
		set_id_hash(&tmp_id, hash);
		id_set.push_back(tmp_id);
	}
	std::vector<uint8_t> id_set_compact =
		compact_id_set(id_set);
	std::vector<id_t_> id_set_new =
		expand_id_set(id_set_compact);
	if(id_set_new == id_set){
		print("successfully decompressed, checks out", P_NOTE);
		const long double compression_ratio =
			(id_set.size()*sizeof(id_t_))/(id_set_compact.size());
		P_V(compression_ratio, P_NOTE);
	}else{
		print("ERROR, COULDN'T PROPERLY DECOMPRESS", P_NOTE);
		P_V(id_set.size(), P_NOTE);
		P_V(id_set_new.size(), P_NOTE);
		// for(uint64_t i = 0;i < (id_set.size() > id_set_new.size()) ? id_set.size() : id_set_new.size();i++){
		// 	if(id_set.size() > i){
		// 		P_V_S(convert::array::id::to_hex(id_set[i]), P_NOTE);
		// 	}
		// 	if(id_set_new.size() > i){
		// 		P_V_S(convert::array::id::to_hex(id_set_new[i]), P_NOTE);
		// 	}
		// }
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

void test(){
	test_escape_string();
	//test_max_tcp_sockets();
	test_id_transport();
	test_nbo_transport();
	test_rsa_key_gen();
	test_rsa_encryption();
	test_aes();
	test_id_set_compression();
}

