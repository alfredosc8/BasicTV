#ifndef TV_FRAME_NUMBERS_H
#define TV_FRAME_NUMBERS_H
/*
  Number/raw streams of data. Attach directly to sensors to get live
  regressions and visualizations of things.
*/

struct tv_frame_number_set_t{
private:
	/*	
	  This should not be stored as math_numbers will read it,
	  there will be a 64-bit timestamp and a 16-bit device
	  ID prepended to it

	  The device ID is just a random number to differentiate between
	  different physical objects (or abstract concepts), which allows
	  for easier conversion to and from alternative/imperial units
	  and derived units (km/s to mph, volts and amps to watts).

	  The device ID can be the same across numbers so long as all
	  units are unique (as they ought to be). If there is a case where
	  one device needs two of the same unit, it should be subdivided
	  into two seperate devices.
	 */
	std::vector<uint8_t> set;
public:
	data_id_t id;
	tv_frame_number_set_t();
	~tv_frame_number_set_t();
};

#endif
