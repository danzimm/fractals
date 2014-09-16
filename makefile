
CXX=clang++
CFLAGS=-Wall -Werror -std=c++11 -O3
fractalgen_LDFLAGS=-lpng
fg_LDFLAGS=-dynamiclib

.PHONY: all clean sc

all: fractalgen valtoval.fg valtologval.fg mandlebrot.fg julia.fg mandlebrotclock.fg

clean:
	rm -rf *.o
	rm -rf fractalgen
	rm -rf *.fg

sc:
	rm -rf fractal.cc.o fractalgen.cc.o

fractalgen: fractal.cc.o fractalgen.cc.o fractal.hpp
	$(CXX) $(filter %.o,$^) -o $@ $(fractalgen_LDFLAGS)

%.fg: %.cc
	$(CXX) $^ -o $@ $(CFLAGS) $(fg_LDFLAGS)

%.cc.o: %.cc
	$(CXX) -c $< -o $@ $(CFLAGS)


