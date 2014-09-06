
#include "fractal.hpp"

static long double parameter = 0.0f;

std::complex<long double> valtoval_iterator(std::complex<long double> value, std::complex<long double> initial, uint64_t i) {
  return std::pow(value, value) + initial;
}

extern "C" {
  
  void *value_iterator(long double parama, long double paramb) {
    parameter = parama;
    return (void *)valtoval_iterator;
  }

}

