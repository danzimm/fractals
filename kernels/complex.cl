
double2 complex_exp(double2);
double complex_mag2(double2);
double complex_mag(double2);
double2 complex_ln(double2 val);
double2 complex_mult(double2 vala, double2 valb);
double2 complex_pow(double2 val, double2 w);
double2 complex_pown(double2 val, ulong n);

double2 complex_exp(double2 val) {
  return exp(val.x) * (double2)(cos(val.y), sin(val.y));
}

double complex_mag2(double2 val) {
  return pown(val.x, 2) + pown(val.y, 2);
}

double complex_mag(double2 val) {
  return sqrt(complex_mag2(val));
}

double2 complex_ln(double2 val) {
  return (double2)(log(complex_mag(val)), atan2(val.y, val.x));
}

double2 complex_mult(double2 vala, double2 valb) {
  return (double2)(vala.x * valb.x - vala.y * valb.y, vala.x * valb.y + vala.y * valb.x);
}

double2 complex_pow(double2 val, double2 w) {
  return complex_exp(complex_mult(w, complex_ln(val)));
}

double2 complex_pown(double2 val, ulong n) {
  double2 ret = val;
  ulong i;
  for (i = 1; i < n; i++) {
    ret = complex_mult(ret, val);
  }
  return ret;
}

double2 complex_divide(double2 vala, double2 valb) {
  double diver = 1 / complex_mag2(valb);
  return diver * complex_mult(vala, (double2)(valb.x, -valb.y));
}

