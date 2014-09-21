
double color_darkener_o(ulong i, ulong m, double mag, double mmag, double oset) {
  if (i < m) {
    double inp = (double)(i) / (double)(m);
    double x = mmag / mag;
    x = (x / (double)(m) + inp) - 0.2;
    return 1 / (1.0 + exp(-15.0 * x));
  }
  return 0.0;
}

double color_darkener(ulong i, ulong m, double mag, double mmag) {
  return color_darkener_o(i, m, mag, mmag, 0.2);
}

