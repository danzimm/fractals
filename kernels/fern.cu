
#include "complex.h"

extern "C" {

__device__
double2 fetch_initial_point(unsigned long i) {
  return (double2){0.0, 0.0};
}

__device__
double2 iterate_point(double2 val, unsigned long i, double2 ipnt, unsigned long func_n) {
  if (func_n < 42949673) {
    func_n = 0;
  } else if (func_n < 3693671875) {
    func_n = 1;
  } else if (func_n < 3994319586) {
    func_n = 2;
  } else {
    func_n = 3;
  }

  switch (func_n) {
    case 0:
      return (double2){0.0, 0.16 * val.y};
    case 1:
      return (double2){0.85 * val.x + 0.04 * val.y, -0.04 * val.x + 0.85 * val.y + 1.6};
    case 2:
      return (double2){0.2 * val.x - 0.26 * val.y, 0.23 * val.x + 0.22 * val.y + 1.6};
    case 3:
      return (double2){-0.15 * val.x + 0.28 * val.y, 0.26 * val.x + 0.24 * val.y + 0.44};
  }
  return val;
}

}

