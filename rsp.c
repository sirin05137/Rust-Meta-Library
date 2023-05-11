#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <pthread.h>
#include <dlfcn.h>
//#include <sys/auxv.h>
//#include <elf.h>
//#include <asm/hwcap2.h>
//#include <immintrin.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <asm/prctl.h>
#include <string.h>

#define PTHREAD_HOOKING_ERROR \
  fprintf(stderr, "Unable to create pthread library hooks\n"); \
  abort(); 

#define DEFAULT_STACK_SIZE ((size_t)0x8000) // 4096*8 byte

typedef int (*pthread_create_t)(pthread_t* restrict, const pthread_attr_t* restrict, void*(*)(void*), void* restrict);
pthread_create_t real_pthread_create = 0;
//pthread_key_t WRAPPER_KEY;
//static pthread_once_t HOOKING_INITIALIZATION = PTHREAD_ONCE_INIT;

typedef struct Wrapper
{
	void *pure_ptr;
	void *housed_ptr;
	void *pure_end;
	void *housed_end;
}Wrapper_t;

__thread Wrapper_t* wrapper = NULL;
//__thread void* extern_stack_ptr = NULL;
__thread void* smallest_addr_used = NULL;

int pthread_create(pthread_t *restrict thread, 
				   const pthread_attr_t *restrict attr, 
				   void *(*routine)(void*), 
				   void *restrict arg);
void* thread_function_hooking(void*);
void __allocate_extern_stack(size_t size);
void *__get_wrapper();
void MEM2FS(void* test);
void *FS2MEM();

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
	void* t = __get_wrapper();
	Wrapper_t *extern_sp = (Wrapper_t*) t;
	//void* extern_sp = __allocate_extern_stack(DEFAULT_STACK_SIZE);
	Argument_t *argument = (Argument_t*) args;
	//asm("mov %0, %%r15;"::"r" (extern_sp):"%r15");
    
	MEM2FS(extern_sp);	
	
	//arch_prctl(ARCH_SET_GS, (uintptr_t)extern_sp);
	
	void *retval = argument->function(argument->args);

	/*uint64_t used_stack_size = (uint64_t)((char*)(extern_sp->pure_ptr) - (char*)smallest_addr_used);
	int num_page = used_stack_size/4096;
	if(num_page < 8)
		num_page = 8;

	else if(used_stack_size%4096)
		num_page = 1 + num_page;

	if(munmap((void*)((char*)(extern_sp->pure_ptr)-num_page*4096), num_page*4096)==-1){
		//printf("%d\n", num_page);
		printf("Unable to release the extern stack\n");
	}*/
	free(argument);
	free(extern_sp->pure_end);
	free(extern_sp->housed_end);
	free(extern_sp);

	return retval;
}

void __allocate_extern_stack(size_t size){
	//wrapper->pure_ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_GROWSDOWN, -1, 0);
	wrapper->pure_end = malloc(size);
	wrapper->housed_end = malloc(size);
	
	//extern_stack_ptr = mi_malloc(size);
	wrapper->pure_ptr = (void*)((char*)(wrapper->pure_end) + size);
	wrapper->housed_ptr = (void*)((char*)(wrapper->housed_end) + size);
	smallest_addr_used = wrapper->pure_ptr;
}

void *__get_wrapper(){
	if(!wrapper){
		wrapper = malloc(sizeof(Wrapper_t));
		__allocate_extern_stack(DEFAULT_STACK_SIZE);
	}
	printf("wrapper    pointer   : %p\n\n", wrapper);
	printf("pure stack pointer   : %p\n\n", wrapper->pure_ptr);
	printf("housed stack pointer : %p\n\n", wrapper->housed_ptr);
	return wrapper;
}

void smallest_address_used(){
	if ((uint64_t)wrapper->pure_ptr < (uint64_t)smallest_addr_used){
		smallest_addr_used = wrapper->pure_ptr;
	}
}

void MEM2FS(void* test){
	/*if ((uint64_t)test < (uint64_t)smallest_addr_used){
		smallest_addr_used = test;
	}*/
	
	asm("movq %0, %%fs:%c[offset]" ::"r" ((uint64_t)test), [offset] "i"(56));	
}

void* FS2MEM(){
	uint64_t temp;
	asm("movq %%fs:%c[offset], %0" : "=r" (temp) :[offset] "i" (56));
	return (void*)temp;
}
