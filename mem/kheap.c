#include "kheap.h"

#include <inc/memlayout.h>
#include <inc/dynamic_allocator.h>
#include "memory_manager.h"
int initialize_kheap_dynamic_allocator(uint32 daStart, uint32 initSizeToAllocate, uint32 daLimit)
{
	startOfHeap=daStart;
	breakOfHeap = startOfHeap + initSizeToAllocate;
	//hardLimitOfHeap = daLimit;
	breakOfHeap = ROUNDUP(breakOfHeap,PAGE_SIZE);
	hardLimitOfHeap = ROUNDUP(daLimit,PAGE_SIZE);
	if(breakOfHeap > daLimit){
		return E_NO_MEM;
	}
	   for(uint32 i=startOfHeap ; i<breakOfHeap ; i+=PAGE_SIZE){
		    //check if the page is exist or not
		    uint32 *ptr_table=NULL;
			struct FrameInfo *ptr_frame_info;//do nothing
	         //allocate new frame
			int ret=allocate_frame(&ptr_frame_info);
			ptr_frame_info->actualva = i;
			//cprintf("%x \n",i);
			if(ret==0){
				int ret1=map_frame(ptr_page_directory,ptr_frame_info,i,PERM_WRITEABLE|PERM_PRESENT);
							if(ret1==0)
								continue;
							else{
								return E_NO_MEM;
							}

			}
			else
				return E_NO_MEM;
			//map the daStart to the allocated frame
    	}
	   initialize_dynamic_allocator(daStart,initSizeToAllocate);
	   return 0;
}

void* sbrk(int increment)
{
	//TODO: [PROJECT'23.MS2 - #02] [1] KERNEL HEAP - sbrk()

	//MS2: COMMENT THIS LINE BEFORE START CODING====
	if(increment==0){
		return (void *)breakOfHeap;
	}
	if(increment > 0){
		uint32 newbreak = ROUNDUP((breakOfHeap+increment),PAGE_SIZE);
		if(newbreak > hardLimitOfHeap){
			panic("break exceeds hard limit");
			return (void*)-1 ;
		}
		uint32 oldbreak = breakOfHeap;
		breakOfHeap = newbreak;
		for(int i=oldbreak;i<breakOfHeap; i+= PAGE_SIZE){
			 uint32 *ptr_table=NULL;
						struct FrameInfo *ptr_frame_info=get_frame_info(ptr_page_directory,i,&ptr_table);
						if(ptr_frame_info!=NULL)
							continue;//do nothing

				         //allocate new frame
						int ret=allocate_frame(&ptr_frame_info);
						ptr_frame_info->actualva=i;
						if(ret==0){
							int ret1=map_frame(ptr_page_directory,ptr_frame_info,i,PERM_WRITEABLE);
						}
						else{
							panic("kernel ran out of memory \n");
							return (void*)-1 ;
						}
		}
		return (void *)oldbreak;
	}
	else if (increment < 0){
		uint32 newbreak  = breakOfHeap+increment;
		uint32 round = ROUNDUP(newbreak,PAGE_SIZE);
		for(uint32 i = round ; i< breakOfHeap ; i+= PAGE_SIZE){
			uint32 * ptr_table = NULL;
			struct FrameInfo *ptr_frame_info = get_frame_info(ptr_page_directory,i,&ptr_table);
			free_frame(ptr_frame_info);
			unmap_frame(ptr_page_directory,i);
		}
		breakOfHeap = newbreak;
		return (void *)breakOfHeap;
	}
	return (void*)-1 ;
	//panic("not implemented yet");
}


void* kmalloc(unsigned int size)
{
	uint32 startofpagealloc = hardLimitOfHeap + PAGE_SIZE;
	void* allocAddress;
	bool found =1;
	if(isKHeapPlacementStrategyFIRSTFIT() == 1){
		if(size <= DYN_ALLOC_MAX_BLOCK_SIZE){

			return alloc_block_FF(size);
		}

		size = ROUNDUP(size,PAGE_SIZE);
        if((size) > (KERNEL_HEAP_MAX-startofpagealloc)){
        	return NULL;
        }
        int itisfree=0;
        //cprintf("size in kmalloc is %d \n",numofpages);
        for(uint32 i = startofpagealloc ; i<KERNEL_HEAP_MAX ; i+= PAGE_SIZE){

        	found =1;
        	uint32 *ptr_table=NULL;
        	itisfree=0;
        	if((i+size) >=KERNEL_HEAP_MAX){
        		return NULL;
        	}
        	if(get_frame_info(ptr_page_directory,i,&ptr_table) == NULL && (i+size < KERNEL_HEAP_MAX)){
        		itisfree=1;
        		for(uint32 j=i; j<i+size; j+=PAGE_SIZE){
        			if(get_frame_info(ptr_page_directory,j,&ptr_table) != 0){
        				found =0;
        				i = j;
        				break;
        			}
        		}
        		if(found == 1 && (i+size < KERNEL_HEAP_MAX) && itisfree==1){
        			allocAddress = (void *)i;
        			for(uint32 j=i; j<i+size; j+=PAGE_SIZE){
        				struct FrameInfo *ptr_frame_info=NULL;
        				int ret=allocate_frame(&ptr_frame_info);
        				if(j==i){
        					//cprintf("the frame in kmalloc is %p \n",ptr_frame_info);
        				}
        				ptr_frame_info->va = i;
        				ptr_frame_info->size = size;
        				//if(j==i)
        				//cprintf("the size in size is %d \n",ptr_frame_info->size);
        				if(ret==0){
        					ptr_frame_info->actualva = j;
        					int ret1=map_frame(ptr_page_directory,ptr_frame_info,j, (PERM_WRITEABLE & ~(PERM_USER)));
        				}
        				else{
        					panic("cannot allocate frame \n");
        				}
        		   }
        			return allocAddress;
        	}
        }
	}
	//TODO: [PROJECT'23.MS2 - #03] [1] KERNEL HEAP - kmalloc()
}
	return NULL;
}
void kfree(void* virtual_address)
{

     if ((uint32)virtual_address < breakOfHeap)
     {

		free_block(virtual_address);

     }
     else if ((uint32)virtual_address >= (hardLimitOfHeap+PAGE_SIZE) && (uint32)virtual_address < KERNEL_HEAP_MAX)
     {
    uint32 * ptr_page_table = NULL;
       for(uint32 i = (uint32)virtual_address; i< KERNEL_HEAP_MAX; i+= PAGE_SIZE){
    	   struct FrameInfo * ptr_frame = get_frame_info(ptr_page_directory,i,&ptr_page_table);
    	   if(ptr_frame != NULL && ptr_frame->va == (uint32)virtual_address){
    		   free_frame(ptr_frame);
    		   ptr_frame->actualva = 0;
    		   ptr_frame->va =0;
    		   unmap_frame(ptr_page_directory,i);
    	   }
    	   else{
    		   break;
    	   }
       }
     }
	else
	{
		panic("invalid address");
		//panic("kfree() is not implemented yet...!!");
	}


	//TODO: [PROJECT'23.MS2 - #04] [1] KERNEL HEAP - kfree()
	//refer to the project presentation and documentation for details
	// Write your code here, remove the panic and write your code

}

unsigned int kheap_virtual_address(unsigned int physical_address)
{
	struct FrameInfo * ptr_frame ;
	 ptr_frame = to_frame_info(physical_address);
	 //cprintf("the frame is %x and va is %x the pa is %x \n",ptr_frame, ptr_frame->actualva ,physical_address);
	if(ptr_frame->references > 0 && ptr_frame->references ==1){
		return (( ptr_frame->actualva & 0xFFFFF000)+(physical_address & 0x00000FFF));
	}
	//change this "return" according to your answer
	return 0;
}

unsigned int kheap_physical_address(unsigned int virtual_address)
{
		uint32 *ptr_page_table=NULL;
		struct FrameInfo * ptr_frame = get_frame_info(ptr_page_directory,virtual_address,&ptr_page_table);
		if(ptr_frame != NULL)
		{
		return ((ptr_page_table[PTX(virtual_address)] & 0xFFFFF000)+(virtual_address & 0x00000FFF));
		}
   return 0;
}


void kfreeall()
{
	panic("Not implemented!");

}

void kshrink(uint32 newSize)
{
	panic("Not implemented!");
}

void kexpand(uint32 newSize)
{
	panic("Not implemented!");
}

int allocating(void *virtual_address, uint32 size, uint32 theva){
	        int found= 1;
	        int itisfree=0;
	        uint32 *ptr_table=NULL;
	        //cprintf("size in kmalloc is %d \n",numofpages);

	        for(uint32 i = (uint32)virtual_address ; i<(uint32)virtual_address+size ; i+= PAGE_SIZE){
             if(get_frame_info(ptr_page_directory,i,&ptr_table)!=NULL){
            	 found = 0;
            	 break;
             }
		}
	        if(found ){
	        	  for(uint32 i = (uint32)virtual_address ; i<(uint32)virtual_address+size ; i+= PAGE_SIZE){
	        		  struct FrameInfo *ptr_frame_info=NULL;
	        		          				int ret=allocate_frame(&ptr_frame_info);
	        		          				ptr_frame_info->va = theva;
	        		          				ptr_frame_info->size = size;
	        		          				//if(j==i)
	        		          				//cprintf("the size in size is %d \n",ptr_frame_info->size);
	        		          				if(ret==0){
	        		          					ptr_frame_info->actualva =i ;
	        		          					int ret1=map_frame(ptr_page_directory,ptr_frame_info,i, (PERM_WRITEABLE & ~(PERM_USER)));
	        		          				}
	        			}
	        }
	       return found;

	        return 0;
}


//=================================================================================//
//============================== BONUS FUNCTION ===================================//
//=================================================================================//
// krealloc():

//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to kmalloc().
//	A call with new_size = zero is equivalent to kfree().

void *krealloc(void *virtual_address, uint32 new_size)
{
	 //cprintf("hiiii \n");
	if(virtual_address == NULL){
		return kmalloc(new_size);
	}
	if(new_size == 0){
		kfree(virtual_address);
		return NULL;
	}
	if((uint32)virtual_address <breakOfHeap){
		//cprintf("hereeeee \n");
		return realloc_block_FF(virtual_address,new_size);
	}
	else if((uint32)virtual_address >= (hardLimitOfHeap+PAGE_SIZE) && (uint32)virtual_address < KERNEL_HEAP_MAX){
     new_size = ROUNDUP(new_size,PAGE_SIZE);
     uint32 * ptr_table = NULL;
     struct FrameInfo * ptr_farme = get_frame_info(ptr_page_directory,(uint32)virtual_address,&ptr_table);
     if(ptr_farme != NULL){
    	 uint32 thesize = ptr_farme->size;
    	 //cprintf(" i am here with the size %x and new size %x \n",thesize , new_size);
    	 if(new_size == thesize){
    		 //cprintf("i am here \n");
    		 return virtual_address;
    	 }
    	 if(new_size > thesize){
    		 uint32 thediff = new_size - thesize;
    		 uint32 theva = (uint32)virtual_address+ thesize;
    		 if(theva + thediff < KERNEL_HEAP_MAX){
                   int ret = allocating((void *)theva,thediff,(uint32)virtual_address);
                   if(ret == 1){
                	   return virtual_address;
                   }
    		 }
    		 void * returnva = kmalloc(new_size);
    		                    if(returnva != NULL){
    		                 	   kfree(virtual_address);
    		                    }
    		                    return returnva;
    	 }
    	 else if(new_size < thesize){
    		 ptr_farme->size =new_size;
    		 uint32 * ptr_page_table = NULL;
    		 for(uint32 i = (uint32)virtual_address+new_size; i<(uint32)virtual_address+thesize; i+= PAGE_SIZE){
    		     	   struct FrameInfo * ptr_frame = get_frame_info(ptr_page_directory,i,&ptr_page_table);
    		     		   //free_frame(ptr_frame);
    		     		   ptr_frame->actualva = 0;
    		     		   ptr_frame->va =0;
    		     		   unmap_frame(ptr_page_directory,i);

    		        }
    		 return virtual_address;
    	 }
     }

	}
	else{
		panic("INVALID ADDRESS");
	}
	//TODO: [PROJECT'23.MS2 - BONUS#1] [1] KERNEL HEAP - krealloc()
	// Write your code here, remove the panic and write your code
	return NULL;
	//panic("krealloc() is not implemented yet...!!");
}
