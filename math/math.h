#ifndef MATH_H
#define MATH_H
/*
  Fancy math things I need that aren't provided by the C++ standard (in a
  friendly way) can be handled through this API. Some examples include:
  Regressions
  Hypothesis testing
  Large data collecting
  Variable precision numbers exporting to string (tv_frame_numbers)
  Generic device unit conversions
  - Pressure to altitude above sea level
  - Wattage from amperage and voltage
  - Imperial units from metric units
  - (Possible) conversions to and from log scales
 */
#endif
#include "stats/math_stats.h"
#include "numbers/math_numbers.h"

/*
  A few of these are only vectors for uniformity across calls,
  and it might make more sense to redefine it as just two
  numbers (number and base, probably uint64_t if we want to
  get more sane)
 */

#define MATH_ADD(x, y) math::number::calc::add({x, y})
#define MATH_SUB(x, y) math::number::calc::sub({x, y})
#define MATH_MUL(x, y) math::number::calc::mul({x, y})
#define MATH_DIV(x, y) math::number::calc::div({x, y})
#define MATH_POW(x, y) math::number::calc::pow({x, y})
#define MATH_LOG_BASE(x, y) math::number::calc::log_base(x)

#define MATH_NUMBER_SIGN_POSITIVE 0
#define MATH_NUMBER_SIGN_NEGATIVE 1

namespace math{
	namespace number{
		void add_data_to_set(
			std::vector<std::vector<uint8_t> > data,
			id_t_ math_number_set_id);
		namespace get{
			/*
			  PLEASE NOTE:
			  having the number be exportable to long double
			  doesn't define the upper internal limit as that
			  of a long double, but just as a simple way of 
			  exporting and representing it.

			  There really is no defined upper limit of
			  these numbers, at least not in terms of defined
			  bit lengths
			 */
			long double number(std::vector<uint8_t> data);
			uint64_t unit(std::vector<uint8_t> data);
			std::pair<std::vector<uint8_t>,
				  std::vector<uint8_t> > raw_species(
					  std::vector<uint8_t> data);
		}
		std::vector<uint8_t> create(long double number,
					    uint64_t unit);
		std::vector<uint8_t> create(uint64_t number,
					    uint64_t unit);
		std::vector<uint8_t> create(int64_t number,
					    uint64_t unit);
		/*
		  TODO: IDs, as they stand now, are the only categorical
		  statistics category I have to deal with (chi-squared).
		  There should be things added to the units to ensure that
		  numerical analysis is not done to the data
		 */
		std::vector<uint8_t> create(id_t_ number,
					    uint64_t unit);
		namespace simple{
			std::vector<uint8_t> set_sign(
				std::vector<uint8_t> data,
				uint8_t sign);
			uint8_t get_sign(
				std::vector<uint8_t> data);
			std::vector<uint8_t> flip_sign(
				std::vector<uint8_t> data);
			int8_t sign_to_int(
				uint8_t sign);
		};
		namespace calc{
			std::vector<uint8_t> add(
				std::vector<std::vector<uint8_t> > data);
			std::vector<uint8_t> sub(
				std::vector<std::vector<uint8_t> > data);
			std::vector<uint8_t> mul(
				std::vector<std::vector<uint8_t> > data);
			std::vector<uint8_t> div(
				std::vector<std::vector<uint8_t> > data);
			std::vector<uint8_t> pow(
				std::vector<std::vector<uint8_t> > data);
		};
		namespace cmp{
			bool greater_than(
				std::vector<uint8_t> x,
				std::vector<uint8_t> y);
			bool less_than(
				std::vector<uint8_t> x,
				std::vector<uint8_t> y);
			// equality DOES NOT CHECK UNITS
			bool equal_to(
				std::vector<uint8_t> x,
				std::vector<uint8_t> y);
		};
		/*
		  standardized types of ID sets used throughout the program
		 */
	};
	namespace functions{
		std::vector<uint8_t> sum_inclusive(
			id_t_ math_number_set_id,
			uint64_t interval_dim,
			uint64_t sum_dim,
			std::vector<uint8_t> start_sum,
			std::vector<uint8_t> end_sum);
	};
	/*
	  math_stat_pval_t (uint32_t) is a measure of P-value, on a scale of 0-1
	 */
	namespace stat{
		namespace set{
			/*
			  Ideally all sample sets collected internally in
			  BasicTV can be referenced in this sort of way
			 */
			namespace add{
				void throughput_number_set(
					id_t_ set_id,
					uint64_t volume,
					uint64_t timestamp_micro_s);
			};
			namespace reduce{
				
			};
			// actually deleting information from number sets is
			// based more on upper memory limits, and the
			// definition of deleting itself is fairly hairy,
			// since we can perform some simplification functions
			// depending on the type of data in play
			// 'reduction' is the name of these sorts of
			// simplifications and transformations, where each type
			// of set has it's own optimal transformation for
			// optimal size
		};
		namespace vars{
			std::vector<uint8_t> mean(
				std::vector<std::vector<uint8_t> > data);
			std::vector<uint8_t> q1(
				std::vector<std::vector<uint8_t> > data);
			std::vector<uint8_t> q2(
				std::vector<std::vector<uint8_t> > data);
			std::vector<uint8_t> q3(
				std::vector<std::vector<uint8_t> > data);
			std::vector<uint8_t> min(
				std::vector<std::vector<uint8_t> > data);
			std::vector<uint8_t> max(
				std::vector<std::vector<uint8_t> > data);
			std::vector<uint8_t> std_dev(
				std::vector<std::vector<uint8_t> > data);
		};
		// direct access to density curves
		namespace dist{
			math_stat_pval_t z(
				std::vector<uint8_t> start,
				std::vector<uint8_t> end);
			math_stat_pval_t t(
				std::vector<uint8_t> start,
				std::vector<uint8_t> end,
				std::vector<uint8_t> df);
			math_stat_pval_t chi_squared(
				std::vector<uint8_t> start,
				std::vector<uint8_t> end,
				std::vector<uint8_t> df);
			math_stat_pval_t f(
				std::vector<uint8_t> start,
				std::vector<uint8_t> end,
				std::vector<uint8_t> df);
			math_stat_pval_t binom(
				std::vector<uint8_t> start,
				std::vector<uint8_t> end,
				std::vector<uint8_t> df);
			math_stat_pval_t geo(
				std::vector<uint8_t> start,
				std::vector<uint8_t> end,
				std::vector<uint8_t> df);
			math_stat_pval_t hypergeo(
				std::vector<uint8_t> start,
				std::vector<uint8_t> end,
				std::vector<uint8_t> df);
		};
		namespace test{
			// returns P-values
			namespace z{
				math_stat_pval_t one_sample(
					std::vector<uint8_t> mean,
					std::vector<uint8_t> std_dev,
					std::vector<uint8_t> value,
					uint8_t cmp);
				math_stat_pval_t two_sample(
					std::vector<uint8_t> mean_1,
					std::vector<uint8_t> std_dev_1,
					std::vector<uint8_t> mean_2,
					std::vector<uint8_t> std_dev_2,
					uint8_t cmp);
			};
			namespace t{
				math_stat_pval_t one_sample(
					std::vector<uint8_t> mean,
					std::vector<uint8_t> std_dev,
					std::vector<uint8_t> value,
					uint8_t cmp);
				math_stat_pval_t two_sample(
					std::vector<uint8_t> mean_1,
					std::vector<uint8_t> std_dev_1,
					std::vector<uint8_t> mean_2,
					std::vector<uint8_t> std_dev_2,
					uint8_t cmp);
			};
			namespace chi_squared{
				math_stat_pval_t gof(
				        std::vector<std::vector<uint8_t> > obs,
					std::vector<std::vector<uint8_t> > exp);
				math_stat_pval_t two_way(
					std::vector<std::vector<std::vector<uint8_t> > > data);
			};
			namespace f{
				math_stat_pval_t anova(
					std::vector<std::vector<std::vector<uint8_t> > > data);
			};
		};
	};
};
