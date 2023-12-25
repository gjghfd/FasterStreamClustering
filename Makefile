all: main

main: main.cpp util.cpp common.h template.h template.cpp streamKMpp.h streamKMpp.cpp
	g++ -std=c++11 -O2 util.cpp template.cpp streamKMpp.cpp main.cpp -o main

clean:
	rm main