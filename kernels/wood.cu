
#include "complex.h"

extern "C" {

__device__ const unsigned npnts = 3;

__device__
double2 pnts[npnts] = {
  {0.0, 0.0},
  {1.0, 0.0},
  {0.0, 0.86602540378}
};

__device__
double2 fetch_initial_point(unsigned long i) {
  i = i % npnts;
  return pnts[i];
}

__device__
double2 iterate_value(double2 val, unsigned long i, double2 ipnt, unsigned long func_n) {
  func_n = func_n % npnts;
  return (double)(func_n+1) / (double)npnts * (val + pnts[func_n]);
}

}

