
#include "complex.h"

extern "C" {

__device__
unsigned npnts = 3;

__device__
double2 pnts[] = {
  {0.0, 0.0},
  {1.0, 0.0},
  {0.5, 0.86602540378}
  //{1.0, 0.0},
  //{0.0, 1.0}
};

__device__
double2 fetch_initial_point(unsigned long i) {
  i = i % npnts;
  return pnts[i];
}

__device__
double2 iterate_point(double2 val, unsigned long i, double2 ipnt, unsigned long func_n) {
  func_n = func_n % npnts;
  return 0.5 * val + 0.5 * pnts[func_n];
}

}

