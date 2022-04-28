CXXFLAGS=-g # -g = debug, -O2 for optimized code CXXFLAGS for g++
CC=g++

iosched: iosched.cpp
	module load gcc-9.2; \
	$(CC) $(CXXFLAGS) -o iosched iosched.cpp

clean:
	rm -f iosched *~