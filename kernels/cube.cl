
#include "complex.cl"
#include "colors.cl"

__kernel void mandlebrot(__write_only image2d_t output, __read_only ulong4 metadata, double4 frame, double4 col) {

  const int2 pos = {get_global_id(0), get_global_id(1)};
  double2 coordsize = {frame.y - frame.x, frame.w - frame.z};
  int2 imgsize = {metadata.z, metadata.w};//{get_image_width(output), get_image_height(output)};
  double2 dimgsize = convert_double2(imgsize);
  double2 dpos = convert_double2(pos);
  dpos.y += (double)(metadata.y);
  dpos.x += (double)(metadata.x);
  ulong maxiter = 100;

  double2 coord = {frame.x + coordsize.x * dpos.x / dimgsize.x, frame.z + coordsize.y * (dimgsize.y-dpos.y) / dimgsize.y};

  double2 val = coord;
  ulong i = 0;
  while (i < maxiter) {
    val = (val - complex_divide(complex_pown(val, 3) - (double2)(1.0, 0.0), 3 * complex_pown(val, 2)));
    if (complex_mag2(val) >= 3.0) {
      break;
    }
    i++;
  }
  double4 dcol = color_darkener_o(i, maxiter, complex_mag2(val), 3.0, -0.1) * col;
  dcol.w = 1.0;

  write_imagef(output, pos, convert_float4_sat(dcol));
}

