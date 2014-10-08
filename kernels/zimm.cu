
#include <stdint.h>
#include "complex.h"

extern "C" {

__device__
double2 iterate_value(unsigned long i, double mag, double2 val, double2 coord) {
  //return complex_pow(val, (double2){3.7, 0.0}) - complex_pow(val, (double2)(1.5, 0.0)) + coord;
  return complex_pow(val, (double2){2.1, 0.0}) - complex_pow(val, (double2){1.7, 0.0}) + coord;
}

}

