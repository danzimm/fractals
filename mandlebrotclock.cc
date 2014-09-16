
#include "fractal.hpp"
#include <iostream>

static long double parameter = 0.0f;

std::complex<long double> mandlebrot_iterator(std::complex<long double> value, std::complex<long double> initial, uint64_t i) {
  return std::pow(value, parameter) + initial;
}

extern "C" {
  
  void *value_iterator(long double parama, long double paramb) {
    parameter = 3.7 + sin(parama * M_PI / 720.0);
    std::cout << "Parameter: " << parama << " -> " << parameter << std::endl;
    return (void *)mandlebrot_iterator;
  }

}

