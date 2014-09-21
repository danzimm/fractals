
CXX=clang++
CFLAGS=-Wall -Werror -std=c++1y -O3
fractalgen_LDFLAGS=-lpng -framework OpenCL

.PHONY: all clean

all: fractalgen

clean:
	rm -rf *.o
	rm -rf fractalgen

fractalgen: fractalgen.cc.o
	$(CXX) $(filter %.o,$^) -o $@ $(fractalgen_LDFLAGS)

%.cc.o: %.cc
	$(CXX) -c $< -o $@ $(CFLAGS)


