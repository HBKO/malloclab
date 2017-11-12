/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "hello world",
    /* First member's full name */
    "hekewen",
    /* First member's email address */
    "821200725@qq.com",
    /* Second member's full name (leave blank if none) */
    "hekewen_2",
    /* Second member's email address (leave blank if none) */
    "821200725@qq.com"
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))


//把申请为static静态的声明都放在源文件的头部，表示这个是该源文件私有的，不放在头文件上面，因为头文件的更多是提供给别的函数用的
static void* head_listp;
static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t size);     //找到合适的块
static void place(void* bp,size_t asize);   //放置块并进行分割


/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    /* Create the initial empty heap */
    if((head_listp=mem_sbrk(4*WSIZE))==(void *)-1)
        return -1;
    
    PUT(head_listp,0);       /* Alignment padding */
    PUT(head_listp+(1*WSIZE),PACK(DSIZE,1));  /* Prologue header */
    PUT(head_listp+(2*WSIZE),PACK(DSIZE,1));  /* Prologue footer */
    PUT(head_listp+(3*WSIZE),PACK(0,1));      /* Epilogue header */
    head_listp+=(2*WSIZE); 
//    printf("the size of head_listp :%x\n",GET_SIZE(HDRP(head_listp)));
//    printf("the alloc of head_listp: %x\n",GET_ALLOC(FTRP(head_listp)));
    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if(extend_heap(CHUNKSIZE/WSIZE)==NULL)
    {
        return -1;
    }
    return 0;
}


//扩展堆的大小
static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size=(words %2)? (words+1)*WSIZE: words*WSIZE;
    if((long)(bp=mem_sbrk(size))==-1)
        return NULL;
    
    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp),PACK(size,0));  /* Free block header */
    PUT(FTRP(bp),PACK(size,0));  /* Free block footer */
    PUT(HDRP(NEXT_BLKP(bp)),PACK(0,1));   /* New epilogue header */


    /* Coalesce if the previous block was free */
    return coalesce(bp);
}


/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;   /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char *bp;

    /* Ignore spurious requests */
    if(size==0)
    {
        return NULL;
    }

    /* Adjust block size to include overhead and alignment reqs. */
    if(size<=DSIZE)
        asize=2*DSIZE;
    else
        asize=DSIZE*((size+(DSIZE)+(DSIZE-1))/DSIZE);
    
    /* Search the free list for a fit */
    if((bp=find_fit(asize))!=NULL)
    {
        place(bp,asize);
        return bp;
    }

    /* No fit found. Get more memory and place the block */
    extendsize=MAX(asize,CHUNKSIZE);
    //不够申请
    if((bp=extend_heap(extendsize/WSIZE))==NULL)
    {
        return NULL;
    }
    place(bp,asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size=GET_SIZE(HDRP(ptr));
    //头尾归为free的block
    PUT(HDRP(ptr),PACK(size,0));
    PUT(FTRP(ptr),PACK(size,0));
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}







static void *coalesce(void *bp)
{
    //关于这一块的改free操作已经在free函数的过程中执行了


    size_t prev_alloc=GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc=GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size=GET_SIZE(HDRP(bp));

    //情况一，前一块和后一块都被申请了
    if(prev_alloc && next_alloc)
    {
        return bp;
    }


    //情况二，后一块是空闲的
    else if(prev_alloc && !next_alloc)
    {
        size+=GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp),PACK(size,0));
        //改完头部大小就变了，只能直接访问尾部，对尾部进行改大小的操作
        PUT(FTRP(bp),PACK(size,0));
        return bp;
    }

    //情况三，前一块是空闲的
    else if(!prev_alloc && next_alloc)
    {
        size+=GET_SIZE(FTRP(PREV_BLKP(bp)));
        PUT(FTRP(bp),PACK(size,0));
        PUT(HDRP(PREV_BLKP(bp)),PACK(size,0));
        return PREV_BLKP(bp);
    }

    //情况四，前后都是空的
    else
    {
        size+=(GET_SIZE(HDRP(NEXT_BLKP(bp)))+GET_SIZE(FTRP(PREV_BLKP(bp))));
        PUT(FTRP(NEXT_BLKP(bp)),PACK(size,0));
        PUT(HDRP(PREV_BLKP(bp)),PACK(size,0));
        return PREV_BLKP(bp);
    }
}



//寻找合适的块
static void *find_fit(size_t size)
{
    /* First fit search */
    void* bp;
    for(bp=head_listp;GET_SIZE(HDRP(bp))>0;bp=NEXT_BLKP(bp))
    {
        if(!GET_ALLOC(HDRP(bp)) && (size<=GET_SIZE(HDRP(bp))))
            return bp;
    }
    return NULL;    
}


static void place(void* bp,size_t asize)
{
    size_t left=GET_SIZE(HDRP(bp))-asize;
    //大于双字要把头尾都考虑进行说明，可以进行分割,由于输入的asize肯定是双字结构，这样就保证了分割出来的内容也都是双字结构
    if(left>=(DSIZE*2))
    {
        //申请的块为忙碌
        PUT(HDRP(bp),PACK(asize,1));
        PUT(FTRP(bp),PACK(asize,1));
        //分割出来的块为空闲
        PUT(HDRP(NEXT_BLKP(bp)),PACK(left,0));
        PUT(FTRP(NEXT_BLKP(bp)),PACK(left,0));
    }
    //无法进行分割
    else
    {
        size_t allsize=GET_SIZE(HDRP(bp));
        //全部设定为忙碌
        PUT(HDRP(bp),PACK(allsize,1));
        PUT(FTRP(bp),PACK(allsize,1));
    }
}


int mm_checkheap(void)
{
//    char* start=head_listp;
//    char* next;
//    char* hi=mem_heap_hi();
    return 1;
}



