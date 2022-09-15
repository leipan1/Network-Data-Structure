/**
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include "debug.h"
#include "sfmm.h"

double aggregate_payload=0;
double heap_size=0;


sf_size_t block_alignment_size(sf_size_t size){
    sf_size_t block_size=size+8; //finds the minimum block size needed with header
    if(block_size<32){//checks if block size is less than 32 bytes
        block_size=32;
    }
    else{
        sf_size_t add_size=block_size%16;
        if(add_size!=0){//checks if size is multiple of 16
            add_size=16-add_size; //find amount of bytes needed for padding 
        }
        block_size+=add_size; //adds the size+header+padding
    } 
    debug("aligned block size: %i", block_size);
    return block_size;
}

//finding the appropriate index for free list !! have to change because of a stupid piazza post
int find_index(sf_size_t size){
    int multiple= size/32;
    int index= (int)(ceil(log(multiple)/log(2)));
    if(index>=10){
        return 9;
    }
    return index;
}

void* initialize_freelist(sf_size_t size){
    void* new_page=sf_mem_grow();
    heap_size+=(double)PAGE_SZ;

    if(new_page==NULL){
        sf_errno = ENOMEM;
        return NULL;
    }

    debug("succesfully got start of new page at %p", new_page);


    //intializing the prologue
    sf_block* prologue = sf_mem_start();
    debug("mem start addr: %p", prologue);
    prologue->header = 32;
    prologue->header |= THIS_BLOCK_ALLOCATED;
    debug("prologue header: %li", prologue->header);
    
    debug("prologue intialized");
    
    //intializing free list
    for(int i=0;i<NUM_FREE_LISTS;i++){
        sf_free_list_heads[i].body.links.next= &sf_free_list_heads[i];
        sf_free_list_heads[i].body.links.prev= &sf_free_list_heads[i];
    }


    //intializing free block
    debug("intializing the first block");
    sf_block *first_block= sf_mem_start()+32; //incrementing first block to correct row
    first_block->prev_footer=prologue->header;
    debug("first block starting addr: %p", (void*)first_block);
    first_block->header= (PAGE_SZ-48);
    first_block->header |= PREV_BLOCK_ALLOCATED;
    debug("first block header: %li", first_block->header);
    

    //getting the index for the appropriate list_header
    int index=find_index(PAGE_SZ-48);
    debug("index for list_header %i", index);
    
    //init the next and prev of the first block to the appropriate index
    first_block->body.links.next=&sf_free_list_heads[index];
    first_block->body.links.prev=&sf_free_list_heads[index];
    sf_free_list_heads[index].body.links.next=first_block;
    sf_free_list_heads[index].body.links.prev=first_block;

    //creating the footer for epilogue
    sf_block* epilogue= sf_mem_end()-16;
    epilogue->prev_footer=first_block->header;
    epilogue->header |= THIS_BLOCK_ALLOCATED;
    debug("epilogue initialized");

    //initializing quick list
    for(int i=0;i<NUM_QUICK_LISTS;i++){
        sf_quick_lists[i].first=NULL;
    }

    sf_show_heap();

    return ((void*)first_block+8);
}

void deallocate_from_free_list(void* block_address){
    debug("addr to deallocate from free list: %p",(void*)block_address+8);
    int index;
    int found=0;
    for(int i=0;i<NUM_FREE_LISTS;i++){
        sf_block* traverse_fl=&sf_free_list_heads[i];
        debug("currently at free list index: %i",i);
        while(traverse_fl->body.links.next!=&sf_free_list_heads[i]&&found==0){
            traverse_fl=traverse_fl->body.links.next;
            debug("traverse_fl addr: %p",traverse_fl);
            if(traverse_fl==block_address){
                index=i;
                debug("match found at free list index: %i", index);
                found=1;
                i=NUM_FREE_LISTS;
            }
        }
    }

    
    sf_block* prev_block=&sf_free_list_heads[index];
    while (prev_block->body.links.next!=block_address)
    {
        prev_block=prev_block->body.links.next;
    }
    debug("the prev block should be: %p",prev_block);

    sf_block* next_block=&sf_free_list_heads[index];
    while (next_block->body.links.prev!=block_address)
    {
        next_block=next_block->body.links.next;
    }
    debug("the next block should be: %p", next_block);

    sf_block* remove_block=(sf_block*)block_address;
    prev_block->body.links.next=remove_block->body.links.next;
    next_block->body.links.prev=remove_block->body.links.prev;

    remove_block->body.links.next=NULL;
    remove_block->body.links.prev=NULL;

    debug("done delinking");
    sf_show_heap();
    
    
}

void add_to_free_list(sf_block* block_addr, sf_size_t size){

    debug("adding to free list pointer now");
    debug("addr of passed pointer: %p", block_addr);
    int index=find_index(size);
    
    debug("the index for the list_header: %i", index);

    sf_block* prev_addr=&sf_free_list_heads[index];
    debug("the prev addr should be %p", prev_addr);

    sf_block* next_addr=prev_addr->body.links.next;
    debug("the next addr should be %p", next_addr);

    block_addr->body.links.prev=prev_addr;
    block_addr->body.links.next=prev_addr->body.links.next;

    prev_addr->body.links.next=block_addr;
    next_addr->body.links.prev=block_addr;


    sf_show_heap();

}


void* coalece(void* block_address){
    //first combine the current black and the next block (if not alloc)
    debug("now coalecing the address: %p", block_address);
    sf_block* this_block=(sf_block*) block_address;
    sf_header this_block_size=this_block->header&0xFFFFFF0;
    debug("this block size is: %li", this_block_size);
    
    sf_block* next_block= (void*)this_block+this_block_size;
    sf_header next_block_alloc= (next_block->header&0x4)>>2;
    debug("next block alloc?: %li", next_block_alloc);

    if(next_block_alloc!=1){
        debug("combining with next block");
        deallocate_from_free_list(this_block);
        deallocate_from_free_list(next_block);

        sf_header next_block_size=next_block->header&0xFFFFFF0;
        debug("next block size: %ld", next_block_size);
        this_block->header=this_block->header+next_block->header;
        sf_block* next_next_block_prev_footer=(void*)this_block+next_block_size+this_block_size;
        next_next_block_prev_footer->prev_footer=this_block->header;

        debug("new block address is: %p", block_address);
        debug("done combining this block with next block");

        add_to_free_list(this_block,this_block_size+next_block_size);
        //next_block_alloc= (next_block->header&0x4)>>2;
        sf_show_heap();

        sf_block* next_next_block=(void*)this_block+next_block_size+this_block_size;
        next_block_alloc=(next_next_block->header&0x4)>>2;
    }


    debug("now coalecing the address: %p", block_address);
    this_block=(sf_block*)block_address;
    this_block_size=(this_block->header)&0xFFFFFF0;
    debug("this block size is: %li", this_block_size);
     
    sf_header prev_block_alloc=(this_block->prev_footer&0x4)>>2;
    debug("prev block alloc?: %li", prev_block_alloc);

    if(prev_block_alloc!=1){
        debug("combining with prev block");
        deallocate_from_free_list(this_block);
        sf_header prev_block_size=this_block->prev_footer&0xFFFFFF0;
        deallocate_from_free_list((void*)this_block-prev_block_size);
        sf_header prev_block_prev_alloc=this_block->prev_footer&0x2;
        debug("prev block size: %li", prev_block_size);
        sf_block* combine_prev=(void*)this_block-prev_block_size;
        combine_prev->header=prev_block_size+this_block_size+prev_block_prev_alloc;
        
        sf_block* prev_block= (void*)this_block-prev_block_size;
        combine_prev->prev_footer=prev_block->prev_footer;

        next_block->prev_footer=combine_prev->header;

        block_address=(void*)combine_prev;
        debug("new block address is: %p", block_address);
        debug("done combining this block with prev block");

        add_to_free_list(block_address,this_block_size+prev_block_size);
        sf_show_heap();
        prev_block_alloc=(combine_prev->prev_footer&0x4)>>2;
    }

    debug("prev block alloc: %li, next block alloc %li", prev_block_alloc,next_block_alloc);
    if(prev_block_alloc==1 && next_block_alloc ==1){
        deallocate_from_free_list(block_address);
        debug("done coalescing");
        return block_address;
    }

    debug("calling recursive coalesce");
    block_address=coalece(block_address);
    sf_show_heap();

    return block_address;
    

}


//size=payload, aligned_block=aligned size, fl_pointer=addr of pointer that is being split, total_size=total size of the freeblock, 
void* slice_block(sf_size_t size, sf_size_t aligned_block, sf_block* fl_pointer, sf_size_t total_size){

    //changing the prev and next
    debug("splitting blocks now");
    sf_show_heap();

    
    deallocate_from_free_list(fl_pointer);


    //setting up headers for the new alloc block
    sf_block* lower_block=fl_pointer;
    debug("addr for alloc block starts at: %p", lower_block);
    debug("aligned size: %d",aligned_block);
    lower_block->header=aligned_block;
    lower_block->header|=THIS_BLOCK_ALLOCATED;
    aggregate_payload+=(double)size;
    lower_block->header|=PREV_BLOCK_ALLOCATED;
    sf_header payload_size_header=size;
    payload_size_header=payload_size_header<<32;
    lower_block->header=(lower_block->header & 0xFFFFFFF)+payload_size_header;
    
    debug("lower_block payload size: %li", lower_block->header>>32);
    debug("alloced returned address: %p", lower_block->body.payload);
    
    sf_size_t remaining_size=total_size-aligned_block;
    debug("remaining size of this block is: %d", remaining_size);


    //adjusting the "upper" block
    debug("aligned block size is: %i", aligned_block);
    debug("lower block address starts at: %p", lower_block);
    sf_block* upper_block=(void*)lower_block+aligned_block;
    debug("upper_block addr is: %p", upper_block);
    upper_block->header=remaining_size;
    debug("upper_block size is: %li",upper_block->header);
    upper_block->header|=PREV_BLOCK_ALLOCATED;
    upper_block->prev_footer=lower_block->header^sf_magic();
    sf_block* this_footer= (void*)upper_block+remaining_size;
    this_footer->prev_footer=upper_block->header^sf_magic();

    sf_show_heap();

    if(remaining_size!=0){
        add_to_free_list(upper_block, remaining_size); 
    }  

    return lower_block->body.payload; //RETURNS ADDR OF THE ALLOC BLOCK


}


void* insert_free_list(sf_size_t size, void* pointer){
    int found_list=0;
    sf_size_t avail_size;
    sf_size_t block_size=block_alignment_size(size);
    debug("the aligned block size: %i", block_size);
    
    debug("heap is already initialized; finding appropriate freelist start at index: %i", find_index(block_size));
    sf_block* fl_pointer;
    do{
        for(int i=find_index(block_size);i<NUM_FREE_LISTS;i++){
            debug("i val: %i",i);
            fl_pointer=(void*)(sf_free_list_heads[i].body.links.next); //gets the next block of free list
            debug("current free_list address: %p", &sf_free_list_heads[i]);
            debug("fl_pointer after next: %p", fl_pointer);

            sf_size_t prev_allocation=(fl_pointer->header);//checks if prev_alloc is set
            prev_allocation=(prev_allocation & 0x2)>>1;
            debug("prev_alloc?: %u", prev_allocation);
            
            if(prev_allocation==1){
                debug("got the index of an appropriate free list");

                avail_size=fl_pointer->header;
                avail_size=(avail_size>>4)<<4;
                debug("the avail_size is: %u", avail_size);
                if((avail_size>=block_size) && (avail_size-block_size>=32 || avail_size-block_size==0)){
                    debug("enough space and won't cause splinter");
                    i=NUM_FREE_LISTS;
                    found_list=1;
                }
            }
        }
        if(found_list!=1){
            debug("no free list with enough size\n");
        
            void * add_page=sf_mem_grow();
            heap_size+=PAGE_SZ;
            if(add_page==NULL){
                sf_errno = ENOMEM;
                return NULL;
            }
            debug("successfully added a new page at %p", add_page);
            debug("block starts at this addr: %p", (void*)add_page-16);

            sf_block* new_page_block= (void*)add_page-16;
            new_page_block->header= PAGE_SZ;
            debug("new page block size: %li", new_page_block->header);

            if((new_page_block->prev_footer&0x4)>>2==1){
                debug("previous block is allocated");
                new_page_block->header|=PREV_BLOCK_ALLOCATED;
            }   

            
            sf_block* new_page_epilogue= (void*)sf_mem_end()-16;
            new_page_epilogue->prev_footer=new_page_block->header;
            new_page_epilogue->header|= THIS_BLOCK_ALLOCATED;
            add_to_free_list(new_page_block,PAGE_SZ);
            debug("new page initialized");
            sf_show_heap();

            sf_block* combined_blocks=(sf_block*)coalece(new_page_block);
            debug("combined block addr: %p", combined_blocks);
            sf_show_heap();

            //deallocate_from_free_list(combined_blocks);
            add_to_free_list(combined_blocks,combined_blocks->header&0xFFFFFF0);
        }
    }while(found_list!=1);
    
    debug("the address of free list being passed to be split is: %p", fl_pointer);
    sf_show_heap();
    sf_block* alloc_block=slice_block(size, block_size,fl_pointer,avail_size);
        
    return alloc_block;
    
}

void* remove_from_quick_list(int index){
    debug("removing first block from quick list");
    sf_block* removed_block= sf_quick_lists[index].first;
    sf_quick_lists[index].first=removed_block->body.links.next;

    removed_block->header=removed_block->header-0x4;
    debug("alloc changed to 0");

    sf_block* removed_block_next=(void*)removed_block+(removed_block->header&0xFFFFFF0);
    removed_block_next->header=removed_block_next->header-0x2;
    debug("next block's prev alloc is set to 0");
    removed_block_next->prev_footer=removed_block->header;

    sf_block* removed_block_next_next=(void*)removed_block_next+(removed_block_next->header&0xFFFFFF0);
    removed_block_next_next->prev_footer=removed_block_next->header;
    
    return removed_block;
}

void make_quick_list(int index,void* pp){
    sf_block* pp_block = (sf_block*)pp;
    sf_block* removed_block;
    sf_header pp_block_size=((pp_block->header)<<32)>>32;
    pp_block_size=(pp_block_size>>2)<<2;
    pp_block->header=pp_block_size;
    pp_block->header|=IN_QUICK_LIST;
    debug("pp header: %li",pp_block->header);

    if(sf_quick_lists[index].length!=QUICK_LIST_MAX){
        debug("quick list isn't full yet");
        sf_block* prev_first=sf_quick_lists[index].first;
        sf_quick_lists[index].first=pp_block;
        sf_quick_lists[index].length++;
        sf_quick_lists[index].first->body.links.next=prev_first;
    }
    else{
        debug("quick list full, flushing");
        for(int i=0;i<QUICK_LIST_MAX;i++){
            removed_block=(sf_block*)remove_from_quick_list(index);
            sf_size_t removed_block_size=(removed_block->header>>4)-8;
            insert_free_list(removed_block_size,removed_block);
        }
        make_quick_list(index,pp);
    }
    sf_show_heap();
    return;
}



/*
 * This is your implementation of sf_malloc. It acquires uninitialized memory that
 * is aligned and padded properly for the underlying system.
 *
 * @param size The number of bytes requested to be allocated.
 *
 * @return If size is 0, then NULL is returned without setting sf_errno.
 * If size is nonzero, then if the allocation is successful a pointer to a valid region of
 * memory of the requested size is returned.  If the allocation is not successful, then
 * NULL is returned and sf_errno is set to ENOMEM.
 */
void *sf_malloc(sf_size_t size) {
    debug("called sf_malloc");
    void* pointer;
    debug("passed sized: %u", size);
    //checks size number
    if (size<=0){
        return NULL;
    }
    //if heap wasn't intialized
    if(sf_mem_start()==sf_mem_end()){
        debug("initializing heap");
        pointer = initialize_freelist(size);
        if(pointer==NULL){
            return NULL;
        }
    }

    sf_size_t block_size=block_alignment_size(size);
    if(block_size<(32+(16*9))){
        debug("small enough to insert to quicklist");
        int index=(block_size-32)/16;
        debug("the index for quicklist: %i",index);  
        if(sf_quick_lists[index].length==0){
            debug("empty quicklist, inserting to free list");
            pointer=insert_free_list(size,pointer);
        }
        else{
            debug("alloc to quicklist");
            pointer=remove_from_quick_list(index);
            debug("addr to the quicklist: %p",pointer);
        }

    }
    else{
        debug("size too large for quicklist, inserting to freelist");
        pointer=insert_free_list(size,pointer);
    }

    //allocation request
    //pointer= insert_heap(size);
    


    debug("end of program returning pointer: %p\n",pointer);
    sf_show_heap();

    return pointer;
}


void sf_free(void *pp) {
    debug("called sf_free");

    if(pp==NULL){
        debug("pp null, aborting");
        abort();
    }

    
    debug("pp addr: %p", pp);
    pp-=16;
    sf_block* pp_block= (sf_block*)pp;
    sf_size_t pp_block_size=((pp_block->header >> 4) << 4);
    debug("pp block size: %u",pp_block_size);


    if(pp_block_size%16!=0){
        debug("not multiple of 16, aborting");
        abort();
    }


    if((((void*)(&(pp_block->header))<sf_mem_start()+40))|| (((void*)(&(pp_block->header))>sf_mem_end()-8))){
        debug("block header is at incorrect section of the heap, aborting");
        abort();
    }

    debug("pp_block if allocated: %ld",(((pp_block->header)&0x4)>>2));
    if((((pp_block->header)&0x4)>>2)==0){
        debug("not allocated, aborting");
        abort();
    }

    sf_footer pp_prev_size= ((pp_block->prev_footer)>>32);
    debug("size of previous block is: %ld", pp_prev_size);
    void* get_addr=&pp_block->prev_footer;
    debug("addr of pp_prev_footer: %p", get_addr);
    get_addr=get_addr-pp_prev_size;
    debug("should be addr of the previous header: %p",get_addr);
    sf_block* pp_prev_header=(sf_block*)get_addr;
    sf_header pp_prev_header_alloc=(((pp_prev_header->header)&0x4)>>2);
    debug("pp prev header alloc: %ld", pp_prev_header_alloc);
    
    sf_footer pp_prev_footer= (((pp_block->prev_footer)&0x4)>>2);
    debug("pp prev_footer prev_alloc: %ld",pp_prev_footer);
    if((pp_prev_footer==0)&&(pp_prev_header_alloc!=0)){
        debug("footer and header alloc mismatch, aborting");
        abort();
    }

    

    debug("pointer is an allocated block verified\n");

    
    if(pp_block_size<(32+(16*9))){
        debug("small enough to insert to quicklist");
        int index=(pp_block_size-32)/16;
        debug("the index for quicklist: %i",index);
        make_quick_list(index,pp);
        debug("finished making quick list");
    }
    else{
        debug("size too large for quicklist, inserting to freelist");
        //sf_size_t pp_payload_size=pp_block->header>>32;
        //pp=insert_free_list(pp_payload_size,&pp_block->prev_footer);
        debug("freeing the block");
        pp_block->header=pp_block->header-0x4;
        sf_block* pp_next_block=(void*)pp_block+pp_block_size;
        pp_next_block->prev_footer=pp_block->header;
        pp_next_block->header=pp_next_block->header-0x2;
        debug("changing footer of next block");
        sf_size_t pp_next_block_size=pp_next_block->header&0xFFFFF0;
        sf_block* pp_next_next_block=(void*)pp_next_block+pp_next_block_size;
        pp_next_next_block->prev_footer=pp_next_block->header;
        sf_show_heap();
        add_to_free_list(pp_block,pp_block_size);
        
        sf_block* combined_blocks=(sf_block*)coalece(pp);
        debug("combined block addr: %p", combined_blocks);
        sf_show_heap();
        //deallocate_from_free_list(combined_blocks);
        add_to_free_list(combined_blocks,combined_blocks->header&0xFFFFFF0);
    }
    debug("done freeing");
    sf_show_heap();
    return;
}



void *sf_realloc(void *pp, sf_size_t rsize) {
    debug("resizing %p to %i",pp,rsize);
    void* pointer;
    debug("called sf_realloc");

    if(pp==NULL){
        debug("pp null, aborting");
        abort();
    }
    
    debug("pp addr: %p", pp);
    pp-=16;
    sf_block* pp_block= (sf_block*)pp;
    sf_size_t pp_block_size=((pp_block->header >> 4) << 4);
    debug("pp block size: %u",pp_block_size);


    if(pp_block_size%16!=0){
        debug("not multiple of 16, aborting");
        abort();
    }


    if((((void*)(&(pp_block->header))<sf_mem_start()+40))|| (((void*)(&(pp_block->header))>sf_mem_end()-8))){
        debug("block header is at incorrect section of the heap, aborting");
        abort();
    }

    debug("pp_block if allocated: %ld",(((pp_block->header)&0x4)>>2));
    if((((pp_block->header)&0x4)>>2)==0){
        debug("not allocated, aborting");
        abort();
    }

    sf_footer pp_prev_size= ((pp_block->prev_footer)>>32);
    debug("size of previous block is: %ld", pp_prev_size);
    void* get_addr=&pp_block->prev_footer;
    debug("addr of pp_prev_footer: %p", get_addr);
    get_addr=get_addr-pp_prev_size;
    debug("should be addr of the previous header: %p",get_addr);
    sf_block* pp_prev_header=(sf_block*)get_addr;
    sf_header pp_prev_header_alloc=(((pp_prev_header->header)&0x4)>>2);
    debug("pp prev header alloc: %ld", pp_prev_header_alloc);
    
    sf_footer pp_prev_footer= (((pp_block->prev_footer)&0x4)>>2);
    debug("pp prev_footer prev_alloc: %ld",pp_prev_footer);
    if((pp_prev_footer==0)&&(pp_prev_header_alloc!=0)){
        debug("footer and header alloc mismatch, aborting");
        abort();
    }

    debug("pointer is an allocated block verified\n");
    
    if(rsize==0){
        debug("rsize is 0");
        sf_free(pp);
        return 0;
    }


    if(pp_block_size<rsize)
    {
        debug("reallocating to a larger size");
        pointer=sf_malloc(rsize);
     
        if(pointer==NULL)
        {
            return NULL;
        }

        memcpy(pointer,pp_block->body.payload,pp_block_size);
        debug("did memcpy");
        sf_free(pp_block->body.payload);

        debug("finish reallocating");
        return pointer;
    }    
    else if(pp_block_size>rsize){
        debug("reallocating to a smaller size");
        sf_size_t minimumsize=block_alignment_size(rsize);
        if(pp_block_size-minimumsize<32){
            debug("splitting will cause splinter, changing heading");
            sf_header new_payload=rsize;
            new_payload<<=32;
            pp_block->header=((pp_block->header<<32)>>32)+new_payload;

            //pp_block->header=(pp_block->header&0xFFFFFFFF0000000F)+rsize;
            sf_block* next_block=(void*)pp_block+pp_block_size;
            next_block->prev_footer=pp_block->header;
            sf_show_heap();
            return pp_block->body.payload;
            
        }
        else{
            debug("splitting the block");

            sf_block* lower_block=pp_block;
            lower_block->header=minimumsize;
            lower_block->header|=THIS_BLOCK_ALLOCATED;
            aggregate_payload+=rsize;
            lower_block->header|=PREV_BLOCK_ALLOCATED;
            sf_header payload_size_header=rsize;
            payload_size_header=payload_size_header<<32;
            lower_block->header=(lower_block->header & 0xFFFFFFF)+payload_size_header;

            size_t remaining_size=pp_block_size-minimumsize;

            sf_block* upper_block=(void*)lower_block+minimumsize;
            upper_block->header=remaining_size;
            upper_block->header|=PREV_BLOCK_ALLOCATED;
            upper_block->prev_footer=lower_block->header^sf_magic();
            sf_block* this_footer=(void*)upper_block+remaining_size;
            this_footer->prev_footer=upper_block->header^sf_magic();

            sf_block* upper_block_next=(void*)pp_block+pp_block_size;
            upper_block_next->header=upper_block_next->header-0x2;

            sf_block* upper_block_next_next=(void*)upper_block_next+(upper_block_next->header&0xFFFFFFF0);
            upper_block_next_next->prev_footer=upper_block_next->header;

            sf_show_heap();

            if(remaining_size!=0){
                add_to_free_list(upper_block,remaining_size);
            }

            pointer=lower_block->body.payload;
            
            sf_block* combined_blocks=(sf_block*)coalece((void*)pp_block+minimumsize);
            sf_show_heap();
            add_to_free_list(combined_blocks,combined_blocks->header&0xFFFFFF0);
            return pointer;
        }
    }
    else{
        debug("no need for realloc");
        return pp_block->body.payload;
    }
}

double sf_internal_fragmentation() {
    debug("called sf_internal_frag");
    double total_payload=0;
    double total_alloc_blocks=0;
    if(sf_mem_start()==sf_mem_end()){
        return 0.0;
    }
    void* start=sf_mem_start()+32;
    void* end=sf_mem_end()-8;

    sf_block* block= (sf_block*)start;
    while ((void*)block!=end)
    {
       sf_size_t block_payload= block->header>>32;
       sf_size_t block_alloc= block->header&0xFFFFFFF0;

       total_payload+=(double)block_payload;
       total_alloc_blocks+=(double)total_alloc_blocks;
       block=(void*)block+block_alloc;
    }
    if(total_alloc_blocks==0){
        return 0.0;
    }
    return total_payload/total_alloc_blocks;
    
}

double sf_peak_utilization() {
    debug("called sf_peak_utilization");
    if(sf_mem_start()==sf_mem_end()){
        return 0.0;
    }
    return aggregate_payload/heap_size;

}
