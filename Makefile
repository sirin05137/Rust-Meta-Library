librsp.so: rsp.c
	gcc -Wall -DRUNTIME -shared -fpic -o librsp.so rsp.c -lpthread

clean:
	rm *.so
