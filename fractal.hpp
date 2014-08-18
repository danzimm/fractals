
#include <dispatch/dispatch.h>
#include <png.h>

#include <cstdint>
#include <complex>
#include <cmath>
#include <functional>

#ifndef __fractal_H
#define __fractal_H


extern "C" {
  void _fractal_worker(void *);
};

class fractal {
public:
  struct pixel_data {
    uint64_t iterations;
    long double modulus;
  };
  template<typename T>
  struct range {
    T first, last;
  };
  struct fractal_payload {
    fractal *instance;
    range<uint64_t> x, y;
  };
  struct fractal_color {
    long double red, green, blue;
    friend fractal_color operator*(long double multi, const fractal_color& rhs) {
      return fractal_color{multi * rhs.red, multi * rhs.green, multi * rhs.blue};
    }
    long double& operator[](uint8_t i) {
      switch (i) {
        case 0:
          return red;
        case 1:
          return green;
        case 2:
          return blue;
        default:
          throw "Out of range";
      }
    }
  };
  enum image_format {
    png8,
    png16
  };
  uint64_t max_iterations;

private:
  uint64_t width, height, nontrivialiteration, nnontrivialpixels;
  uint16_t nworkers;
  long double left, bottom, top, right, escape, xstep, ystep;
  dispatch_queue_t queue;
  dispatch_group_t group;
  bool rendering;
  pixel_data *buffer;

protected:
  virtual void _finishRendering() {
    rendering = false;
  }

public:

  std::function<std::complex<long double>(std::complex<long double>, std::complex<long double>, uint64_t)> value_iterator;
  std::function<fractal_color(fractal&, fractal_color, uint64_t, uint64_t, pixel_data)> colorizer;

  fractal(uint64_t w, uint64_t h, long double l, long double b, long double t, long double r, uint16_t nw = 8, uint64_t max = 100, long double e = 37.0) : max_iterations(max), width(w), height(h), nworkers(nw), left(l), bottom(b), top(t), right(r), escape(e) {
    nworkers &= ~1;
    xstep = (right - left) / (long double)width;
    ystep = (top - bottom) / (long double)height;
    buffer = new pixel_data[width * height];
    group = NULL;
    queue = NULL;
    value_iterator = NULL;
    colorizer = NULL;
    rendering = false;
  }

  virtual ~fractal() {
    delete buffer;
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
      fractal_payload *payload = new fractal_payload;
      payload->instance = this;
      payload->x = range<uint64_t>{0, halfwidth};
      payload->y = range<uint64_t>{by, ey};
      dispatch_group_async_f(group, queue, payload, _fractal_worker);
      payload = new fractal_payload;
      payload->instance = this;
      payload->x = range<uint64_t>{halfwidth, width};
      payload->y = range<uint64_t>{by, ey};
      dispatch_group_async_f(group, queue, payload, _fractal_worker);
    }
    dispatch_group_notify(group, queue, ^{
      _finishRendering();
    });
    if (!async) {
      dispatch_group_wait(group, DISPATCH_TIME_FOREVER);
      rendering = false;
    }
    return (*this);
  }
  
  fractal& render(range<uint64_t> x, range<uint64_t> y) {
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

  bool create_image(char *name, fractal_color color = fractal_color{1.0,1.0,1.0}, image_format format = image_format::png16) {
    if (format != image_format::png8 && format != image_format::png16) {
      throw "Unknown image format!";
    }
    if (!hasRendered()) {
      fprintf(stderr, "ERR: Failed to create image, not done rendering!\n");
      return false;
    }
    png_bytep *row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * height);
    uint64_t i, j;
    bool eightbit = format == image_format::png8;
    for (i = 0; i < height; i++) { // TODO: make this more memory efficient (per row)
      row_pointers[i] = (png_bytep)malloc(sizeof(png_byte) * width * 3 * (eightbit ? 1 : 2));
      for (j = 0; j < width; j++) {
        fractal_color col = color_for_pixel(color, i, j, buffer[j + (height - i - 1) * width]);
        if (eightbit) {
          row_pointers[i][0 + j*3] = (png_byte)floor(col.red * 255.0);
          row_pointers[i][1 + j*3] = (png_byte)floor(col.green * 255.0);
          row_pointers[i][2 + j*3] = (png_byte)floor(col.blue * 255.0);
        } else {
          ((uint16_t *)(row_pointers[i]))[0 + j*3] = (uint16_t)floor(col.red * 65535.0);
          ((uint16_t *)(row_pointers[i]))[1 + j*3] = (uint16_t)floor(col.green * 65535.0);
          ((uint16_t *)(row_pointers[i]))[2 + j*3] = (uint16_t)floor(col.blue * 65535.0);
        }
      }
    }
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
      free(row_pointers);
      fprintf(stderr, "ERR: Failed to create png write struct\n");
      return false;
    }
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
      free(row_pointers);
      png_destroy_write_struct(&png_ptr, NULL);
      fprintf(stderr, "ERR: Failed to create png info struct\n");
      return false;
    }
    FILE *fp = fopen(name, "wb");
    if (!fp) {
      free(row_pointers);
      fprintf(stderr, "ERR: Failed to open %s for writing the png\n", name);
      fclose(fp);
      return false;
    }
    if (setjmp(png_jmpbuf(png_ptr))) {
      free(row_pointers);
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
    png_write_image(png_ptr, row_pointers);
    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    free(row_pointers);
    fclose(fp);
    return true;
  }
  
  fractal_color color_for_pixel(fractal_color input, uint64_t x, uint64_t y, pixel_data data) {
    return colorizer ? colorizer((*this), input, x, y, data) : input;
  }

};

#endif

