#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <pthread.h>
#include "/usr/local/include/mimalloc-2.0/mimalloc.h"
#include <dlfcn.h>


#define PTHREAD_HOOKING_ERROR \
  fprintf(stderr, "Unable to create pthread library hooks\n"); \
  abort(); 

#define DEFAULT_STACK_SIZE ((size_t)0x10000) // 64KB



typedef int (*pthread_create_t)(pthread_t* restrict, const pthread_attr_t* restrict, void*(*)(void*), void* restrict);
pthread_create_t real_pthread_create = 0;
static pthread_once_t HOOKING_INITIALIZATION = PTHREAD_ONCE_INIT;

void *__get_extern_stack_ptr();

typedef struct Argument{
	void* function;
	void* args;
}Argument_t;

void init_threading_hooks(){
  real_pthread_create = dlsym(RTLD_NEXT, "pthread_create");
  if(!real_pthread_create){
    PTHREAD_HOOKING_ERROR
  }
}

void ensure_initialized(){
    pthread_once(&HOOKING_INITIALIZATION, init_threading_hooks);
}

int pthread_create(pthread_t *restrict thread, 
				   const pthread_attr_t *restrict attr, 
				   void *(*routine)(void*), 
				   void *restrict arg){
	
	Argument_t temp;
	temp.function = (void*) routine;
	temp.args = arg;
	//printf("test");
	return real_pthread_create(thread, attr, thread_function_hooking, &temp);
}

void* thread_function_hooking(void* args){
	void* extern_sp = __get_extern_stack_ptr();
	Argument_t argument = *(Argument_t*) args;
	void* (*origin_function)(void*) = argument.function;
	void* origin_args = argument.args;
	asm("mov %0, %%r15;"::"r" (extern_sp):"%r15");
	return origin_function(origin_args);
}

void *__allocate_extern_stack(size_t size)
{
	void *extern_stack_ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS | MAP_GROWSDOWN, -1, 0);
	//void *extern_stack_ptr = mi_malloc(size);
	return extern_stack_ptr;// + size;
}

void *__get_extern_stack_ptr()
{
	ensure_initialized();
	//printf("how much\n");
	void* extern_stack_pointer = __allocate_extern_stack(DEFAULT_STACK_SIZE);	
	printf("extern stack pointer : %p\n\n", extern_stack_pointer);
	//printf("default heap address : %p\n\n", mi_get_default_heap());
	return extern_stack_pointer;
}
