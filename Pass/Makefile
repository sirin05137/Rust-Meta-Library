simple.opt.bc : RSPIsolation.so ./llvm_ll/multithread_simple2.ll
	opt -load ./RSPIsolation.so -rust-smart-pointer-isolation -enable-new-pm=0 ./llvm_ll/multithread_simple2.ll -o simple.opt.bc

RSPIsolation.o : RSPIsolation.cpp
	clang++ -c -fpic -fno-rtti `llvm-config --cppflags` RSPIsolation.cpp -o RSPIsolation.o

RSPIsolation.so : RSPIsolation.o
	clang++ -shared -o RSPIsolation.so RSPIsolation.o

clean : 
	rm *.o RSPIsolation.so *.opt.*

