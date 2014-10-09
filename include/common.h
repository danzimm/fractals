
#ifndef __common_h
#define __common_h

typedef struct {
  unsigned long maxiter;
  float frame[4];
  float escape;
  double color[4];
  bool first_run;
  double previous_coords[2048]; // up to 1024 (x,y) coords stored
} metadata;

typedef struct {
  double coord[2];
  uint8_t color[4];
} colored_point;

#endif

