
#include <stdint.h>

#include "complex.h"

extern "C" {

__device__
double2 iterate_value(unsigned long i, double mag, double2 val, double2 coord) {
  return (val - complex_divide(complex_pown(val, 3) - (double2){1.0, 0.0}, 3 * complex_pown(val, 2)));
}

}

