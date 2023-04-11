librsp.so: rsp.c
	gcc -Wall -DRUNTIME -shared -fpic -ffixed-r15 -o librsp.so rsp.c -lpthread -L/usr/local/lib/ -lmimalloc

clean:
	rm *.so
