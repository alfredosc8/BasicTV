#ifndef TV_FRAME_DATA_H
#define TV_FRAME_DATA_H
#include "../main.h"
#include "../util.h"

#include <climits>

/*
  I'm working with some friends to make a space balloon with tons of sensors
  on it, and I had the idea of broadcasting all of these raw numbers onto
  the network, running some statistics on the data, and having custom
  visualizations of these numbers as a part of the tv_window_t's internal
  switching of channels (i.e. I create an unbroadcasted channel which hosts
  the visualizations).

  Because of the lack of bandwidth in space, i'm designating a Raspberry Pi
  to handling receiving the data from the packet radio, connecting that software
  to BasicTV somehow (through some numbers interface), and handling all of that
  here.

  Since the numbers themselves are possibly the most compact chunks of data
  relative to their refresh rate, numbers can be nested inside of number sets,
  decreasing the computing, networking, and storage overheads for the data.
  
  Numbers broadcasted on the network don't NEED units to work fine, but units
  can help greatly, as some units can be derived from others (altitude 
  from pressure, wattage from ampres and volts, speed from position over time,
  etc), not to mentiont that external APIs can be used with the data for better
  representation (mostly Google Earth and GPS coordinates).If no units are
  provided, you can just put the units in the title. Regressions and stats can
  be ran on unitless data.
 */

/*
  I want to define units as math formulas so I can get more complicated
  math working, as well as better representation of existing data.

  Basic modifications of order of operations for simplicity:
  Everything after DIVIDED is under (3/1*3 == 1, not 9)
  All units are simplified to a standardized format


  Four bit operator after four-bit unit
  METER DIVIDED SECOND is acceleration
  METER DIVIDED SECOND TIMES SECOND

  A 64-bit number (largest generally supported native) can hold up to 8
  different units, which looks to be beyond the length and complexity of
  most units.

  First byte is just a table of non-unit uses of the number (GPS, mainly)

  Any number, for any reason, has an upper limit of 128-bits
*/

// base SI unit or complex SI unit, doesn't matter
#define TV_FRAME_NUMBER_USE_SI 1
// use as a GPS coordinate (major and minor change value)
#define TV_FRAME_NUMBER_USE_COORD 2
// no formal units
#define TV_FRAME_NUMBER_USE_NONE 3

#define TV_FRAME_NUMBER_OPERATOR_BLANK 0
#define TV_FRAME_NUMBER_OPERATOR_MULTIPLY 1
#define TV_FRAME_NUMBER_OPERATOR_DIVIDE 2
#define TV_FRAME_NUMBER_OPERATOR_EXPONENT 3
#define TV_FRAME_NUMBER_OPERATOR_NEGATE 4

// SI base units, fit nicely into 3-bits

#define TV_FRAME_NUMBER_BASE_BLANK 0
#define TV_FRAME_NUMBER_BASE_METER 1
#define TV_FRAME_NUMBER_BASE_KILOGRAM 2
#define TV_FRAME_NUMBER_BASE_SECOND 3
#define TV_FRAME_NUMBER_BASE_AMPERE 4
#define TV_FRAME_NUMBER_BASE_KELVIN 5
#define TV_FRAME_NUMBER_BASE_MOLE 6
#define TV_FRAME_NUMBER_BASE_CANDELA 7

namespace number_api{
	namespace get{
		uint16_t device(std::vector<uint8_t> data);
		long double number(std::vector<uint8_t> data);
		uint64_t unit(std::vector<uint8_t> data);
		uint64_t timestamp(std::vector<uint8_t> data);
	}
	std::vector<uint8_t> create(uint16_t device,
				    long double number,
				    uint64_t unit,
				    uint64_t timestamp);
};

struct tv_frame_number_device_t{
private:
	std::vector<std::vector<uint8_t> > raw_number_data;
public:
	data_id_t id;
	tv_frame_number_device_t();
	~tv_frame_number_device_t();
	void add_raw_data(
		std::vector<uint8_t> data);
	std::vector<std::vector<uint8_t> > get_raw_data();
};

#endif
