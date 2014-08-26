
#include <dispatch/dispatch.h>
#include <png.h>

#include <cstdint>
#include <complex>
#include <cmath>
#include <functional>
#include <vector>
#include <iostream>

#ifndef __fractal_H
#define __fractal_H


extern "C" {
  void _fractal_worker(void *);
};

class fractal {
public:
  struct color {
    long double r, g, b;
    friend color operator*(long double multi, const color& rhs) {
      return color{multi * rhs.r, multi * rhs.g, multi * rhs.b};
    }
    friend color operator+(const color& c1, const color& c2) {
      return color{c1.r + c2.r, c1.g + c2.g, c1.b + c2.b};
    }
    long double& operator[](uint8_t i) {
      switch (i) {
        case 0:
          return r;
        case 1:
          return g;
        case 2:
          return b;
        default:
          throw "Out of range";
      }
    }
  };

  class gradient {
  public:
    struct component {
      fractal::color c;
      long double location;
    };

  protected:
    std::vector<component> components;

    void _prepare(void) {
      size_t s = components.size();
      if (s > 0) {
        std::sort(components.begin(), components.end(), [](const component& g1, const component& g2) -> bool {
          return g1.location < g2.location;
        });
        /*
        components[0].location = 0.0f;
        if (s > 1)
          components[s-1].location = 1.0f;
        */
      }
    }

  public:
    gradient& add_color(color c) {
      components.push_back(component{c, 0.0});
      _prepare();
      return (*this);
    }
    gradient& add_color(color c, long double loc) {
      components.push_back(component{c, loc});
      _prepare();
      return (*this);
    }
    color color_for_location(long double loc) {
      loc = loc < 0.0 ? 0.0 : loc;
      loc = loc > 1.0 ? 1.0 : loc;
      
      component comp, compp;
      long double scale, offset;
      size_t i, place = (size_t)-1, s = components.size();
      for (i = 0; i < s; i++) {
        comp = components[i];
        if (comp.location > loc) {
          place = i - 1;
          break;
        }
      }
      /*
      if (i == 0 && place == (size_t)-1) {
        comp = components[0];
        compp = comp;
        scale = 1.0 / compp.location;
      } else if (i == s && place == (size_t)-1) {
        comp = components[s-1];
        compp = comp;
        scale = 1.0 / (1.0 - comp.location);
      } else {
      */
        comp = components[place];
        compp = components[place+1];
        scale = 1.0 / (compp.location - comp.location);
      //}
      offset = comp.location;
      return (1.0 - scale * (loc - offset)) * comp.c + scale * (loc - offset) * compp.c;
    }

  };
  struct pixel_data {
    uint64_t iterations;
    long double modulus;
  };
  template<typename T>
  struct range {
    T first, last;
  };
  struct payload {
    fractal *instance;
    range<uint64_t> x, y;
  };
  enum image_format {
    png8,
    png16
  };
  int verbosity;
  uint64_t max_iterations;
  long double escape;

private:
  uint64_t width, height, nontrivialiteration, nnontrivialpixels, *histogram, sumhisto;
  uint16_t nworkers;
  long double left, bottom, top, right, xstep, ystep;
  dispatch_queue_t queue;
  dispatch_group_t group;
  bool rendering;
  pixel_data *buffer;

protected:
  virtual void _finishRendering() {
    rendering = false;
    uint64_t i;
    sumhisto = 0;
    for (i = 0; i < max_iterations; i++) {
      sumhisto += histogram[i];
    }
  }

public:

  std::function<std::complex<long double>(std::complex<long double>, std::complex<long double>, uint64_t)> value_iterator;
  std::function<color(fractal&, color, uint64_t, uint64_t, pixel_data, uint64_t *, uint64_t)> colorizer;

  fractal(uint64_t w, uint64_t h, long double l, long double b, long double t, long double r, uint16_t nw = 8, uint64_t max = 100, long double e = 37.0) : verbosity(0), max_iterations(max), escape(e), width(w), height(h), histogram(NULL), nworkers(nw), left(l), bottom(b), top(t), right(r) {
    nworkers &= ~1;
    xstep = (right - left) / (long double)width;
    ystep = (top - bottom) / (long double)height;
    buffer = new pixel_data[width * height];
    group = NULL;
    queue = NULL;
    value_iterator = NULL;
    colorizer = NULL;
    rendering = false;
    histogram = new uint64_t[max_iterations+1];
  }

  virtual ~fractal() {
    delete [] buffer;
    delete [] histogram;
    if (queue) {
      dispatch_release(queue);
    }
    if (group) {
      dispatch_release(group);
    }
  }

  fractal& render(bool async = true) {
    uint16_t halfworkers = nworkers / 2, i;
    uint64_t by, ey, halfwidth, pieceheight;
    halfwidth = width / 2;
    pieceheight = height / halfworkers;
    queue = dispatch_queue_create("fractal queue", DISPATCH_QUEUE_CONCURRENT);
    group = dispatch_group_create();
    rendering = true;
    for (i = 0; i < halfworkers; i++) {
      by = i * pieceheight;
      ey = (i+1) * pieceheight;
      if (i == halfworkers - 1) {
        ey = height;
      }
      payload *pl = new payload;
      pl->instance = this;
      pl->x = range<uint64_t>{0, halfwidth};
      pl->y = range<uint64_t>{by, ey};
      dispatch_group_async_f(group, queue, pl, _fractal_worker);
      pl = new payload;
      pl->instance = this;
      pl->x = range<uint64_t>{halfwidth, width};
      pl->y = range<uint64_t>{by, ey};
      dispatch_group_async_f(group, queue, pl, _fractal_worker);
    }
    if (async) {
      dispatch_group_notify(group, queue, ^{
        _finishRendering();
      });
    } else {
      dispatch_group_wait(group, DISPATCH_TIME_FOREVER);
      _finishRendering();
    }
    return (*this);
  }
  
  fractal& render(range<uint64_t> x, range<uint64_t> y) {
    if (verbosity >= 1) {
      printf("Rendering x:{%llu,%llu} y:{%llu,%llu}\n", x.first, x.last, y.first, y.last);
    }
    uint64_t i, j;
    pixel_data tmp;
    for (i = x.first; i < x.last; i++) {
      for (j = y.first; j < y.last; j++) {
        buffer[i + j*width] = tmp = fetch_pixel(left + i * xstep, bottom + j * ystep);
        if (tmp.iterations != 0 && tmp.iterations != max_iterations) {
          nnontrivialpixels++;
          nontrivialiteration += tmp.iterations;
        }
      }
    }
    if (verbosity >= 1) {
      printf("Finished rendering x:{%llu,%llu} y:{%llu,%llu}\n", x.first, x.last, y.first, y.last);
    }
    return (*this);
  }
  
  pixel_data fetch_pixel(long double x, long double y) {
    uint64_t i;
    std::complex<long double> val(x, y), initial(x, y);
    for (i = 0; i < max_iterations; i++) {
      val = iterate_value(val, initial, i);
      if (std::norm(val) >= escape) {
        break;
      }
    }
    histogram[i]++;
    return pixel_data{i, std::norm(val)};
  }
  
  virtual std::complex<long double> iterate_value(std::complex<long double> value, std::complex<long double> initial, uint64_t i) {
    return value_iterator ? value_iterator(value, initial, i) : value + initial;
  }

  bool isRendering() {
    return rendering;
  }
  bool hasRendered() {
    return !rendering && queue != NULL;
  }

  bool create_image(char *name, color c = color{1.0,1.0,1.0}, image_format format = image_format::png16) {
    if (format != image_format::png8 && format != image_format::png16) {
      throw "Unknown image format!";
    }
    if (!hasRendered()) {
      fprintf(stderr, "ERR: Failed to create image, not done rendering!\n");
      return false;
    }
    bool eightbit = format == image_format::png8;
    png_bytep row_pointer = (png_bytep)malloc(sizeof(png_byte) * width * 3 * (eightbit ? 1 : 2));
    uint64_t i, j;

    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
      free(row_pointer);
      fprintf(stderr, "ERR: Failed to create png write struct\n");
      return false;
    }
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
      free(row_pointer);
      png_destroy_write_struct(&png_ptr, NULL);
      fprintf(stderr, "ERR: Failed to create png info struct\n");
      return false;
    }
    FILE *fp = fopen(name, "wb");
    if (!fp) {
      free(row_pointer);
      fprintf(stderr, "ERR: Failed to open %s for writing the png\n", name);
      fclose(fp);
      return false;
    }
    if (setjmp(png_jmpbuf(png_ptr))) {
      free(row_pointer);
      png_destroy_write_struct(&png_ptr, &info_ptr);
      fprintf(stderr, "ERR: Error from libpng! Break at line 64\n");
      return false;
    }
    png_init_io(png_ptr, fp);
    png_set_IHDR(png_ptr, info_ptr, width, height, eightbit ? 8 : 16, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    //png_set_invert_alpha(png_ptr); // TODO: add alpha to rgb
    png_write_info(png_ptr, info_ptr);
    if (!eightbit)
      png_set_swap(png_ptr);
    
    for (i = 0; i < height; i++) { // TODO: make this more memory efficient (per row)
      for (j = 0; j < width; j++) {
        color col = color_for_pixel(c, i, j, buffer[j + (height - i - 1) * width]);
        if (eightbit) {
          row_pointer[0 + j*3] = (png_byte)floor(col.r * 255.0);
          row_pointer[1 + j*3] = (png_byte)floor(col.g * 255.0);
          row_pointer[2 + j*3] = (png_byte)floor(col.b * 255.0);
        } else {
          ((uint16_t *)row_pointer)[0 + j*3] = (uint16_t)floor(col.r * 65535.0);
          ((uint16_t *)row_pointer)[1 + j*3] = (uint16_t)floor(col.g * 65535.0);
          ((uint16_t *)row_pointer)[2 + j*3] = (uint16_t)floor(col.b * 65535.0);
        }
      }
      png_write_rows(png_ptr, &row_pointer, 1);
    }
    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    free(row_pointer);
    fclose(fp);
    return true;
  }
  
  color color_for_pixel(color input, uint64_t x, uint64_t y, pixel_data data) {
    return colorizer ? colorizer((*this), input, x, y, data, histogram, sumhisto) : input;
  }

};

#endif

