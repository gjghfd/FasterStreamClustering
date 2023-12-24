all: main kmeans

main: main.cpp util.cpp common.h template.h template.cpp
	g++ -std=c++11 -O2 util.cpp template.cpp main.cpp -o main

kmeans: kmeans.cpp
	g++ -std=c++11 -O2 kmeans.cpp -o kmeans

clean:
	rm main
	rm kmeans