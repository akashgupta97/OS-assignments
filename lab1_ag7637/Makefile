CXXFLAGS=-g # -g = debug, -O2 for optimized code CXXFLAGS for g++
CC=g++

linker: linker.cpp
	module load gcc-9.2; \
	$(CC) $(CXXFLAGS) -o linker linker.cpp

clean:
	rm -f linker *~