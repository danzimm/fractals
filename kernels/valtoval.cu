
#include <stdint.h>

#include "complex.cu"

extern "C" {

__device__
void processPixel(unsigned long *ii, double *magg, unsigned long maxiter, double escape, double2 coord) {
  double2 val = coord;
  double mag = 0.0f;
  unsigned long i = 0;
  while (i < maxiter) {
    val = complex_pow(val, val) + coord;
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

