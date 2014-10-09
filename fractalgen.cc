
#include <stdio.h>
#include <stdint.h>
#include <png.h>
#include <iostream>
#include <cuda_runtime.h>
#include <curand_kernel.h>

#include "helpers.h"
#include "ptx.h"
#include "common.h"

void save_png(const char *filename, uint8_t *buffer, unsigned long width, unsigned long height) {
  png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr) {
    std::cerr << "Failed to create png write struct" << std::endl;
    return;
  }
  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    std::cerr << "Failed to create info_ptr" << std::endl;
    png_destroy_write_struct(&png_ptr, NULL);
    return;
  }
  FILE *fp = fopen(filename, "wb");
  if (!fp) {
    std::cerr << "Failed to open " << filename << " for writing" << std::endl;
    png_destroy_write_struct(&png_ptr, &info_ptr);
    return;
  }
  if (setjmp(png_jmpbuf(png_ptr))) {
    png_destroy_write_struct(&png_ptr, &info_ptr);
    std::cerr << "Error from libpng!" << std::endl;
    return;
  }
  png_init_io(png_ptr, fp);
  png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
  png_write_info(png_ptr, info_ptr);
  png_byte *row_pnts[height];
  size_t i;
  for (i = 0; i < height; i++) {
    row_pnts[i] = buffer + width * 4 * i;
  }
  png_write_image(png_ptr, row_pnts);
  png_write_end(png_ptr, info_ptr);
  png_destroy_write_struct(&png_ptr, &info_ptr);
  fclose(fp);
}

cudaError_t err = cudaSuccess;
CUresult errr = CUDA_SUCCESS;

void usage(const char *progname) {
  std::cout << progname << ", version " << VERSION << ", made by DanZimm" << std::endl << std::endl;
  std::cout << "usage: " << progname << " [-m MAX_TILE_HEIGHT] [-w WIDTH] [-h HEIGHT] [-o OUTFILE] [-z]" << std::endl;
  std::cout << "                   [-l LEFT] [-r RIGHT] [-b BOTTOM] [-t TOP] [-c COLOR] [-n MAXITER]" << std::endl;
  std::cout << "                   [-e ESCAPE] [-p FRACTAL_PTX] [-q COLORIZER_PTX] [-s BASE_PTX]" << std::endl;
  std::cout << "                   [-u EXTRA_PTX]... [-k]" << std::endl << std::endl;
  std::cout << " -m : Specifies the height a tile rendered can be. Defaults to 10" << std::endl;
  std::cout << " -w : Specifies the width of the image to generate. Defaults to 1000" << std::endl;
  std::cout << " -h : Specifies the height of the image to generate. Defaults to 1000" << std::endl;
  std::cout << " -o : Specifies the name of the output png. Defaults to out.png" << std::endl;
  std::cout << " -z : Shows this help and exits" << std::endl;
  std::cout << " -c : Specify the color in hex for the fractal e.g. A9E062. Defaults to FFFFFF" << std::endl;
  std::cout << " -l : The left most `x' that will be rendered. Defaults to -2.0" << std::endl;
  std::cout << " -r : The right most `x' that will be rendered. Defaults to 2.0" << std::endl;
  std::cout << " -t : The top most `y' that will be rendered. Defaults to 2.0" << std::endl;
  std::cout << " -b : The bottom most `y' that will be rendered. Defaults to -2.0" << std::endl;
  std::cout << "      The last 4 parameters set up the coordinate system for the " << std::endl;
  std::cout << "      image to be rendered" << std::endl;
  std::cout << " -n : The maximum number of iterations to check if the sequence diverges." << std::endl;
  std::cout << "      Defaults to 100" << std::endl;
  std::cout << " -e : The escape value to test the magnitude of the current iteration against." << std::endl;
  std::cout << "      If the magnitude squared is greater than or equal to the number supplied" << std::endl;
  std::cout << "      then the loop breaks (this is unlike the usual escape value which checks" << std::endl;
  std::cout << "      directly against the magnitude. Defaults to 4.0" << std::endl;
  std::cout << " -p : The name of the PTX file containing the function `processPixel` which contains" << std::endl;
  std::cout << "      the actual algorithm for the given fractal. See below for more details." << std::endl;
  std::cout << "      Defaults to mandlebrot.ptx" << std::endl;
  std::cout << " -q : The name of the PTX file containing the `colorizePixel` function. See below" << std::endl;
  std::cout << "      for more details. Defaults to an internal file called `default_colorizer.ptx" << std::endl;
  std::cout << " -s : The name of the PTX file containing the kernel `genimage`. See below for more" << std::endl;
  std::cout << "      details. Defaults to an internal file called base_ptx.ptx" << std::endl;
  std::cout << " -u : The name of an extra PTX file. You may supply this argument up to 16 times." << std::endl;
  std::cout << "      These will be loaded when compiling the full kernel. Defaults to no extra files" << std::endl;
  std::cout << " -k : Specifies to keep the ratio of the coordinate system with respect to the" << std::endl;
  std::cout << "      specified width and height. Automatically resizes the height, keeping its" << std::endl;
  std::cout << "      original center. Defaults to being off" << std::endl;

  std::cout << std::endl;
  std::cout << "How to generate a fractal" << std::endl;
  std::cout << "-------------------------" << std::endl;
  std::cout << "In order to generate a fractal you really need to algorithms; one to specify the sequence that characterizes the fractal and another to specify how to color each pixel in accordance to the sequence. I (DanZimm) will be posting a small write up about this soon after I write this help page. Anyways, the sequence algorithm should be placed in a file that has a function with the following signiture: `void processPixel(unsigned long *ii, double *magg, unsigned long maxiter, double escape, double2 coord)`. The parameter `ii` is where you place the output number of iterations it took for the sequence to diverge. The `magg` parameter is where you should place the output magnitude squared (meaning the square magnitude of the last iteration of the sequence). The `maxiter` parameter specifies the maximum number of iterations that you should check to see if your sequence has diverged. `escape` specifies the maximum square of the magnitude that is allowed at each iteration until you should consider the sequence as divergent. Finally `coord` is the current coordinate of the pixel that is being rendered. This is in the actual fractal coordinates, not in the pixel coordinates (i.e. the top left would be (left, top) instead of (0,0) where left is the number supplied to the `-l` flag and top is the number supplied to the `-t` flag). Check out the source of mandlebrot.ptx (meaning the file mandlebrot.cu) to see an example of how this function should work. The colorizer PTX usually won't be specified since I worked pretty hard to get the default one to look nice, however if you want to create your own simply follow the file `default_colorizer.cu` in the intermediates directory of the source code. Long story short is that you are given a `double[4]` array that represents the rgba of a pixel and you're supposed to modify it according to the number of iterations the current pixel took and its final magnitude. The inital value of the pixel is the color specified with `-c`. The base PTX file contains the actual kernel. As of now `fractalgen` uses an internal one that has a lot of boilerplate code. Again if you want to write your own kernel feel free, simply follow the template `base_ptx.cu` in the intermediates directory of the source code. Finally the extra PTX allow you to separate out different PTXs from each other, hopefully allowing `fractalgen` to be more modular. At this point I don't currently use it but will in the future." << std::endl;
  exit(0);
}

// clang doesn't let me pass in a string as a macro :(
#define DEFAULT_FRACTAL_PTX_NAME "kernels/zimm.ptx"

int pown(int a, unsigned n) {
  return n == 0 ? 1 : a * pown(a, n-1);
}

bool is_number(const char *str, unsigned *out) {
  unsigned val = 0;
  size_t len = strlen(str), i;
  for (i = len-1; i != (size_t)-1; i--) {
    if (str[i] >= '0' && str[i] <= '9') {
      val += (str[i] - '0') * pown(10, i);
    } else {
      return false;
    }
  }
  if (out)
    *out = val;
  return true;
}

int main(int argc, char *const argv[]) {
  
  const char *outfile = "out.png";
  const char *base_ptx_name = NULL;
  const char *fractal_ptx_name = DEFAULT_FRACTAL_PTX_NAME;
  const char *colorizer_ptx_name = NULL;
  unsigned fractal_type = 0;
  unsigned char nother = 0;
  const char *other_ptx_names[16];
  CUmodule module;
  CUdevice dev;
  CUcontext ctx;
  CUfunction func;
  CUlinkState linkState;
  void *out_cubin = NULL;
  size_t out_len = 0;
  int dev_id = 0;
  ulong2 dims = {1000, 1000};
  unsigned long grid_height = 10, nstartpoints;
  unsigned long block_size = 32, current_row = 0;
  size_t pitch;
  uint8_t *h_pixels = NULL;
  CUdeviceptr pixels, meta, rng_states;
  metadata *h_meta = NULL;
  size_t i;
  bool keeps_ratio = false;
  unsigned long long seed = 0;
  
  unsigned long maxiter = 100;
  float frame[4] = {-2.0, 2.0, -2.0, 2.0};
  float escape = 4.0;
  double color[4] = {1.0, 1.0, 1.0, 1.0};
  
  int ch;
  while ((ch = getopt(argc, argv, "m:w:h:o:zl:r:t:b:c:n:e:p:q:u:s:k")) != -1) {
    switch (ch) {
      case 'm':
        grid_height = (unsigned long)atol(optarg);
        break;
      case 'w':
        dims.x = (unsigned long)atol(optarg);
        break;
      case 'h':
        dims.y = (unsigned long)atol(optarg);
        break;
      case 'o':
        outfile = optarg;
        break;
      case 'l':
        frame[0] = atof(optarg);
        break;
      case 'r':
        frame[1] = atof(optarg);
        break;
      case 'b':
        frame[2] = atof(optarg);
        break;
      case 't':
        frame[3] = atof(optarg);
        break;
      case 'z':
        usage(argv[0]);
        break;
      case 'c': {
        const char *hex = optarg;
        size_t len = strlen(hex);
        if (len < 6 && len > 7) {
          usage(argv[0]);
        }
        if (len == 7) {
          hex = hex + 1;
        }
        char tmp[3];
        tmp[2] = '\0';
        tmp[0] = hex[0];
        tmp[1] = hex[1];
        color[0] = (double)strtol(tmp, NULL, 16) / 255.0;
        tmp[0] = hex[2];
        tmp[1] = hex[3];
        color[1] = (double)strtol(tmp, NULL, 16) / 255.0;
        tmp[0] = hex[4];
        tmp[1] = hex[5];
        color[2] = (double)strtol(tmp, NULL, 16) / 255.0;
        color[3] = 1.0;
        } break;
      case 'n':
        maxiter = (unsigned long)atol(optarg);
        break;
      case 'e':
        escape = atof(optarg);
        break;
      case 'p':
        fractal_ptx_name = optarg;
        break;
      case 'q':
        if (!is_number(optarg, &fractal_type)) {
          colorizer_ptx_name = optarg;
        }
        break;
      case 'u':
        if (nother == 15) {
          std::cerr << "Failed to load " << optarg << " because we currently limit to 16 other ptx" << std::endl;
          exit(-1);
        }
        other_ptx_names[++nother] = optarg;
        break;
      case 's':
        if (!is_number(optarg, &fractal_type)) {
          base_ptx_name = optarg;
        }
        break;
      case 'k':
        keeps_ratio = true;
        break;
      default:
        usage(argv[0]);
        break;
    };
  }

  if (!fractal_ptx_name) {
    usage(argv[0]);
  }
  if (fractal_type >= number_fractal_types && (base_ptx_name == NULL || colorizer_ptx_name == NULL)) {
    fprintf(stderr, "Invalid fractal type: %d (we only have %d fractal types)\n", fractal_type, number_fractal_types);
    usage(argv[0]);
  }
  
  if (keeps_ratio) {
    double coordwidth = frame[1] - frame[0];
    double coordheight = coordwidth * ((double)dims.y / (double)dims.x);
    double yoff = (frame[3] + frame[2]) / 2.0;
    frame[2] = yoff - (coordheight / 2);
    frame[3] = yoff + (coordheight / 2);
  }

  dim3 threads_per_block;
  dim3 blocks;
  
  switch(fractal_type) {
    case 0: {
        threads_per_block = dim3(block_size, block_size, 1);
        dim3 remainders(dims.x % threads_per_block.x, dims.y % threads_per_block.y);
        blocks = dim3(dims.x / threads_per_block.x + (remainders.x == 0 ? 0 : 1), dims.y / threads_per_block.y + (remainders.y == 0 ? 0 : 1), 1);
      } break;
    case 1:
      nstartpoints = grid_height;
      block_size = nstartpoints;
      threads_per_block = dim3(block_size, 1, 1);
      blocks = dim3(1, 1, 1);
      break;
  };

  CU_ERR(cuInit(0));
  std::cout << "Compiling kernel" << std::endl;

  dev_id = fetch_best_device();
  CU_ERR(cuDeviceGet(&dev, dev_id));
  CU_ERR(cuCtxCreate(&ctx, 0, dev));

#define LOG_SIZE 4096

#define CU_LINK_ERR(e) \
  if ((errr = e) != CUDA_SUCCESS) { \
    const char *errstr; \
    cuGetErrorString(errr, &errstr); \
    fprintf(stderr, "CUDA link err at `%s' %d: %s\n", #e, errr, errstr); \
    fprintf(stderr, "Info log:\n%s\n", linker_info_log); \
    fprintf(stderr, "Error log:\n%s\n", linker_error_log); \
    exit(-1); \
  } \

  char linker_info_log[LOG_SIZE];
  char linker_error_log[LOG_SIZE];

  CUjit_option options[] = {
    CU_JIT_LOG_VERBOSE,
    CU_JIT_INFO_LOG_BUFFER_SIZE_BYTES,
    CU_JIT_INFO_LOG_BUFFER,
    CU_JIT_ERROR_LOG_BUFFER_SIZE_BYTES,
    CU_JIT_ERROR_LOG_BUFFER
  };
  void *option_vals[] = {
    (void *)true,
    (void *)sizeof(linker_info_log),
    (void *)linker_info_log,
    (void *)sizeof(linker_error_log),
    (void *)linker_error_log
  };
  CU_LINK_ERR(cuLinkCreate(sizeof(options)/sizeof(options[0]), (CUjit_option *)options, (void **)option_vals, &linkState));
  if (base_ptx_name) {
    CU_LINK_ERR(cuLinkAddFile(linkState, CU_JIT_INPUT_PTX, base_ptx_name, 0, NULL, NULL));
  } else {
    CU_LINK_ERR(cuLinkAddData(linkState, CU_JIT_INPUT_PTX, (void*)base_ptxs[fractal_type], strlen(base_ptxs[fractal_type]) + 1, "base_ptx" , 0, NULL, NULL));
  }
  if (colorizer_ptx_name) {
    CU_LINK_ERR(cuLinkAddFile(linkState, CU_JIT_INPUT_PTX, colorizer_ptx_name, 0, NULL, NULL));
  } else {
    CU_LINK_ERR(cuLinkAddData(linkState, CU_JIT_INPUT_PTX, (void*)colorizer_ptxs[fractal_type], strlen(colorizer_ptxs[fractal_type]) + 1, "colorizer_ptx", 0, NULL, NULL));
  }
  CU_LINK_ERR(cuLinkAddFile(linkState, CU_JIT_INPUT_PTX, fractal_ptx_name, 0, NULL, NULL));
  for (i = 0; i < nother; i++) {
    CU_LINK_ERR(cuLinkAddFile(linkState, CU_JIT_INPUT_PTX, other_ptx_names[i], 0, NULL, NULL));
  }
  CU_LINK_ERR(cuLinkComplete(linkState, &out_cubin, &out_len));
  CU_LINK_ERR(cuLinkDestroy(linkState));
  CU_ERR(cuModuleLoadData(&module, out_cubin));
  /*
  char name[100];
  cuDeviceGetName(name, 100, cuDevice);
  std::cout << "Using device " << dev_id << ": " << name << std::endl;
  */
  
  h_pixels = (uint8_t *)malloc(dims.x * 4 * sizeof(uint8_t) * dims.y);
  size_t j;
  for (j = 0; j < dims.y; j++) {
    for (i = 0; i < dims.x; i++) {
      h_pixels[j * dims.x * 4 + 4 * i + 0] = 0;
      h_pixels[j * dims.x * 4 + 4 * i + 1] = 0;
      h_pixels[j * dims.x * 4 + 4 * i + 2] = 0;
      h_pixels[j * dims.x * 4 + 4 * i + 3] = 255;
    }
  }

  h_meta = (metadata *)malloc(sizeof(metadata));
  h_meta->maxiter = maxiter;
  h_meta->first_run = true;
  for (i = 0; i < 4; i++) {
    h_meta->frame[i] = frame[i];
  }
  h_meta->escape = escape;
  for (i = 0; i < 4; i++) {
    h_meta->color[i] = color[i];
  }
  CU_ERR(cuMemAlloc(&meta, sizeof(metadata)));
  CU_ERR(cuMemcpyHtoD(meta, h_meta, sizeof(metadata)));

  std::cout << "Rendering..." << std::endl;
  
  switch (fractal_type) {
    case 0: {
      CU_ERR(cuModuleGetFunction(&func, module, "genimage"));
      while (current_row < dims.y) {
        pitch = 0;
        pixels = 0;
        
        if (current_row + grid_height < dims.y) {
          blocks.y = grid_height;
        } else {
          blocks.y = dims.y - current_row;
        }
        CU_ERR(cuMemAllocPitch(&pixels, &pitch, dims.x * 4 * sizeof(uint8_t), blocks.y * threads_per_block.y, 4));
        void *args[] = {&pixels, &pitch, &dims.x, &dims.y, &current_row, &meta};
        CU_ERR(cuLaunchKernel(func, blocks.x, blocks.y, blocks.z, threads_per_block.x, threads_per_block.y, threads_per_block.z, 0, NULL, args, NULL));

        CUDA_MEMCPY2D p;
        p.srcXInBytes = p.srcY = 0;
        p.srcMemoryType = CU_MEMORYTYPE_DEVICE;
        p.srcDevice = pixels;
        p.srcPitch = pitch;
        p.dstXInBytes = 0;
        p.dstY = current_row;
        p.dstMemoryType = CU_MEMORYTYPE_HOST;
        p.dstHost = h_pixels;
        p.dstPitch = dims.x * 4 * sizeof(uint8_t);
        p.WidthInBytes = dims.x * 4 * sizeof(uint8_t);
        p.Height = blocks.y;

        CU_ERR(cuMemcpy2D(&p));
        CU_ERR(cuMemFree(pixels));

        current_row += grid_height;
      }
    } break;
    case 1: {
      colored_point *points = (colored_point *)malloc(maxiter * nstartpoints * sizeof(colored_point));
      unsigned long bigness = 100, npnts;
      while (current_row < maxiter * nstartpoints) {
        pixels = 0;
        rng_states = 0;
        
        CU_ERR(cuModuleGetFunction(&func, module, "initrng"));
        arc4random_buf(&seed, sizeof(seed));
        CU_ERR(cuMemAlloc(&rng_states, sizeof(curandState) * threads_per_block.x * blocks.x));
        void *args_rng[] = {&rng_states, &seed};
        CU_ERR(cuLaunchKernel(func, blocks.x, blocks.y, blocks.z, threads_per_block.x, threads_per_block.y, threads_per_block.z, 0, NULL, args_rng, NULL));
        CU_ERR(cuModuleGetFunction(&func, module, "gencoord"));

        if (current_row + nstartpoints * bigness < maxiter * nstartpoints) {
          npnts = bigness;
        } else {
          npnts = maxiter - current_row / nstartpoints;
        }
        CU_ERR(cuMemAlloc(&pixels, npnts * nstartpoints * sizeof(colored_point)));
        void *args[] = {&pixels, &npnts, &meta, &rng_states};
        CU_ERR(cuLaunchKernel(func, blocks.x, blocks.y, blocks.z, threads_per_block.x, threads_per_block.y, threads_per_block.z, 0, NULL, args, NULL));

        CU_ERR(cuMemcpyDtoH((void *)((char *)points + sizeof(colored_point) * current_row), pixels, npnts * nstartpoints * sizeof(colored_point)));
        CU_ERR(cuMemFree(rng_states));
        CU_ERR(cuMemFree(pixels));

        current_row += nstartpoints * npnts;
        CU_ERR(cuMemcpyDtoH(h_meta, meta, sizeof(metadata)));
        h_meta->first_run = false;
        CU_ERR(cuMemcpyHtoD(meta, h_meta, sizeof(metadata)));
      }
      std::cout << "Placing pixels in pixel buffer" << std::endl;
      size_t badp = 0;
      for (i = 0; i < maxiter * nstartpoints; i++) {
        colored_point pnt = points[i];
        double pixel_size[2] = {(frame[1] - frame[0]) / (double)dims.x, (frame[3] - frame[2]) / (double)dims.y};
        unsigned long loc[2] = {(unsigned long)(floor((pnt.coord[0] - frame[0]) / pixel_size[0])), (unsigned long)(floor((pnt.coord[1] - frame[2]) / pixel_size[1]))};
        if (loc[0] >= dims.x || loc[1] >= dims.y) {
          //std::cout << "Bad pixel found: " << loc[0] << ", " << loc[1] << " from coords: " << pnt.coord[0] << ", " << pnt.coord[1] << std::endl;
          badp++;
          continue;
        }
        h_pixels[loc[1] * dims.x * 4 + loc[0] * 4 + 0] = pnt.color[0];
        h_pixels[loc[1] * dims.x * 4 + loc[0] * 4 + 1] = pnt.color[1];
        h_pixels[loc[1] * dims.x * 4 + loc[0] * 4 + 2] = pnt.color[2];
        h_pixels[loc[1] * dims.x * 4 + loc[0] * 4 + 3] = pnt.color[3];
      }
      std::cout << "Got " << badp << " / " << maxiter * nstartpoints << " bad points (" << (double)badp / (double)maxiter << ")" << std::endl;
      free(points);
    } break;
  }
  std::cout << "Saving image" << std::endl;

  save_png(outfile, h_pixels, dims.x, dims.y);

  free(h_pixels);

  CU_ERR(cuMemFree(meta));
  CU_ERR(cuCtxDestroy(ctx));

  return 0;
}

