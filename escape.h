#ifndef ESCAPE_H
#define ESCAPE_H
#include "tuple"
#include "vector"
#include "util.h"

/*
  This is a generalized escape system, pretty simple.

  If no prefix for the size is found, an exception is thrown.
 */

std::vector<uint8_t> escape_vector(
	std::vector<uint8_t> vector,
	uint8_t escape_char);
std::pair<std::vector<uint8_t>, std::vector<uint8_t> > unescape_vector(
	std::vector<uint8_t> vector,
	uint8_t escape_char);
std::pair<std::vector<std::vector<uint8_t> >, std::vector<uint8_t> > unescape_all_vectors(
	std::vector<uint8_t> vector,
	uint8_t escape_char);
#endif
