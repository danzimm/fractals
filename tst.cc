
#include "fractal.hpp"

static long double parameter = 0.0f;

std::complex<long double> tst_iterator(std::complex<long double> value, std::complex<long double> initial, uint64_t i) {
  return std::pow(value, 3.0) + value * value + value + initial;
}

extern "C" {
  
  void *value_iterator(long double param) {
    parameter = param;
    return (void *)tst_iterator;
  }

}

