
#include <stdint.h>

#include "complex.cu"

extern "C" {

__device__
void processPixel(unsigned long *ii, double *magg, unsigned long maxiter, double escape, double2 coord) {
  double2 val = coord;
  double mag = 0.0f;
  unsigned long i = 0;
  while (i < maxiter) {
    val = (val - complex_divide(complex_pown(val, 3) - (double2){1.0, 0.0}, 3 * complex_pown(val, 2)));
    mag = complex_mag2(val);
    if (mag >= escape) {
      break;
    }
    i++;
  }
  *ii = i;
  *magg = mag;
}

}

