librsp.so: rsp.c
	gcc -Wall -DRUNTIME -shared -fpic -ffixed-r15 -o librsp.so rsp.c -lpthread

clean:
	rm *.so
