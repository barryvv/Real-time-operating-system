#include "rtx.h"
#include "rtx_errno.h"
#include "k_mem.h"



void   *mem_alloc(size_t size){
	if(size == 0){
		return NULL;
	} else if (size != 0 && size > 0){
		void* tmp = k_mpool_alloc(MPID_IRAM1, size);
		if(tmp == NULL){
			//bad address
			errno = EFAULT;
			return NULL;
		} else {
			return tmp;
		}
	} else {
		return NULL;
	}
}

__svc(SVC_MEM2_ALLOC) void *mem2_alloc(size_t size){
	if(size == 0){
		return NULL;
	} else if (size != 0 && size > 0){
		void* tmp = k_mpool_alloc(MPID_IRAM2, size);
		if(tmp == NULL){
			//bad address
			errno = EFAULT;
			return NULL;
		} else {
			return tmp;
		}
	} else {
		return NULL;
	}
}

//no success return in k_mpool_dealloc???
__svc(SVC_MEM_DEALLOC) int mem_dealloc(void* ptr){
	//ptr = NULL
	if(ptr == NULL){
		return -1;
	} else if(ptr > (void*) RAM1_START && ptr < (void*) RAM1_END){
		int returnValue = k_mpool_dealloc(MPID_IRAM1, ptr);
		if(returnValue == -1){
			errno = RTX_ERR;
			return -1;
		} else {
			//success
			return 0;
		}
	} else {
		return -1;
	}
}

__svc(SVC_MEM2_DEALLOC) int mem2_dealloc(void* ptr){
	//ptr = NULL
	if(ptr == NULL){
		return -1;
	} else if(ptr > (void*) RAM2_START && ptr < (void*) RAM2_END){
		int returnValue = k_mpool_dealloc(MPID_IRAM2, ptr);
		if(returnValue == -1){
			errno = RTX_ERR;
			return -1;
		} else {
			//success
			return 0;
		}
	} else {
		return -1;
	}
}



__svc(SVC_MEM_DUMP) int mem_dump(void){
	int returnValue = k_mpool_dump(MPID_IRAM1);
	if (returnValue == 0){
		errno = EFAULT;
		return 0;
	} else {
		//how many free list
		return returnValue;
	}
}

__svc(SVC_MEM2_DUMP) int mem2_dump(void){
	int returnValue = k_mpool_dump(MPID_IRAM2);
	if (returnValue == 0){
		errno = EFAULT;
		return 0;
	} else {
		//how many free list
		return returnValue;
	}
}
