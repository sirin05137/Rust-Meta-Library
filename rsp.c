#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <pthread.h>
#include "/usr/local/include/mimalloc-2.0/mimalloc.h"
#include <dlfcn.h>

#define PTHREAD_HOOKING_ERROR \
  fprintf(stderr, "Unable to create pthread library hooks\n"); \
  abort(); 

#define DEFAULT_STACK_SIZE ((size_t)0x8000) // 4096*8 byte

typedef int (*pthread_create_t)(pthread_t* restrict, const pthread_attr_t* restrict, void*(*)(void*), void* restrict);
pthread_create_t real_pthread_create = 0;
//static pthread_once_t HOOKING_INITIALIZATION = PTHREAD_ONCE_INIT;

__thread void* extern_stack_ptr = NULL;

void *__get_extern_stack_ptr();
void* thread_function_hooking(void*);

typedef struct Argument{
	void* function;
	void* args;
}Argument_t;

int pthread_create(pthread_t *restrict thread, 
				   const pthread_attr_t *restrict attr, 
				   void *(*routine)(void*), 
				   void *restrict arg){
	
	Argument_t temp;
	temp.function = (void*) routine;
	temp.args = arg;
	real_pthread_create = dlsym(RTLD_NEXT, "pthread_create");
  	if(!real_pthread_create){
   		PTHREAD_HOOKING_ERROR
  	}
	//printf("test");
	return real_pthread_create(thread, attr, thread_function_hooking, &temp);
}

void* thread_function_hooking(void* args){
	void* extern_sp = __get_extern_stack_ptr();
	Argument_t argument = *(Argument_t*) args;
	void* (*origin_function)(void*) = argument.function;
	void* origin_args = argument.args;
	asm("mov %0, %%r15;"::"r" (extern_sp):"%r15");
	
	void *retval = origin_function(origin_args);

	/*void* temp_addr;
	asm("mov %%r15, %0;":"=r" (temp_addr)::);
	uint64_t used_stack_size = (uint64_t)((char*)extern_sp - (char*)temp_addr);
	//printf("extern stack ptr : %p\n", extern_sp);
	//printf("temp addr : %p\n", temp_addr);
	//printf("used stack size : %ld\n", used_stack_size);
	int num_page = used_stack_size/4096;
	if(num_page < 8)
		num_page =8;

	if(used_stack_size%4096)
		num_page = 1+ num_page;

	if(munmap((void*)((char*)extern_sp-num_page*4096), num_page*4096)==-1){
		//printf("%d\n", num_page);
		printf("Unable to release the extern stack\n");
	}*/
	if(munmap(extern_sp-DEFAULT_STACK_SIZE, DEFAULT_STACK_SIZE)==-1){
		printf("Unable to release the extern stack\n");
	}
	//mi_free(extern_sp-DEFAULT_STACK_SIZE);
	return retval;
}

void *__allocate_extern_stack(size_t size){
	extern_stack_ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_GROWSDOWN, -1, 0);
	//extern_stack_ptr = mi_malloc(size);
	extern_stack_ptr = (void*)((char*)extern_stack_ptr + size);
	
	return extern_stack_ptr;
}

void *__get_extern_stack_ptr(){
	if(!extern_stack_ptr){
		__allocate_extern_stack(DEFAULT_STACK_SIZE);
	}
	printf("extern stack pointer : %p\n\n", extern_stack_ptr);
	return extern_stack_ptr;
}
