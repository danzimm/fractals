
#include <unistd.h>
#include <dlfcn.h>
#include "fractal.hpp"

static long double _param = 0.0f;
static long double _colexp = 4.0f;

std::complex<long double> default_iterator(std::complex<long double> value, std::complex<long double> initial, uint64_t i) {
  return std::pow(value, _param) - value * std::sqrt(value) + initial;
}

fractal::fractal_color default_colorizer(fractal& instance, fractal::fractal_color color, uint64_t x, uint64_t y, fractal::pixel_data data) {
  if (data.iterations == instance.max_iterations)
    return 0.0 * color;
  long double darkener = 1 - std::pow(1 - (long double)data.iterations / (long double)instance.max_iterations, _colexp);
  return darkener * color;
}

int main(int argc, char * const argv[]) {

  int ch, verbosity = 0, workers = 8, depth = 16;
  long double right, left, top, bottom, red, green, blue, escape = 37.0, colexp = 4.0, param = 0.1;
  uint64_t width, height, maxiterations;
  char *out = (char *)malloc(8), *generator = NULL;
  strcpy(out, "out.png");
  bool keepsratio = false;
  right = top = 2.0;
  bottom = left = -2.0;
  red = green = blue = 1.0;
  maxiterations = 100;
  width = height = 1000;
  
  while ((ch = getopt(argc, argv, "r:l:t:b:w:h:vc:m:ho:n:e:d:kj:p:g:")) != -1) {
    switch (ch) {
      case 'r':
        right = strtold(optarg, NULL);
        break;
      case 'l':
        left = strtold(optarg, NULL);
        break;
      case 't':
        top = strtold(optarg, NULL);
        break;
      case 'b':
        bottom = strtold(optarg, NULL);
        break;
      case 'w':
        width = atoll(optarg);
        break;
      case 'h':
        height = atoll(optarg);
        break;
      case 'v':
        printf("Increasing verbo!\n");
        verbosity++;
        break;
      case 'c':
        if (optarg[0] == '#') {
          optarg++;
        }
        if (strlen(optarg) != 6) {
          fprintf(stderr, "Need a color with 6 hex nibbles\n");
          return -1;
        }
        char tmp[3];
        tmp[2] = '\0';
        tmp[0] = optarg[0];
        tmp[1] = optarg[1];
        red = (long double)strtol(tmp, NULL, 16) / 255.0;
        tmp[0] = optarg[2];
        tmp[1] = optarg[3];
        green = (long double)strtol(tmp, NULL, 16) / 255.0;
        tmp[0] = optarg[4];
        tmp[1] = optarg[5];
        blue = (long double)strtol(tmp, NULL, 16) / 255.0;
        break;
      case 'm':
        maxiterations = atoll(optarg);
        break;
      case 'o':
        out = (char *)reallocf(out, strlen(optarg)+1);
        strcpy(out, optarg);
        break;
      case 'n':
        workers = atoi(optarg);
        break;
      case 'e':
        escape = atof(optarg);
        break;
      case 'd':
        depth = atoi(optarg);
        if (depth != 8 && depth != 16) {
          fprintf(stderr, "Invalid bit depth %d\n", depth);
          depth = 16;
        }
        break;
      case 'k':
        keepsratio = !keepsratio;
        break;
      case 'j':
        colexp = atof(optarg);
        break;
      case 'p':
        param = atof(optarg);
        break;
      case 'g':
        generator = (char *)malloc(strlen(optarg)+1);
        strcpy(generator, optarg);
        break;
      default:
        fprintf(stderr, "Unknown option %c\n", ch);
        return -1;
    }
  }
  if (keepsratio) {
    long double ratio = (long double)height / (long double)width, h = ratio * (right - left);
    top = bottom + (top - bottom) / 2 + h / 2;
    bottom = top - h;
  }
  
  void *lib = dlopen(generator, RTLD_LAZY);
  typedef std::complex<long double> (*iterator_creator)(std::complex<long double>, std::complex<long double>, uint64_t);
  typedef fractal::fractal_color (*colorizer_creator)(fractal&, fractal::fractal_color, uint64_t, uint64_t, fractal::pixel_data);
  
  iterator_creator (*iteratorsym)(long double) = (iterator_creator (*)(long double))dlsym(lib, "value_iterator");
  colorizer_creator (*colorizersym)(long double) = (colorizer_creator (*)(long double))dlsym(lib, "colorizer");

  _colexp = colexp;
  _param = param;

  fractal *frac = new fractal(width, height, left, bottom, top, right, workers, maxiterations, escape);
  
  frac->value_iterator = iteratorsym ? iteratorsym(param) : default_iterator;
  frac->colorizer = colorizersym ? colorizersym(colexp) : default_colorizer;

  frac->render(false).create_image(out, fractal::fractal_color{red, green, blue});

  free(out);
  dlclose(lib);
  if (generator)
    free(generator);
  delete frac;
  return 0;
}

