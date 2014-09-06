
#include "fractal.hpp"
#include <vector>
#include <iostream>

static std::complex<long double> parameter = 0.0f;

std::complex<long double> julia_iterator(std::complex<long double> value, std::complex<long double> initial, uint64_t i) {
  return i == 0 ? initial : value * value + parameter;
}

fractal::gradient _g, _gg;

fractal::color julia_colorizer(fractal& instance, fractal::color color, uint64_t x, uint64_t y, fractal::pixel_data data) {
  if (data.iterations == instance.max_iterations) {
    return color;
    //return _g.color_for_location(/*1.0 / (std::exp(-5.0*data.modulus + 6.0) + 1.0)*/data.modulus / instance.escape);
  }
  return _gg.color_for_location((long double)data.iterations / (long double)instance.max_iterations);
  /*
  long double darkener = 1 - std::pow(1 - (long double)data.iterations / (long double)instance.max_iterations, 4.0);
  return darkener * color;
  */
}

extern "C" {
  
  void *value_iterator(long double parama, long double paramb) {
    parameter = std::complex<long double>(parama, paramb);
    return (void *)julia_iterator;
  }
  
  void *colorizer(long double cparam, long double red, long double green, long double blue) {
    _g.add_color(fractal::color{98.0/255.0, 118.0/255.0, 224.0/255.0}, 0.0).add_color(fractal::color{0.862745098,0.862745098,0.862745098}, 1.0)
      .add_color(fractal::color{212.0/255.0, 128.0/255.0, 93.0/255.0}, 0.2);
    _gg.add_color(fractal::color{76.0/255.0,84.0/255.0,173.0/255.0}, 0.0).add_color(fractal::color{1.0,1.0,200.0/255.0}, 1.0)
      .add_color(fractal::color{0.941176471,0.941176471,0.941176471}, 0.4);
    return (void *)julia_colorizer;
  }

}

