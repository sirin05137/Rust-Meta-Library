#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <pthread.h>
#include <dlfcn.h>

#define PTHREAD_HOOKING_ERROR \
  fprintf(stderr, "Unable to create pthread library hooks\n"); \
  abort(); 

#define DEFAULT_STACK_SIZE ((size_t)0x8000) // 4096*8 byte

typedef int (*pthread_create_t)(pthread_t* restrict, const pthread_attr_t* restrict, void*(*)(void*), void* restrict);
pthread_create_t real_pthread_create = 0;
//static pthread_once_t HOOKING_INITIALIZATION = PTHREAD_ONCE_INIT;

__thread void* extern_stack_ptr = NULL;
__thread void* smallest_addr_used = NULL;

void *__get_extern_stack_ptr();
void* thread_function_hooking(void*);

typedef struct Argument{
	void* (*function)(void*);
	void* args;
}Argument_t;

int pthread_create(pthread_t *restrict thread, 
				   const pthread_attr_t *restrict attr, 
				   void *(*routine)(void*), 
				   void *restrict arg){
	
	Argument_t *temp = malloc(sizeof(Argument_t));
	temp->function = routine;
	temp->args = arg;
	real_pthread_create = dlsym(RTLD_NEXT, "pthread_create");
  	if(!real_pthread_create){
   		PTHREAD_HOOKING_ERROR
  	}
	return real_pthread_create(thread, attr, thread_function_hooking, temp);
}

void* thread_function_hooking(void* args){
	void* extern_sp = __get_extern_stack_ptr();
	//void* extern_sp = __allocate_extern_stack(DEFAULT_STACK_SIZE);
	Argument_t *argument = (Argument_t*) args;
	//asm("mov %0, %%r15;"::"r" (extern_sp):"%r15");

	void *retval = argument->function(argument->args);

	uint64_t used_stack_size = (uint64_t)((char*)extern_sp - (char*)smallest_addr_used);
	int num_page = used_stack_size/4096;
	if(num_page < 8)
		num_page = 8;

	else if(used_stack_size%4096)
		num_page = 1 + num_page;

	if(munmap((void*)((char*)extern_sp-num_page*4096), num_page*4096)==-1){
		//printf("%d\n", num_page);
		printf("Unable to release the extern stack\n");
	}
	free(argument);
	return retval;
}

void *__allocate_extern_stack(size_t size){
	extern_stack_ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_GROWSDOWN, -1, 0);
	//extern_stack_ptr = mi_malloc(size);
	extern_stack_ptr = (void*)((char*)extern_stack_ptr + size);
	smallest_addr_used = extern_stack_ptr;
	
	return extern_stack_ptr;
}

void *__get_extern_stack_ptr(){
	if(!extern_stack_ptr){
		//assert?
		__allocate_extern_stack(DEFAULT_STACK_SIZE);
	}
	//printf("extern stack pointer : %p\n\n", extern_stack_ptr);
	return extern_stack_ptr;
}

void smallest_address_used(){
	if ((uint64_t)extern_stack_ptr < (uint64_t)smallest_addr_used){
		smallest_addr_used = extern_stack_ptr;
	}
}

void* register_2_memory(void* static_top){
	extern_stack_ptr = static_top;
	smallest_address_used();
	return extern_stack_ptr;
	//asm("mov %%r15, %0;":"=r"(extern_stack_ptr)::);
}

