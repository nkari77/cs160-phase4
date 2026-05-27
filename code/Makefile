CXX      = g++
CXXFLAGS = -std=c++20 -O2 -pthread -Wall

main: main.cpp graph.h concurrent_queue.h loader.h
	$(CXX) $(CXXFLAGS) -o main main.cpp

clean:
	rm -f main