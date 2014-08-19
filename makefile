
CXX=clang++
CFLAGS=-Wall -Werror -std=c++11 -O3
fractalgen_LDFLAGS=-lpng
fg_LDFLAGS=-dynamiclib

.PHONY: all clean

all: fractalgen valtoval.fg valtologval.fg mandlebrot.fg

clean:
	rm -rf *.o
	rm -rf fractalgen
	rm -rf *.fg

fractalgen: fractal.cc.o fractalgen.cc.o
	$(CXX) $^ -o $@ $(fractalgen_LDFLAGS)

%.fg: %.cc
	$(CXX) $^ -o $@ $(CFLAGS) $(fg_LDFLAGS)

%.cc.o: %.cc
	$(CXX) -c $< -o $@ $(CFLAGS)


