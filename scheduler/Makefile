CXXFLAGS=-g # -g = debug, -O2 for optimized code CXXFLAGS for g++
CC=g++

sched: sched.cpp
	module load gcc-9.2; \
	$(CC) $(CXXFLAGS) -o sched sched.cpp

clean:
	rm -f sched *~