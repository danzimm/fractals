
#include <stdio.h>
#include <stdint.h>
#include <png.h>
#include <iostream>
#include <cuda_runtime.h>

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
  std::cout << "                   [-l LEFT] [-r RIGHT] [-b BOTTOM] [-t TOP]" << std::endl << std::endl;
  std::cout << "This program take in an OpenCL program which has a kernel that takes a 2d image for writing, a ulong4 of metadata and a coordinateframe" << std::endl;
  std::cout << "where the metadata is of the form {offsetx, offsety, width, height} where offsetx/y is the offset of the tile currently being rendered" << std::endl;
  std::cout << "(i.e. the pixel coordinate of the tile being rendered) and width/height are the total width/height of the entire image." << std::endl;
  std::cout << "This was originally created in order to generate fractals but can be used to generate any image that needs to be big." << std::endl << std::endl;
  std::cout << "  -m : Specifies the height a tile rendered can be. Defaults to 10" << std::endl;
  std::cout << "  -w : Specifies the width of the image to generate" << std::endl;
  std::cout << "  -h : Specifies the height of the image to generate" << std::endl;
  std::cout << "  -o : Specifies the name of the output png" << std::endl;
  std::cout << "  -z : Shows this help and exits" << std::endl;
  std::cout << "  -c : Specify the color in hex for the fractal" << std::endl;
  std::cout << "  -l : The left most `x' that will be rendered" << std::endl;
  std::cout << "  -r : The right most `x' that will be rendered" << std::endl;
  std::cout << "  -t : The top most `y' that will be rendered" << std::endl;
  std::cout << "  -b : The bottom most `y' that will be rendered" << std::endl;
  std::cout << "    The last 4 parameters set up the coordinate system for the image to be rendered." << std::endl;
// #error Add flags to usage for the ptx inputs
  exit(0);
}

int main(int argc, char *const argv[]) {
  
  const char *outfile = "out.png";
  const char *base_ptx_name = NULL;
  const char *fractal_ptx_name = "mandlebrot.ptx";
  const char *colorizer_ptx_name = NULL;
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
  unsigned long grid_height = 10;
  unsigned long block_size = 32, current_row = 0;
  size_t pitch;
  uint8_t *h_pixels = NULL;
  CUdeviceptr pixels;
  metadata *h_meta = NULL;
  CUdeviceptr meta;
  size_t i;
  
  unsigned long maxiter = 100;
  float frame[4] = {-2.0, 2.0, -2.0, 2.0};
  float escape = 4.0;
  double color[4] = {1.0, 1.0, 1.0, 1.0};
  
  int ch;
  while ((ch = getopt(argc, argv, "m:w:h:o:zl:r:t:b:c:n:e:p:q:u:s:")) != -1) {
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
        colorizer_ptx_name = optarg;
        break;
      case 'u':
        if (nother == 15) {
          std::cerr << "Failed to load " << optarg << " because we currently limit to 16 other ptx" << std::endl;
          exit(-1);
        }
        other_ptx_names[++nother] = optarg;
        break;
      case 's':
        base_ptx_name = optarg;
        break;
      default:
        usage(argv[0]);
        break;
    };
  }

  dim3 threads_per_block(block_size, block_size, 1);
  dim3 remainders(dims.x % threads_per_block.x, dims.y % threads_per_block.y);
  dim3 blocks(dims.x / threads_per_block.x + (remainders.x == 0 ? 0 : 1), dims.y / threads_per_block.y + (remainders.y == 0 ? 0 : 1), 1);

  CU_ERR(cuInit(0));
  std::cout << "Compiling kernel" << std::endl;

  dev_id = fetch_best_device();
  CU_ERR(cuDeviceGet(&dev, dev_id));
  CU_ERR(cuCtxCreate(&ctx, 0, dev));
  CU_ERR(cuLinkCreate(0, NULL, NULL, &linkState));
  if (base_ptx_name) {
    CU_ERR(cuLinkAddFile(linkState, CU_JIT_INPUT_PTX, base_ptx_name, 0, NULL, NULL));
  } else {
    CU_ERR(cuLinkAddData(linkState, CU_JIT_INPUT_PTX, (void*)base_ptx, sizeof(base_ptx), "base_ptx", 0, NULL, NULL));
  }
  if (colorizer_ptx_name) {
    CU_ERR(cuLinkAddFile(linkState, CU_JIT_INPUT_PTX, colorizer_ptx_name, 0, NULL, NULL));
  } else {
    CU_ERR(cuLinkAddData(linkState, CU_JIT_INPUT_PTX, (void*)default_colorizer, sizeof(default_colorizer), "default_colorizer", 0, NULL, NULL));
  }
  CU_ERR(cuLinkAddFile(linkState, CU_JIT_INPUT_PTX, fractal_ptx_name, 0, NULL, NULL));
  for (i = 0; i < nother; i++) {
    CU_ERR(cuLinkAddFile(linkState, CU_JIT_INPUT_PTX, other_ptx_names[i], 0, NULL, NULL));
  }
  CU_ERR(cuLinkComplete(linkState, &out_cubin, &out_len));
  CU_ERR(cuLinkDestroy(linkState));
  CU_ERR(cuModuleLoadData(&module, out_cubin));
  CU_ERR(cuModuleGetFunction(&func, module, "genimage"));
  /*
  char name[100];
  cuDeviceGetName(name, 100, cuDevice);
  std::cout << "Using device " << dev_id << ": " << name << std::endl;
  */
  
  h_pixels = (uint8_t *)malloc(dims.x * 4 * sizeof(uint8_t) * dims.y);
  memset(h_pixels, 0, dims.x * 4 * sizeof(uint8_t) * dims.y);
  h_meta = (metadata *)malloc(sizeof(metadata));
  h_meta->maxiter = maxiter;
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
  
  std::cout << "Saving image" << std::endl;

  save_png(outfile, h_pixels, dims.x, dims.y);

  free(h_pixels);

  CU_ERR(cuMemFree(meta));
  CU_ERR(cuCtxDestroy(ctx));

  return 0;
}

