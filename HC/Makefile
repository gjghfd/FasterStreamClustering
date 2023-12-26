CPPFLAGS = -Wall -O3 -std=c++11 -lm -w -mcmodel=medium -g
PROGRAMS = main 

all: $(PROGRAMS)

main:main.cpp  PCUSketch.h \
	params.h BOBHash.h CMSketch.h CUSketch.h  ASketch.h\
	cuckoo_counter.h ElasticSketch.h MVSketch.h
	g++ -o cuckoo main.cpp $(CPPFLAGS)

clean:
	rm -f *.o $(PROGRAMS)
