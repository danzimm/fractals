
#include <stdint.h>
#include <curand_kernel.h>
#include "common.h"
#include "complex.cu"

extern "C" {

__device__
double2 fetch_initial_point(unsigned long i);

__device__
double2 iterate_point(double2 val, unsigned long i, double2 ipnt, unsigned long func_n);

__global__
void initrng(curandState *const rng_states, const unsigned int seed) {
  unsigned int tid = blockIdx.x * blockDim.x + threadIdx.x;
  curand_init(seed, tid, 0, &rng_states[tid]);
}

__global__
void gencoord(colored_point *points, unsigned long npnts, metadata *meta, curandState *const rng_states) {
  unsigned tid = blockIdx.x * blockDim.x + threadIdx.x;
  curandState localState = rng_states[tid];
  double2 initial_point, val;
  unsigned long i = 0, ai = 2 * npnts * tid + 2 * i, func_n = 0;
  if (meta->first_run) {
    initial_point = fetch_initial_point(tid);
  } else {
    initial_point = (double2){meta->previous_coords[2 * tid + 0], meta->previous_coords[2 * tid + 1]};
  }
  val = initial_point;
  
  while (i < npnts) {
    func_n = curand(&localState);
    points[ai].coord[0] = val.x;
    points[ai].coord[1] = val.y;
    points[ai].color[0] = 0;
    points[ai].color[1] = 255;
    points[ai].color[2] = 0;
    points[ai].color[3] = 255;
    val = iterate_point(val, i, initial_point, func_n);
    i++;
    ai = npnts * tid + i;
  }
  meta->previous_coords[2 * tid + 0] = val.x;
  meta->previous_coords[2 * tid + 1] = val.y;
}

}

