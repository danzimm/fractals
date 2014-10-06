
#include <stdint.h>
#include "common.h"

extern "C" {

__device__
void processPixel(unsigned long *ii, double *magg, unsigned long maxiter, double escape, double2 coord);

__device__
void colorizePixel(double pixel[4], double mag, double escape, double i, double maxiter);

__global__
void genimage(uint8_t *pixels, size_t pitch, unsigned long width, unsigned long height, unsigned long yoffset, metadata *meta) {
  unsigned long maxiter = meta->maxiter;
  float4 frame = {meta->frame[0], meta->frame[1], meta->frame[2], meta->frame[3]};
  double escape = meta->escape;
  double color[4] = {meta->color[0], meta->color[1], meta->color[2], meta->color[3]};
  unsigned block_size = blockDim.x;
  uint2 location = {blockIdx.x*block_size, blockIdx.y*block_size + yoffset};
  ulong2 pixel_location = {threadIdx.x, threadIdx.y};
  ulong2 real_location = {location.x + pixel_location.x, location.y + pixel_location.y};
  if (real_location.x >= width || real_location.y >= height)
    return;
  
  double pixel[4] = {color[0], color[1], color[2], color[3]};

  double2 coordsize = {frame.y - frame.x, frame.w - frame.z};
  double2 dimgsize = {(double)width, (double)height};
  double2 dpos = {(double)real_location.x, (double)real_location.y};

  double2 coord = {frame.x + coordsize.x * dpos.x / dimgsize.x, frame.z + coordsize.y * (dimgsize.y-dpos.y) / dimgsize.y};
  unsigned long i = 0;
  double mag = 0.0;
  processPixel(&i, &mag, maxiter, escape, coord);
  
  colorizePixel(pixel, mag, escape, i, maxiter);

  uint8_t *row = (uint8_t *)((char *)pixels + (real_location.y - yoffset) * pitch);
  row[real_location.x * 4 + 0] = (uint8_t)(pixel[0] * 255.0);
  row[real_location.x * 4 + 1] = (uint8_t)(pixel[1] * 255.0);
  row[real_location.x * 4 + 2] = (uint8_t)(pixel[2] * 255.0);
  row[real_location.x * 4 + 3] = (uint8_t)(pixel[3] * 255.0);
}

}

