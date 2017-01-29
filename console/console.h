#ifndef CONSOLE_H
#define CONSOLE_H

#include "../util.h"
#include "../convert.h"
#include "../net/net_socket.h"
#include "../id/id.h"
#include "../id/id_api.h"
#include <string>
#include <vector>
#include <algorithm>

/*
  Generic interface to strip the reference to the function being called and
  calling it. This is done for every level.
*/

#define CONSOLE_REGISTER_SIZE 64

#define LIST_CMD(x) if(cmd_vector.at(0) == #x){if(cmd_vector.size() > 1){x(std::vector<std::string>(cmd_vector.begin()+1, cmd_vector.end()));}else{x(std::vector<std::string>{});}ran = true;}
#define DEF_CMD(x) void x(std::vector<std::string> cmd_vector)
#define DEC_CMD(x) void console_t::x(std::vector<std::string> cmd_vector)

/*
  console_t: console interface for BasicTV

  This console is a load-store architecture. 

  All output is directed to output_table. Registers can be modified as such:
  reg_set_const: set it as a constant
  reg_set_table: set it from the x and y coordinates from the output table
  reg_copy: copy from one register to another
  reg_clear: clear all registers

  When IDs are manipulated in the output table and registers, they are ALWAYS
  stored in the standard 81 hex char format

  All calls to commands are encapsulated in try catch blocks (execute())
*/

struct console_t{
private:
	id_t_ socket_id = ID_BLANK_ID;
	std::vector<uint8_t> working_input; // buffer
	std::vector<std::vector<std::string> > output_table;
	std::array<std::string, 64> registers;

	DEF_CMD(exit);
	DEF_CMD(output_table_clear);
	
	/*
	  Print operations
	 */
	DEF_CMD(print_output_table);
	DEF_CMD(print_reg);
	DEF_CMD(print_reg_with_type);
	/*
	  Register operators: Set the IDs in the registers

	  This is a load-store architecture, so these are the only functions
	  that take parameters in from the command line

	  Defined in console_reg.cpp
	*/
	DEF_CMD(reg_set_const); // first: reg number, second: const
	DEF_CMD(reg_set_table); // first: reg number, second: output table X, third: output table Y
	DEF_CMD(reg_copy); // first: from, second: to
	DEF_CMD(reg_swap);
	DEF_CMD(reg_get_type);
	DEF_CMD(reg_clear); // no param
	DEF_CMD(reg_left_shift);
	DEF_CMD(reg_right_shift);
	DEF_CMD(reg_print_type);
	/* /\* */
	/*   ID interface */

	/*   IDs are ALWAYS stored in the 80 char format in the registers */

	/*   Defined in console_id_api.cpp */
	/*  *\/ */
	/* // API */
	DEF_CMD(id_api_get_type_cache); // first reg: type
	DEF_CMD(id_api_get_all);
	/* DEF_CMD(id_api_destroy); // first reg: id */
	/* DEF_CMD(id_api_request); // first reg: id */
	/* // linked list operations */
	/* DEF_CMD(id_linked_list_forward); // first reg: id */
	/* DEF_CMD(id_linked_list_backwards); // first reg: id */

	/* /\* */
	/*   Data creation interface */

	/*   Defined in console_data.cpp */
	/*  *\/ */
	/* DEF_CMD(data_get_production_priv_key_id); // no reg */
	/* DEF_CMD(data_set_production_priv_key_id); // first reg: id */
	
	/* /\* */
	/*   TV viewing interface */

	/*   Defined in console_tv.cpp */
	/*  *\/ */
	/* DEF_CMD(tv_list_all_active_streams); // no reg */
	/* DEF_CMD(tv_list_clear_all_active_streams); // no reg */
	/* DEF_CMD(tv_list_del_active_streams); // first reg: id to del */
	
	DEF_CMD(tv_window_list_active_streams); // first reg: tv_window_t id
	DEF_CMD(tv_window_clear_active_streams); // first reg: tv_window_t id
	DEF_CMD(tv_window_add_active_stream); // first reg: chan id, second reg: stream id
	DEF_CMD(tv_window_del_active_stream); // first reg: chan id, second reg: stream id/
	DEF_CMD(tv_window_create);
	DEF_CMD(tv_window_set_to_time);
	DEF_CMD(tv_window_set_timestamp_offset);
	DEF_CMD(tv_window_set_channel_id);
	DEF_CMD(tv_channel_create);
	DEF_CMD(tv_channel_get_stream_list);
	DEF_CMD(tv_audio_load_wav);
	void print_socket(std::string);
	void execute(std::vector<std::string> cmd_vector);
public:
	data_id_t id;
	console_t();
	~console_t();
	void set_socket_id(id_t_ socket_id_);
	id_t_ get_socket_id();
	void run();
};

// easier to guarantee typenames

#define ADD_COLUMN_TO_TABLE(vector, id_column, getter, converter, type) \
	vector[0].push_back(#getter);					\
	for(uint64_t i = 0;i < vector.size();i++){			\
		id_t_ tmp_id =						\
			convert::array::id::from_hex(			\
				vector[i][id_column]);			\
		type *tmp_ptr =						\
			PTR_DATA(tmp_id, type);				\
		if(tmp_ptr != nullptr){					\
			vector[i].push_back(converter(tmp_ptr->getter())); \
		}else{							\
			vector[i].push_back("NULL");			\
		}							\
	}								\
	
extern std::vector<std::vector<std::string> > console_generate_generic_id_table(std::vector<id_t_> id_vector);

extern void console_init();
extern void console_loop();
extern void console_close();

#endif
