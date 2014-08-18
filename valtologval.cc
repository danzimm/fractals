
#include "fractal.hpp"

static long double parameter = 0.0f;

std::complex<long double> valtologval_iterator(std::complex<long double> value, std::complex<long double> initial, uint64_t i) {
  return std::pow(value, std::log(value));
}

extern "C" {
  
  void *value_iterator(long double param) {
    parameter = param;
    return (void *)valtologval_iterator;
  }

}

