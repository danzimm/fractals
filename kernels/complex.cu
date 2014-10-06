
__device__
const double2 operator-(const double2& vala, const double2& valb) {
  return (double2){vala.x - valb.x, vala.y - valb.y};
}

__device__
const double2 operator+(const double2& vala, const double2& valb) {
  return (double2){vala.x + valb.x, vala.y + valb.y};
}

__device__
const double2 operator*(const double& vala, const double2& valb) {
  return (double2){vala * valb.x, vala * valb.y};
}

__device__
const double2 operator*(const double2& vala, const double& valb) {
  return valb * vala;
}

__device__ double2 complex_exp(double2 val) {
  return exp(val.x) * (double2){cos(val.y), sin(val.y)};
}

__device__ double pown(double val, unsigned long i) {
  unsigned long j;
  double ret = val;
  for (j = 1; j < i; j++) {
    ret = ret * val;
  }
  return ret; 
}

__device__ double complex_mag2(double2 val) {
  return pown(val.x, 2) + pown(val.y, 2);
}

__device__ double complex_mag(double2 val) {
  return sqrt(complex_mag2(val));
}

__device__ double2 complex_ln(double2 val) {
  return (double2){log(complex_mag(val)), atan2(val.y, val.x)};
}

__device__ double2 complex_mult(double2 vala, double2 valb) {
  return (double2){vala.x * valb.x - vala.y * valb.y, vala.x * valb.y + vala.y * valb.x};
}

__device__ double2 complex_pow(double2 val, double2 w) {
  return complex_exp(complex_mult(w, complex_ln(val)));
}

__device__ double2 complex_pown(double2 val, unsigned long n) {
  double2 ret = val;
  unsigned long i;
  for (i = 1; i < n; i++) {
    ret = complex_mult(ret, val);
  }
  return ret;
}

__device__ double2 complex_divide(double2 vala, double2 valb) {
  double diver = 1 / complex_mag2(valb);
  return diver * complex_mult(vala, (double2){valb.x, -valb.y});
}


