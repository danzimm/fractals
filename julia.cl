
#include "complex.cl"

__kernel void julia(__write_only image2d_t output, __read_only ulong4 metadata, double4 frame, double4 col) {

  const int2 pos = {get_global_id(0), get_global_id(1)};
  double2 coordsize = {frame.y - frame.x, frame.w - frame.z};
  int2 imgsize = {metadata.z, metadata.w};
  double2 dimgsize = convert_double2(imgsize);
  double2 dpos = convert_double2(pos);
  dpos.y += (double)(metadata.y);
  dpos.x += (double)(metadata.x);
  ulong maxiter = 10000;

  double2 coord = {frame.x + coordsize.x * dpos.x / dimgsize.x, frame.z + coordsize.y * (dimgsize.y-dpos.y) / dimgsize.y};
  double2 c = {-0.8, 0.156};

  double2 val = coord;
  double2 tmp;
  ulong i = 0;
  while (i < maxiter) {
    val = complex_pown(val, 3) + c;
    if (complex_mag2(val) >= 4) {
      break;
    }
    i++;
  }
  double inp = (double)(i) / (double)(maxiter);
  double darkener = 1 - pown(1 - inp, 500);
  double4 dcol;
  if (i < maxiter) {
    dcol = (4.0 / complex_mag2(val)) * col;
    //dcol = (darkener * col);
  } else {
    dcol = (double4)(0.0);
  }
  dcol.w = 1.0;

  write_imagef(output, pos, convert_float4_sat(dcol));
}

