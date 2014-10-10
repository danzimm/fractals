
#include "complex.h"

extern "C" {

__device__
double2 fetch_initial_point(unsigned long i) {
  return (double2){1.0,1.0};
}

__device__
static double _z = 1.0, h = 0.0001, sigma = 10.0, rho = 28.0, beta = 2.6666666667;

__device__
double2 iterate_point(double2 val, unsigned long i, double2 ipnt, unsigned long func_n) {
  double x = val.x, y = val.y, z = _z;
  //double x = _z, y = val.x, z = val.y;
  double nx, ny, nz;
  nx = x + h * sigma * y - h * sigma * x;
  ny = y + h * x * rho - h * x * z - h * y;
  nz = z + h * x * y - h * beta * z;
  _z = nz;
  return (double2){nx, ny};
  //_z = nx;
  //return (double2){ny, nz};
}

}

