
CUDA_LIB=/usr/local/cuda/lib

NVCC=nvcc
CXX=clang++
CFLAGS=--compiler-options -Wall --compiler-options -Werror -DVERSION=0.1 --compiler-options -O3 --compiler-options -Iinclude/
fractalgen_LDFLAGS=-lpng -L$(CUDA_LIB) -lcuda

.PHONY: all clean kernels

.SUFFIXES:

all: fractalgen kernels

clean:
	rm -rf *.o
	rm -rf fractalgen
	rm -rf intermediates/*.ptx
	rm -rf kernels/*.ptx
	rm -rf include/ptx.h

fractalgen: fractalgen.cc.o
	$(NVCC) $(filter %.o,$^) -o $@ $(fractalgen_LDFLAGS)

include/ptx.h: intermediates/hdrgen.js intermediates/base_ptx.ptx intermediates/default_colorizer.ptx
	cd intermediates && node hdrgen.js ../include/

kernels: kernels/cube.ptx kernels/julia.ptx kernels/mandlebrot.ptx kernels/valtologval.ptx kernels/valtoval.ptx kernels/zimm.ptx

%.ptx: %.cu
	$(NVCC) -ptx $< -o $@ $(CFLAGS)

fractalgen.cc.o: fractalgen.cc include/ptx.h
	$(NVCC) -c $< -o $@ $(CFLAGS)

%.cu.o: %.cu
	$(NVCC) -c $< -o $@ $(CFLAGS)

%.cc.o: %.cc
	$(CXX) -c $< -o $@ $(CFLAGS)

