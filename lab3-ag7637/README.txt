#please load gcc-9.2 module before running make
-> module load gcc-9.2

#then

->make

#a mmu executable will be built

To run the executable do,

->./mmu -f16 -aw -oOPFS <input_file> <rfile>