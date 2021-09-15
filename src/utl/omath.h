/*
 * correlation.h
 *
 *  Created on: Jan 15, 2021
 *      Author: kaveh
 */

#ifndef SRC_UTL_OMATH_H_
#define SRC_UTL_OMATH_H_

#include <stdint.h>
#include <math.h>
#include <assert.h>
#include <iostream>

namespace utl {
namespace math {

// online correlation
struct corr_t {
private:
	double sumx = 0;
	double sumy = 0;
	double sumxsq = 0;
	double sumysq = 0;
	double sumxy = 0;
	uint64_t N = 0;
	double corr = 0;

public:
	double get_corr() {
		corr = (N * sumxy - sumx * sumy);
		//printf("%f %f %f %f %f %f %f\n", N, sumh, sumt, sumhsq,
				//sumtsq, sumht, sqrt(sumhsq - N * sumh*sumh) * sqrt(sumtsq - N * sumt*sumt));

		double covar = (sqrt(N*sumxsq - sumx*sumx) * sqrt(N*sumysq - sumy*sumy));
		if (covar != 0 && !std::isnan(covar))
			corr /= covar;
		else
			corr = 0;
		assert(!std::isnan(corr));
		return corr;
	}

	void add_point(double x, double y) {
		N += 1;
		sumx += x;
		sumy += y;
		sumxsq += x*x;
		sumysq += y*y;
		sumxy += x*y;
	}
};

// Welford online mean/variance calculator
struct avg_t {
private:
	double mean = 0;
	uint64_t count = 0;
	double delta = 0;

public:
	void add_point(double x) {
	    count += 1;
	    delta = x - mean;
	    mean += delta / count;
	}

	double get_mean() { return mean; }
};


// Welford online mean/variance calculator
struct avgstdev_t {
private:
	double mean = 0;
	double m2 = 0;
	uint64_t count = 0;
	double delta = 0;
	double delta2 = 0;
	double sigma2 = 0;

public:
	void add_point(double x) {
	    count += 1;
	    delta = x - mean;
	    mean += delta / count;
	    delta2 = x - mean;
	    m2 += delta * delta2;
	}

	double get_mean() { return mean; }
	double get_sigma2() { sigma2 = m2 / count; return sigma2; }
};

}
}

#endif /* SRC_UTL_OMATH_H_ */
