
extern "C" {

__device__
void colorizePixel(double pixel[4], double mag, double escape, double i, double maxiter) {

  double darkener;
  if (i < maxiter) {
    double inp = (double)i / (double)maxiter;
    double x = escape / mag;
    x = (x / (double)maxiter + inp) - 0.2;
    darkener = 1 / (1.0 + exp(-15.0 * x));
  } else {
    darkener = 0.0f;
  }
  pixel[0] *= darkener;
  pixel[1] *= darkener;
  pixel[2] *= darkener;

}

}

