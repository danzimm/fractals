
#include <stdint.h>

#include "complex.h"

extern "C" {

__device__
double2 iterate_value(unsigned long i, double mag, double2 val, double2 coord) {
  //double2 c = {-0.8, 0.156};
  double2 c = {-0.4, 0.6};
  return complex_pown(val, 2) + c;
}

}

