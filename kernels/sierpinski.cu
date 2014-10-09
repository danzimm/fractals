
#include "complex.h"

extern "C" {

__device__
double2 fetch_initial_point(unsigned long i) {
  i = i % 5;
  switch(i) {
    case 0:
      return (double2){0.0,0.0};
    case 1:
      return (double2){1.0, 0.0};
    case 2:
      return (double2){0.0, 1.0};
    case 3:
      return (double2){1.0, 1.0};
    case 4:
      return (double2){0.5, 0.5};
  }
  return (double2){0.0, 0.0};
}

__device__
double2 iterate_point(double2 val, unsigned long i, double2 ipnt, unsigned long func_n) {
  func_n = func_n % 3;
  switch (func_n) {
    case 0:
      return 0.5 * val;
    case 1:
      return 0.5 * val + (double2){0.5, 0.0};
    case 2:
      return 0.5 * val + (double2){0.0, 0.5};
  }
  return val;
}

}

