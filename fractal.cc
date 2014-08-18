
#include "fractal.hpp"

extern "C" {

void _fractal_worker(void *input) {
  fractal::fractal_payload *payload = (fractal::fractal_payload *)input;
  fractal *frac = payload->instance;
  frac->render(payload->x, payload->y);
  delete payload;
}

}
