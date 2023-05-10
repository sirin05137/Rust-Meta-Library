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
//static pthread_once_t HOOKING_INITIALIZATION = PTHREAD_ONCE_INIT;

struct ExternStackPointer
{
	void *pure_ptr;
	void *housed_ptr;
};


__thread void* extern_stack_ptr = NULL;
__thread void* smallest_addr_used = NULL;

void *__get_extern_stack_ptr();
void* thread_function_hooking(void*);
void MEM2GS(void* test);

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
    
	MEM2GS(extern_sp);	
	
	//arch_prctl(ARCH_SET_GS, (uintptr_t)extern_sp);
	
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
	printf("extern stack pointer : %p\n\n", extern_stack_ptr);
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

/*void MEM2GS(void* test){

	
	//unsigned val = getauxval(AT_HWCAP2);
	//if (val & HWCAP2_FSGSBASE){
	//	printf("MEM2GS\n");
	if ((uint64_t)test < (uint64_t)smallest_addr_used){
		smallest_addr_used = test;
	}
	uintptr_t ptrToInt = (uintptr_t)test;
	//asm("wrgsbase %0"::"r"(ptrToInt));
	arch_prctl(ARCH_SET_GS, ptrToInt);

	uint64_t temp;
	asm("movq %%gs:%c[offset], %0" : "=r" (temp): [offset] "i"(0));	
	printf("1 : 0x%lx\n", temp);
	uintptr_t a;//=_readgsbase_u64();
	arch_prctl(ARCH_GET_GS, &a);
	printf("2 : 0x%lx\n", a);
	
	//_writegsbase_u64(ptrToInt);
	//}
	//asm("movq %0,%%gs:%c[offset]" : : "r" (test), [offset] "i" (0));
}*/
/*
void MEM2GS(void* test){
	if ((uint64_t)test < (uint64_t)smallest_addr_used){
		smallest_addr_used = test;
	}
	//void *temp;
	uint64_t tttt = (uint64_t)test;
	uintptr_t a;
	arch_prctl(ARCH_GET_FS, &a);
	printf("aaa = 0x%lx\n", a);
	void* temp=(void*)(a+56);// = mmap((void*)(a+160), 8, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	//printf("temp = %p\n", temp);
	*(uint64_t*)temp = tttt;
	//memcpy(temp, &tttt, 8);
	printf("temp = %p\n", temp);
	// *(uint64_t*)(a+16) =(uint64_t)test; 
	//munmap((void*)(a+160), 8);
	//asm("movq %0, %%gs:%c[offset]" ::"r" ((uint64_t)test), [offset] "i"(16));	
}
*/
void MEM2GS(void* test){
	if ((uint64_t)test < (uint64_t)smallest_addr_used){
		smallest_addr_used = test;
	}	
	
	asm("movq %0, %%fs:%c[offset]" ::"r" ((uint64_t)test), [offset] "i"(56));	

}




/*void* GS2MEM(){
	//unsigned val = getauxval(AT_HWCAP2);
	void *temp;
	uintptr_t a;//=_readgsbase_u64();
	arch_prctl(ARCH_GET_GS, &a);
	//if (val & HWCAP2_FSGSBASE){
	//	printf("GS2MEM\n");
	//asm("rdgsbase %0":"=r"(a));//}
	temp = (void*)a;
	//asm("movq %%gs:%c[offset], %0" : "=r" (temp) :[offset] "i" (0));
	return temp;
}*/

/*
void* GS2MEM(){
	uint64_t ttttt;
	uint64_t a;
	arch_prctl(ARCH_GET_FS, &a);
	printf("a = 0x%lx\n",a);
	//a+=16;
	void* temp =(void*)(a+56);// = mmap((void*)(a+160), 8, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	//uint64_t* b = (uint64_t*)a;
	//printf("temptwmp = %p\n", temp);
	//memcpy(&ttttt, temp, 8);
	uint64_t* temp2 = (uint64_t*) temp;
	ttttt = *temp2; 


	//uint64_t b = *(uint64_t*)(a+16);
	printf("ttttt = %p\n", (void*)ttttt);
	//munmap((void*)(a+160), 8);
	return (void*)ttttt;
	//uint64_t temp;
	//asm("movq %%gs:%c[offset], %0" : "=r" (temp): [offset] "i"(16));
	//printf("0x%lx\n", temp);
	//return (void*)temp;
}*/

void* GS2MEM(){
	uint64_t temp;
	asm("movq %%fs:%c[offset], %0" : "=r" (temp) :[offset] "i" (56));
	return (void*)temp;
}
