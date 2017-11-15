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







//#define DEBUG


/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))


//把申请为static静态的声明都放在源文件的头部，表示这个是该源文件私有的，不放在头文件上面，因为头文件的更多是提供给别的函数用的
static void* head_listp;
/* 存放链表头的地方，一共13个链表块，分别是{1,8},{9,16},{17,32},{33,64},{65,128},{129,256},{257,512},{513,1024},{1025,2048}
        {2049,4096},{4097,8192},{8192,正无穷} */
static int MAX_SIZE=12;
static void* linkhead[12]={NULL};           



static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t size);     //找到合适的块
static void place(void* bp,size_t asize);   //放置块并进行分割
static void placefree(void* bp);            //将空闲链表放置在链表的开头
static void deletefree(void* bp);           //删除空闲结点
static int findlink(size_t size);           //寻找对应的下标

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
        //链表初始化
    for(int i=0;i<13;++i)
        linkhead[i]=NULL;

    /* Create the initial empty heap */
    if((head_listp=mem_sbrk(4*WSIZE))==(void *)-1)
        return -1;
    
    PUT(head_listp,0);       /* Alignment padding */
    PUT(head_listp+(1*WSIZE),PACK(DSIZE,1));  /* Prologue header */
    PUT(head_listp+(2*WSIZE),PACK(DSIZE,1));  /* Prologue footer */
    PUT(head_listp+(3*WSIZE),PACK(0,1));      /* Epilogue header */
    head_listp+=(2*WSIZE);
    
    



    #ifdef DEBUG
//        printf("the size of head_listp :%x\n",GET_SIZE(HDRP(head_listp)));
//        printf("the alloc of head_listp: %x\n",GET_ALLOC(FTRP(head_listp)));
    #endif
    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if(extend_heap(CHUNKSIZE/WSIZE)==NULL)
    {
        return -1;
    }
    #ifdef DEBUG
        printf("the size of freelist_head=%d\n",GET_SIZE(HDRP(freelist_head)));
    #endif
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

//    placefree(bp);

    #ifdef DEBUG
//        printf("the freelist_head size is :%d\n",GET_SIZE(HDRP(freelist_head)));
//        if(GET_ADDRESS(PRED(freelist_head))==NULL && GET_ADDRESS(SUCC(freelist_head))==NULL) 
//            printf("the double link of freelist_head is NULL");
    #endif


    /* Coalesce if the previous block was free */
    return coalesce(bp);
}


/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    #ifdef DEBUG
//        printf("the size of alloc=%zu\n",size);
    #endif
    size_t asize;   /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char *bp;

    /* Ignore spurious requests */
    if(size==0)
    {
        return NULL;
    }

    /* Adjust block size to include overhead and alignment reqs. */
    //要加上头尾两个指针
    if(size<=DSIZE)
        asize=3*DSIZE;
    else
        asize=DSIZE*((size+(DSIZE*2)+(DSIZE-1))/DSIZE);
    
    /* Search the free list for a fit */
    if((bp=find_fit(asize))!=NULL)
    {
        place(bp,asize);
//        printf("the payload of bp is:%u\n",GET_PAYLOAD(bp));
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
    #ifdef DEBUG
        printf("the point of free=%x\n",(unsigned int)ptr);
    #endif
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
    if(ptr==NULL) return mm_malloc(size);
    if(size==0)
    { 
        mm_free(ptr);
        return ptr;
    }
    if(ptr!=NULL)
    {
        size_t finalsize=0;
        void *newptr=mm_malloc(size);
        size_t oldsize=GET_PAYLOAD(ptr);
        if(oldsize<size)
            finalsize=oldsize;
        else
            finalsize=size;
        memcpy(newptr,ptr,finalsize);
        mm_free(ptr);
        return newptr;
    }
    return NULL;
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
        placefree(bp);
        return bp;
    }


    //情况二，后一块是空闲的
    else if(prev_alloc && !next_alloc)
    {
        deletefree(NEXT_BLKP(bp));
        size+=GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp),PACK(size,0));
        //改完头部大小就变了，只能直接访问尾部，对尾部进行改大小的操作
        PUT(FTRP(bp),PACK(size,0));
        placefree(bp);
        return bp;
    }

    //情况三，前一块是空闲的
    else if(!prev_alloc && next_alloc)
    {
        deletefree(PREV_BLKP(bp));
        size+=GET_SIZE(FTRP(PREV_BLKP(bp)));
        PUT(FTRP(bp),PACK(size,0));
        PUT(HDRP(PREV_BLKP(bp)),PACK(size,0));
        placefree(PREV_BLKP(bp));
        return PREV_BLKP(bp);
    }

    //情况四，前后都是空的
    else
    {
        deletefree(PREV_BLKP(bp));
        deletefree(NEXT_BLKP(bp));
        size+=(GET_SIZE(HDRP(NEXT_BLKP(bp)))+GET_SIZE(FTRP(PREV_BLKP(bp))));
        PUT(FTRP(NEXT_BLKP(bp)),PACK(size,0));
        PUT(HDRP(PREV_BLKP(bp)),PACK(size,0));
        placefree(PREV_BLKP(bp));
        return PREV_BLKP(bp);
    }
}



//寻找合适的块
static void *find_fit(size_t size)
{
    /* First fit search */
    for(int index=findlink(size);index<MAX_SIZE;++index)
    {
        void* bp=linkhead[index];
        while(bp!=NULL)
        {
            if(size<=GET_SIZE(HDRP(bp)))
            {
            return bp;
            }
            bp=GET_ADDRESS(SUCC(bp));
        }
    }

    return NULL;    
}


static void place(void* bp,size_t asize)
{

    size_t left=GET_SIZE(HDRP(bp))-asize;
    //大于双字要把头尾都考虑进行说明，可以进行分割,由于输入的asize肯定是双字结构，这样就保证了分割出来的内容也都是双字结构
    //前继和后继结点都要考虑进行
    if(left>=(DSIZE*3))
    {
        deletefree(bp);
        //申请的块为忙碌
        PUT(HDRP(bp),PACK(asize,1));
        PUT(FTRP(bp),PACK(asize,1));
        //分割出来的块为空闲
        PUT(HDRP(NEXT_BLKP(bp)),PACK(left,0));
        PUT(FTRP(NEXT_BLKP(bp)),PACK(left,0));
        //把该结点从空闲链表中删除，并把下一个结点加入空闲链表
        placefree(NEXT_BLKP(bp));
    }
    //无法进行分割
    else
    {
        size_t allsize=GET_SIZE(HDRP(bp));
        //全部设定为忙碌
        PUT(HDRP(bp),PACK(allsize,1));
        PUT(FTRP(bp),PACK(allsize,1));
        deletefree(bp);
    }
}


//1.使用直接添加到链表头，减少时间消耗，但是空间利用率被牺牲
//2.利用地址顺序来处理
static void placefree(void* bp)
{
    int index=findlink(GET_SIZE(HDRP(bp)));
    void* head=linkhead[index];
    if(head==NULL)
    {
        linkhead[index]=bp;
                //指向的是它的地址，地址里面存的都是是上一个或者下一个块的地址，所以每解一次引用得到的都是对应的下一个块的地址
        GET_ADDRESS(PRED(bp))=NULL;
        GET_ADDRESS(SUCC(bp))=NULL;
    }
    else
    {
        //将结点插在链表头部
        GET_ADDRESS(SUCC(bp))=linkhead[index];
        GET_ADDRESS(PRED(linkhead[index]))=bp;
        GET_ADDRESS(PRED(bp))=NULL;
        linkhead[index]=bp;
    }
}

static void deletefree(void* bp)
{
    int index=findlink(GET_SIZE(HDRP(bp)));
    if(linkhead[index]==NULL)
        printf("freelist is empty. Something must be wrong!!!!!");
    //链表中只有一个空闲块的时候
    if(GET_ADDRESS(PRED(bp))==NULL && GET_ADDRESS(SUCC(bp))==NULL)
    {
        linkhead[index]=NULL;
    }
    else if(GET_ADDRESS(PRED(bp))==NULL && GET_ADDRESS(SUCC(bp))!=NULL)  //链表头，并且不只有一个结点
    {
        GET_ADDRESS(PRED(GET_ADDRESS(SUCC(bp))))=NULL;
        linkhead[index]=GET_ADDRESS(SUCC(bp));
        GET_ADDRESS(SUCC(bp))=NULL;
    }
    else if(GET_ADDRESS(PRED(bp))!=NULL && GET_ADDRESS(SUCC(bp))==NULL)   //链表尾，并且不只有一个结点
    {
        GET_ADDRESS(SUCC(GET_ADDRESS(PRED(bp))))=NULL;
        GET_ADDRESS(PRED(bp))=NULL;
    }
    else                    //中间结点
    {
        GET_ADDRESS(SUCC(GET_ADDRESS(PRED(bp))))=GET_ADDRESS(SUCC(bp));
        GET_ADDRESS(PRED(GET_ADDRESS(SUCC(bp))))=GET_ADDRESS(PRED(bp));
        GET_ADDRESS(PRED(bp))=GET_ADDRESS(SUCC(bp))=NULL;
    }
}



//寻找对应的下标
static int findlink(size_t size)
{
    if(size<=4)
        return 0;
    else if(size<=16)
        return 1;
    else if(size<=32)
        return 2;
    else if(size<=64)
        return 3;
    else if(size<=128)
        return 4;
    else if(size<=256)
        return 5;
    else if(size<=512)
        return 6;
    else if(size<=1024)
        return 7;
    else if(size<=2048)
        return 8;
    else if(size<=4096)
        return 9;
    else if(size<=8192)
        return 10;
    else
        return 11;
}




int mm_checkheap(void)
{
//    char* start=head_listp;
//    char* next;
//    char* hi=mem_heap_hi();
    return 1;
}







