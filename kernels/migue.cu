
#include <stdint.h>
#include "complex.h"

extern "C" {

__device__
double2 iterate_value(unsigned long i, double mag, double2 val, double2 coord) {
  return complex_pow(val, (double2){8.8, 0.0}) + coord;
}

}

