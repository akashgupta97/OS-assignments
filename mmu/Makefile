CXXFLAGS=-g # -g = debug, -O2 for optimized code CXXFLAGS for g++
CC=g++

mmu: mmu.cpp
	module load gcc-9.2; \
	$(CC) $(CXXFLAGS) -o mmu mmu.cpp

clean:
	rm -f mmu *~