
#include "fractal.hpp"

extern "C" {

void _fractal_worker(void *input) {
  fractal::payload *payload = (fractal::payload *)input;
  fractal *frac = payload->instance;
  frac->render(payload->x, payload->y);
  delete payload;
}

}
