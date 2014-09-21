
#include <OpenCL/opencl.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstdint>
#include <png.h>
#include <unistd.h>

#ifndef VERSION
#define VERSION 0.1
#endif

static cl_int err = CL_SUCCESS;

#define CL_ERR( line ) \
  if ( (err = ( line )) != CL_SUCCESS) { \
    std::cerr << "Error (" << err << ") at line: " << __LINE__ << " - " << #line << std::endl; \
    exit(1); \
  } \

void fetch_device_info(cl_device_id id, cl_device_info inf, std::string& desc) {
  size_t s;
  CL_ERR(clGetDeviceInfo(id, inf, 0, NULL, &s));
  desc.resize(s);
  CL_ERR(clGetDeviceInfo(id, inf, s, const_cast<char*>(desc.data()), NULL));
}

void fetch_device_description(cl_device_id id, std::string& desc) {
  static cl_device_info infs[] = {CL_DEVICE_NAME, CL_DEVICE_VENDOR, CL_DEVICE_VERSION, CL_DRIVER_VERSION, CL_DEVICE_OPENCL_C_VERSION, CL_DEVICE_PROFILE, CL_DEVICE_EXTENSIONS};
  size_t i;
  for (i = 0; i < sizeof(infs) / sizeof(infs[0]); i++) {
    std::string tmp;
    fetch_device_info(id, infs[i], tmp);
    desc.append(tmp).append(" ");
  }
  desc.pop_back();
}

void fetch_platform_info(cl_platform_id id, cl_platform_info inf, std::string& desc) {
  size_t s;
  CL_ERR(clGetPlatformInfo(id, inf, 0, NULL, &s));
  desc.resize(s);
  CL_ERR(clGetPlatformInfo(id, inf, s, const_cast<char*>(desc.data()), NULL));
}

void fetch_platform_description(cl_platform_id id, std::string& desc) {
  static cl_platform_info infs[] = {CL_PLATFORM_NAME, CL_PLATFORM_VENDOR, CL_PLATFORM_VERSION, CL_PLATFORM_PROFILE, CL_PLATFORM_EXTENSIONS};
  size_t i;
  for (i = 0; i < sizeof(infs) / sizeof(infs[0]); i++) {
    std::string tmp;
    fetch_platform_info(id, infs[i], tmp);
    desc.append(tmp).append(" ");
  }
  desc.pop_back();
}

std::string load_file(const std::string filename) {
  std::ifstream in(filename);
  return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

void save_png(const char *filename, uint8_t *buffer, cl_ulong width, cl_ulong height) {
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

cl_int fetch_platform_device_ids(cl_platform_id *platform_id, cl_uint *device_id_count, cl_device_id **device_ids) {
  cl_uint nids = 0;
  CL_ERR(clGetPlatformIDs(0, NULL, &nids));
  if (nids == 0) {
    std::cerr << "Failed to find an opencl platform!" << std::endl;
    return CL_OUT_OF_HOST_MEMORY;
  }
  CL_ERR(clGetPlatformIDs(1, platform_id, NULL));

  CL_ERR(clGetDeviceIDs(*platform_id, CL_DEVICE_TYPE_ALL, 0, NULL, &nids));
  if (nids == 0) {
    std::cerr << "Failed to find an opencl device!" << std::endl;
  }
  cl_device_id *dev_ids = (cl_device_id*)malloc(sizeof(cl_device_id) * nids);
  CL_ERR(clGetDeviceIDs(*platform_id, CL_DEVICE_TYPE_ALL, nids, dev_ids, NULL));
  *device_ids = dev_ids;
  *device_id_count = nids;
  return CL_SUCCESS;
}

cl_context create_context(cl_platform_id platform_id, cl_uint device_id_count, cl_device_id *device_ids) {
  const cl_context_properties contextProperties[] = { CL_CONTEXT_PLATFORM, reinterpret_cast<cl_context_properties>(platform_id), 0 };
  cl_context ctx = clCreateContext(contextProperties, device_id_count, device_ids, NULL, NULL, &err);
  CL_ERR(err);
  return ctx;
}

cl_program create_program(cl_context ctx, const char *filename, cl_uint device_id_count, cl_device_id *device_ids, std::string flags) {
  std::string source = load_file(filename);
  const char *srcs[] = { source.c_str() };
  cl_program program = clCreateProgramWithSource(ctx, 1, srcs, NULL, &err);
  CL_ERR(err);
  err = clBuildProgram(program, device_id_count, device_ids, flags.c_str(), NULL, NULL);
  if (err != CL_SUCCESS) {
    size_t tmp = 0;
    clGetProgramBuildInfo(program, device_ids[device_id_count-1], CL_PROGRAM_BUILD_LOG, 0, NULL, &tmp);
    std::string log(tmp, ' ');
    clGetProgramBuildInfo(program, device_ids[device_id_count-1], CL_PROGRAM_BUILD_LOG, tmp, const_cast<char*>(log.c_str()), NULL);
    std::cout << "Build error: " << log << std::endl;
    exit(1);
  }
  return program;
}

cl_command_queue create_command_queue(cl_context ctx, cl_device_id device_id) {
  cl_command_queue queue = clCreateCommandQueue(ctx, device_id, 0, &err);
  CL_ERR(err);
  return queue;
}

cl_kernel create_kernel(cl_context ctx, cl_program program, const char *name) {
  cl_kernel kernel = NULL;
  if (name == NULL) {
    cl_uint nkern = 0;
    CL_ERR(clCreateKernelsInProgram(program, 0, NULL, &nkern));
    if (nkern == 0) {
      CL_ERR(CL_INVALID_VALUE); // No kernels found in file
    }
    cl_kernel kernels[nkern];
    CL_ERR(clCreateKernelsInProgram(program, nkern, kernels, NULL));
    cl_uint i;
    for (i = 1; i < nkern; i++) {
      clReleaseKernel(kernels[i]);
    }
    kernel = kernels[0];
  } else {
    kernel = clCreateKernel(program, name, &err);
    CL_ERR(err);
  }
  return kernel;
}

void render_image(cl_context ctx, cl_command_queue queue, cl_kernel kernel, uint8_t *buffer, cl_ulong offsetx, cl_ulong offsety, cl_ulong width, cl_ulong height, cl_ulong totalwidth, cl_ulong totalheight, cl_double4 coordinateframe, cl_double4 color) {
  
  static const cl_image_format format = { CL_RGBA, CL_UNORM_INT8 };
  static const cl_image_desc img_desc = { CL_MEM_OBJECT_IMAGE2D, width, height, 0, 0, 0, 0, 0, 0, NULL };
  cl_mem output_img = clCreateImage(ctx, CL_MEM_WRITE_ONLY, &format, &img_desc, NULL, &err);
  CL_ERR(err);
  CL_ERR(clSetKernelArg(kernel, 0, sizeof(cl_mem), &output_img));

  cl_ulong4 arg = {{offsetx, offsety, totalwidth, totalheight}};
  CL_ERR(clSetKernelArg(kernel, 1, sizeof(cl_ulong4), &arg));

  CL_ERR(clSetKernelArg(kernel, 2, sizeof(cl_double4), &coordinateframe));
  CL_ERR(clSetKernelArg(kernel, 3, sizeof(cl_double4), &color));

  size_t work_offsets[] = {0, 0, 0};
  size_t work_sizes[] = {width, height, 0};
  CL_ERR(clEnqueueNDRangeKernel(queue, kernel, 2, work_offsets, work_sizes, NULL, 0, NULL, NULL));
    
  size_t origin[3] = {0, 0, 0};
  size_t region[3] = {width, height, 1};
  uint8_t *tmp = (uint8_t *)malloc(width * height * sizeof(uint8_t) * 4);
  if (!tmp) {
    std::cerr << "Failed to allocate tmp!" << std::endl;
  }
  CL_ERR(clEnqueueReadImage(queue, output_img, CL_TRUE, origin, region, 0, 0, tmp, 0, NULL, NULL));
  /*
  static cl_ulong counter = 0;
  char *tmpn = NULL;
  asprintf(&tmpn, "out%llu.png", counter++);
  save_png(tmpn, tmp, width, height);
  free(tmpn);
  */
  cl_ulong i;
  for (i = 0; i < height; i++) {
    memcpy(buffer + totalwidth * 4 * (offsety + i) + offsetx * 4, tmp + width * 4 * i, width*4);
  }
  free(tmp);
  CL_ERR(clReleaseMemObject(output_img));
}

void usage(const char *progname) {
  std::cout << progname << ", version " << VERSION << ", made by DanZimm" << std::endl << std::endl;
  std::cout << "usage: " << progname << " -f FILE.cl [-m MAX_TILE_SIZE] [-w WIDTH] [-h [HEIGHT]] [-o OUTFILE]" << std::endl;
  std::cout << "                   [-l LEFT] [-r RIGHT] [-b BOTTOM] [-t TOP]" << std::endl << std::endl;
  std::cout << "This program take in an OpenCL program which has a kernel that takes a 2d image for writing, a ulong4 of metadata and a coordinateframe" << std::endl;
  std::cout << "where the metadata is of the form {offsetx, offsety, width, height} where offsetx/y is the offset of the tile currently being rendered" << std::endl;
  std::cout << "(i.e. the pixel coordinate of the tile being rendered) and width/height are the total width/height of the entire image." << std::endl;
  std::cout << "This was originally created in order to generate fractals but can be used to generate any image that needs to be big." << std::endl << std::endl;
  std::cout << "  -m : Specifies the max width and height a tile rendered can be. Defaults to 500" << std::endl;
  std::cout << "  -w : Specifies the width of the image to generate" << std::endl;
  std::cout << "  -h : Specifies the height of the image to generate" << std::endl;
  std::cout << "  -f : Specifies the file containing the OpenCL kernel to use" << std::endl;
  std::cout << "  -o : Specifies the name of the output png" << std::endl;
  std::cout << "  -l : The left most `x' that will be rendered" << std::endl;
  std::cout << "  -r : The right most `x' that will be rendered" << std::endl;
  std::cout << "  -t : The top most `y' that will be rendered" << std::endl;
  std::cout << "  -b : The bottom most `y' that will be rendered" << std::endl;
  std::cout << "    The last 4 parameters set up the coordinate system for the image to be rendered." << std::endl;
  exit(0);
}

int main(int argc, char *const argv[]) {
  cl_ulong max_width = 500, max_height = 500;
  cl_ulong height = 1000;
  cl_ulong width = 1000;
  cl_double4 coordinateframe = {{-2.0, 2.0, -2.0, 2.0}};
  cl_double4 color = {{1.0, 1.0, 1.0, 1.0}};
  const char *file = NULL;
  std::string dir("");
  const char *outfile = "out.png";
  cl_ulong row = 0;
  cl_ulong col = 0;
  
  int ch;
  while ((ch = getopt(argc, argv, "m:w:h:f:o:l:r:t:b:c:")) != -1) {
    switch (ch) {
      case 'm':
        max_width = max_height = (cl_ulong)atoll(optarg);
        break;
      case 'w':
        width = (cl_ulong)atoll(optarg);
        break;
      case 'h':
        height = (cl_ulong)atoll(optarg);
        break;
      case 'f':
        file = optarg;
        break;
      case 'o':
        outfile = optarg;
        break;
      case 'l':
        coordinateframe.s[0] = atof(optarg);
        break;
      case 'r':
        coordinateframe.s[1] = atof(optarg);
        break;
      case 'b':
        coordinateframe.s[2] = atof(optarg);
        break;
      case 't':
        coordinateframe.s[3] = atof(optarg);
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
        color.s[0] = (long double)strtol(tmp, NULL, 16) / 255.0;
        tmp[0] = hex[2];
        tmp[1] = hex[3];
        color.s[1] = (long double)strtol(tmp, NULL, 16) / 255.0;
        tmp[0] = hex[4];
        tmp[1] = hex[5];
        color.s[2] = (long double)strtol(tmp, NULL, 16) / 255.0;
        color.s[3] = 1.0;
        } break;
      case ':':
        switch (optopt) {
          case 'h':
            usage(argv[0]);
            break;
          default:
            std::cerr << "Option: " << optopt << " needs an argument!" << std::endl;            break;
        }
      default:
        usage(argv[0]);
        break;
    };
  }
  argc -= optind;
  argv += optind;
  if (file == NULL) {
    if (argc > 0) {
      file = argv[0];
    } else {
      std::cerr << "ERR: Need kernel name for cl program" << std::endl;
      usage(argv[0]);
    }
  }
  
  std::string filename(file);
  size_t pos = filename.rfind("/");
  if (pos != std::string::npos) {
    dir = filename.substr(0, pos);
  }
  std::string flags = "-I " + (dir.size() == 0 ? "." : dir);
  
  cl_platform_id platform_id;
  cl_device_id *device_ids;
  cl_uint device_id_count;
  fetch_platform_device_ids(&platform_id, &device_id_count, &device_ids);
  cl_context ctx = create_context(platform_id, device_id_count, device_ids);
  cl_program program = create_program(ctx, file, device_id_count, device_ids, flags);
  cl_kernel kernel = create_kernel(ctx, program, NULL);
  
  cl_command_queue queue = create_command_queue(ctx, device_ids[device_id_count-1]);
  
  uint8_t *buffer = (uint8_t *)malloc(sizeof(uint8_t) * width * height * 4);
  if (buffer == NULL) {
    std::cerr << "Failed to allocate buffer!" << std::endl;
    return 1;
  }
  cl_ulong deltacol = max_width;
  cl_ulong deltarow = max_height;
  while (row < height) {
    cl_ulong newrow = row + deltarow;
    if (newrow >= height) {
      newrow = height;
    }
    while (col < width) {
      cl_ulong newcol = col + deltacol;
      if (newcol >= width) {
        newcol = width;
      }
      std::cout << "Rendering tile " << col << ", " << row << std::endl;
      render_image(ctx, queue, kernel, buffer, col, row, newcol - col, newrow - row, width, height, coordinateframe, color);
      col = newcol;
    }
    row = newrow;
    col = 0;
  }
  save_png(outfile, buffer, width, height);
  free(buffer);

  CL_ERR(clReleaseCommandQueue(queue));
  CL_ERR(clReleaseKernel(kernel));
  CL_ERR(clReleaseProgram(program));
  CL_ERR(clReleaseContext(ctx));
  free(device_ids);
  return 0;
}

