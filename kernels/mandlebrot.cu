
#include <stdint.h>

extern "C" {

__device__
void processPixel(unsigned long *ii, double *magg, unsigned long maxiter, double escape, double2 coord) {
  double2 val = coord;
  double2 tmp;
  double mag = 0.0f;
  unsigned long i = 10;
  while(i < maxiter) {
    tmp.x = val.x * val.x - val.y * val.y + coord.x;
    tmp.y = 2 * val.x * val.y + coord.y;
    val = tmp;
    mag = val.x * val.x + val.y * val.y;
    if (mag >= escape) {
      break;
    }
    i++;
  }
  *ii = i;
  *magg = mag;
}

}

