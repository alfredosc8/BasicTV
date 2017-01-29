#include "console.h"

DEC_CMD(id_api_get_type_cache){
	output_table =
		console_generate_generic_id_table(
			id_api::cache::get(
				registers.at(0)));
}
