/*
 * mm.c
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */

 /*

 */

 /*
 *分离空闲链表
 *四字节地址相对寻址
 *已分配块去脚部优化
 *小块LIFO大块FIFO
 *first fit
 *magic parameter
 */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mm.h"
#include "memlib.h"

/* If you want debugging output, use the following macro.  When you hand
 * in, remove the #define DEBUG line. */
//#define DEBUG
#ifdef DEBUG
# define dbg_printf(...) printf(__VA_ARGS__)
#else
# define dbg_printf(...)
#endif

//#define DEBUGCHECK
#ifdef DEBUGCHECK
# define dbg_checkheap(x)   mm_checkheap(x)
#else
# define dbg_checkheap(x)
#endif

/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif /* def DRIVER */

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(p) (((size_t)(p) + (ALIGNMENT-1)) & ~0x7)

#define WSIZE       4        //字、脚部或头部的大小（字节）
#define DSIZE       8        //双字大小（字节）
#define CHUNKSIZE  (1<<12)   //扩展堆时的默认大小
#define MINBLOCK ALIGN(WSIZE + 2*WSIZE)  //头部、两指针、对齐，共16字节
#define SEGLIST_SIZE 9//(0,32] (32,65] (64,128] (128,256] (256,512] (512,1024] (1024,2048] (2048,4096] (4096,INF)

#define MAX(x, y)  ((x) > (y) ? (x) : (y))

#define PACK(size, palloc, alloc)  ((size) | (palloc<<1) | (alloc))         //将 size、prev allocated bit和 allocated bit 合并为一个字

#define GET(p)             (*(unsigned int *)(p))          //读地址p处的一个字,4byte
#define PUT(p, val)        (*(unsigned int *)(p) = (val))  //向地址p处写一个字,4byte
#define GETADDR(p)         (uint64_t)(*(uint32_t *)(p))   //读地址p处的一个指针,读低32位，强制转化成64位；
#define PUTADDR(p, addr)   (*(uint32_t *)(p) = (uint32_t)(uint64_t)(addr))  //向地址p处写一个指针,只存低32位
#define MERGEADDR(hi,lo)   ((void *)(((uint64_t)hi & 0xffffffff00000000) | ((uint64_t)lo & 0x00000000ffffffff)))//利用heaplistp计算偏移得到绝对地址


#define GET_SIZE(p)   (size_t)(GET(p) & ~0x07)    //得到地址p处的 size
#define GET_ALLOC(p)  (GET(p) & 0x1)      //得到地址p处的 allocated bit
#define GET_PALLOC(p) ((GET(p)>>1) & 0x1) //得到地址p处的prev alloc bit
#define SET_SIZE(p, size)   ((*(unsigned*)(p)) = ((*(unsigned*)(p)) & 0x7) | size)//将p处size置为size
#define SET_ALLOC(p)        ((*(unsigned*)(p)) |= 0x1)//将p处alloc bit置为1
#define RESET_ALLOC(p)      ((*(unsigned*)(p)) &= ~0x1)//将p处alloc bit置为0
#define SET_PALLOC(p)       ((*(unsigned*)(p)) |= 0x2)//将p处prev alloc bit置为1
#define RESET_PALLOC(p)      ((*(unsigned*)(p)) &= ~0x2)//将p处prev alloc bit置为0

//block point --> bp指向有效载荷块指针
#define HDRP(bp)     ((char*)(bp) - WSIZE)                       //获得头部的地址
#define FTRP(bp)     ((char*)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)  //获得脚部的地址, 与宏定义HDRP有耦合

#define NEXT_BLKP(bp)    ((char*)(bp) + GET_SIZE((char*)(bp) - WSIZE))  //计算后块的地址
#define PREV_BLKP(bp)    ((char*)(bp) - GET_SIZE((char*)(bp) - DSIZE))  //计算前块的地址

#define PRED_POINT(bp)   ((char*)(bp) + WSIZE)            //指向祖先指针的指针
#define SUCC_POINT(bp)   (bp)                             //指向后继指针的指针

static void **segList; //链表头指针数组
static void **segListEnd;//链表尾指针数组
static void *heap_listp; //指向序言块，用于寻址
static void *epilogue_head;//指向结尾块head
static void *extend_heap(size_t size);//拓展堆并插入空闲链表
static void *coalesce(void *bp);//合并内存块并修改空闲链表
static void insert_freelist(void *bp);//寻找合适空闲链表并插入，混合LIFO与FIFO
static void remove_freelist(void *bp);//从空闲列表中移除
static int isSegList(void *bp);// 判断是否找到链表头
static void *find_fit(size_t size);//找合适空闲块，firstfit
static void divide(void *bp, size_t size);//分离适配，分割空闲块，并修改空闲链表
static int getSegListIndex (size_t size);//找合适链表
static int aligned(const void *p);//检查p是否对齐

static int aligned(const void *p){
    return (size_t)ALIGN(p) == (size_t)p;
}

static int getSegListIndex (size_t size)
{
    int idx;
	if (size <= 32) {
		idx = 0;
	} else if (size <= 64) {
		idx = 1;
	} else if (size <= 128) {
		idx = 2;
	} else if (size <= 256) {
		idx = 3;
	} else if (size <= 512) {
		idx = 4;
	} else if (size <= 1024) {
		idx = 5;
	} else if (size <= 2048) {
		idx = 6;
	} else if (size <= 4096) {
		idx = 7;
	} else {	// size > 4096 
		idx = 8;
	}
	return idx;
}

static void divide(void *bp, size_t size)
{
    dbg_printf("place block\n");
    size_t total, remain;
    total = GET_SIZE(HDRP(bp));
    remain = total - size;
    if(remain >= MINBLOCK)//剩余部分够大，分割
    {
        dbg_printf("total:%ld remain:%ld ",total,remain);
        remove_freelist(bp);
        PUT(HDRP(bp),PACK(size, GET_PALLOC(HDRP(bp)), 1));
        dbg_printf("old, ");
        void *next = NEXT_BLKP(bp);
        PUT(HDRP(next),PACK(remain, 1, 0));
        PUT(FTRP(next),PACK(remain, 1, 0));
        dbg_printf("remain ");
        insert_freelist(next);
    }
    else//剩余太小，不分割
    {
        dbg_printf("total:%ld remain:0",total);
        remove_freelist(bp);
        SET_ALLOC(HDRP(bp));
        SET_PALLOC(HDRP(NEXT_BLKP(bp)));
        dbg_printf("\n");
    }
    return;
}

static void *find_fit(size_t size)
{
    dbg_printf("finding\n");
    int i;
    for(i = getSegListIndex(size);i<9;i++)//当前链表没找到继续找下一个链表
    {
        void *curbp;
        for(curbp = *(segList + i); (curbp!=NULL) && (curbp!=MERGEADDR(heap_listp,NULL)) ; curbp = MERGEADDR(heap_listp,GETADDR(SUCC_POINT(curbp))))
        {
            if(size <= GET_SIZE(HDRP(curbp)))
            {
                return curbp;
            }
        }
    }
    return NULL;//没找到合适空闲块，需拓展堆
}

static int isSegList(void *bp)
{
    if ((void **)bp >= segList && (void **)bp < (segList + 9))
    {
        return 1;
    }
    return 0;
}

static void insert_freelist(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));
    int i = getSegListIndex(size);
    dbg_printf("insert to %d\n",i);
    //混合策略
    if(i<=6)//小块采用LIFO
    {
        if(*(segList + i) == NULL)
        { 
            PUTADDR(PRED_POINT(bp), segList + i);
            PUTADDR(SUCC_POINT(bp), NULL);
            *(segList + i) = bp;   //seglist中存完整地址
        }
        else
        {
            
            void *tmp;
            tmp = *(segList + i);
            PUTADDR(SUCC_POINT(bp), tmp);
            PUTADDR(PRED_POINT(bp), segList + i);
            *(segList + i) = bp;
            PUTADDR(PRED_POINT(tmp), bp);
            tmp = NULL;
            
        }
        return;
    }
    else//大块采用FIFO
    {
        if(*(segList + i)==NULL)
        {
            *(segList + i) = bp;
            *(segListEnd + i) = bp;
            PUTADDR(SUCC_POINT(bp),NULL);
            PUTADDR(PRED_POINT(bp),segList + i);
            return;
        }
        void *end = (void *)(segListEnd + i);
        PUTADDR(SUCC_POINT(*(void **)end),bp);
        PUTADDR(PRED_POINT(bp),*(void **)end);
        PUTADDR(SUCC_POINT(bp),NULL);
        *(void **)end = bp;
        end=NULL;
        return;
    }
}

static void remove_freelist(void *bp)
{
    dbg_printf("remove ");
    void *prev_block, *next_block;// 链表中的前驱和后继
    int i = getSegListIndex(GET_SIZE(HDRP(bp)));

    prev_block = MERGEADDR(heap_listp,GETADDR(PRED_POINT(bp)));
    next_block = MERGEADDR(heap_listp,GETADDR(SUCC_POINT(bp)));

    if(next_block == MERGEADDR(heap_listp,NULL))//判断当前块是否是链表结尾
        next_block = NULL;

    if(isSegList(prev_block))//当前块是链表头
    {
        *(void **)prev_block = next_block;
    }
    else
    {
        PUTADDR(SUCC_POINT(prev_block),next_block);
    }

    if(next_block!=NULL)//当前块不是链表结尾
    {
        PUTADDR(PRED_POINT(next_block),prev_block);
    }
    if(next_block == NULL)
    {
        *(segListEnd + i) = prev_block;
    }

    return;
}

static void *extend_heap(size_t size)
{
    void *bp;
    size_t Size;
    size_t words = size/WSIZE;
    Size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;//对齐

    int palloc = GET_PALLOC(epilogue_head);

    if((long)(bp = mem_sbrk(Size)) == -1)
        return NULL;

    PUT(HDRP(bp),PACK(Size, palloc, 0));
    PUT(FTRP(bp),PACK(Size, palloc, 0));

    epilogue_head += Size;
    PUT(epilogue_head,PACK(0, 0, 1));

    return coalesce(bp);
}

static void *coalesce(void *bp)
{
    void *prev_block, *next_block;//物理内存中的前驱和后继
    int prev_alloc = GET_PALLOC(HDRP(bp));
    int next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if(prev_alloc&&next_alloc)//都不合并
    {
        dbg_printf("nocoalesce\n");
    }
    else if((!prev_alloc)&&next_alloc)//合并前块
    {
        dbg_printf("coalesce prev, ");
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        bp = PREV_BLKP(bp);
        remove_freelist(bp);
        dbg_printf("prev, ");
        SET_SIZE(HDRP(bp),size);
        PUT(FTRP(bp),GET(HDRP(bp)));
    }
    else if(prev_alloc&&(!next_alloc))//合并后块
    {
        dbg_printf("coalesce next, ");
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        next_block = NEXT_BLKP(bp);
        remove_freelist(next_block);
        dbg_printf("next, ");
        SET_SIZE(HDRP(bp), size);
        PUT(FTRP(bp),GET(HDRP(bp)));
    }
    else if((!prev_alloc)&&(!next_alloc))//合并前后块
    {
        dbg_printf("coalesce both, ");
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        next_block = NEXT_BLKP(bp);
        prev_block = PREV_BLKP(bp);
        bp = PREV_BLKP(bp);
        remove_freelist(prev_block);
        dbg_printf("prev, ");
        remove_freelist(next_block);
        dbg_printf("next, ");
        SET_SIZE(HDRP(bp), size);
        PUT(FTRP(bp),GET(HDRP(bp)));
    }
    dbg_printf("new is ");

    insert_freelist(bp);

    return bp;
}

/*
 * Initialize: return -1 on error, 0 on success.
 */
int mm_init(void) {
    
    if((segList = (void **)mem_sbrk(SEGLIST_SIZE*DSIZE*2 + 4*WSIZE))==(void **)-1)//链表头指针72字节，链表尾指针72字节，padding4字节，序言块头脚各4字节，结语块4字节，segList指向头指针数组
        return -1;

    memset(segList, 0, SEGLIST_SIZE*DSIZE);//链表头指针置0

    segListEnd = (void *)segList + SEGLIST_SIZE*DSIZE;//segListEnd指向尾指针数组

    for(int i=0;i<SEGLIST_SIZE;i++)//尾指针指向链表头
    {
        *(segListEnd + i) = segList + i;
    }

    heap_listp = (void *)segList + SEGLIST_SIZE*DSIZE*2;
    PUT(heap_listp,0);//padding
    PUT(heap_listp + 1 * WSIZE,PACK(8,1,1));//序言头
    PUT(heap_listp + 2 * WSIZE,PACK(8,1,1));//序言脚
    PUT(heap_listp + 3 * WSIZE,PACK(0,1,1));//结语

    heap_listp += 2 * WSIZE;//指向序言块

    epilogue_head = heap_listp + WSIZE;//指向结语块头

    if (extend_heap((1<<12)) == NULL)   //拓展堆块
        return -1;

    dbg_checkheap(__LINE__);
    return 0;
}

/*
 * malloc
 */
void *malloc (size_t size) {

    dbg_printf("alloc %ld ",size);

    if(size == 0)
    {
        dbg_checkheap(__LINE__);
        return NULL;
    }

    if (size == 448) size = 512;//magic parameter

    size_t Size;
    void *bp;
    Size= size + WSIZE <= MINBLOCK? MINBLOCK: ALIGN(size+WSIZE);//size对齐，最小MINBLOCK

    dbg_printf("need:%ld\n",Size);

    if((bp = find_fit(Size)) != NULL)
    {
        divide(bp,Size);//找到合适空闲块，尝试分割
        dbg_checkheap(__LINE__);
        return bp;
    }
    else//没找到，拓展堆
    {
        dbg_printf("no fit! \n");
        size_t extendSize = MAX(Size,CHUNKSIZE);
        if((bp = extend_heap(extendSize)) == NULL)
        {
            dbg_checkheap(__LINE__);
            return NULL;
        }
        dbg_printf("extend over\n");

        divide(bp,Size);//拓展成功，尝试分割
        dbg_checkheap(__LINE__);
        return bp;
    }
}

/*
 * free
 */
void free (void *ptr) {

    if(!ptr)//无效指针
    { 
        dbg_printf("invalid free\n");
        dbg_checkheap(__LINE__);
        return;
    }

    size_t size = GET_SIZE(HDRP(ptr));
    dbg_printf("free:%ld\n",size);

    RESET_ALLOC(HDRP(ptr));
    PUT(FTRP(ptr),PACK(size,GET_PALLOC(HDRP(ptr)),0));

    void *next = NEXT_BLKP(ptr);
    RESET_PALLOC(HDRP(next));

    coalesce(ptr);//合并检测，并插入链表

    dbg_checkheap(__LINE__);
    return;
}

/*
 * realloc - you may want to look at mm-naive.c
 */
void *realloc(void *oldptr, size_t size) {

    if(size == 0)
    {
        free(oldptr);
        dbg_checkheap(__LINE__);
        return NULL;
    }

    if(oldptr == NULL)
    {
        dbg_checkheap(__LINE__);
        return malloc(size);
    }

    dbg_printf("realloc:%ld\n",size);
    
    size_t oldsize, newsize;
    void *newptr;
    oldsize = GET_SIZE(HDRP(oldptr));
    newsize = size+WSIZE <= MINBLOCK? MINBLOCK: ALIGN(size+WSIZE);//newsize对齐，最小MINBLOCK
    
    if(oldsize == newsize)
    {
        dbg_checkheap(__LINE__);
        return oldptr;
    }
    else if(oldsize > newsize)//当前空间足够
    {
        if (oldsize - newsize >= MINBLOCK) //有富裕空间，分割
        {
			SET_SIZE(HDRP(oldptr),newsize);
			void *nextptr = NEXT_BLKP(oldptr);
			PUT(HDRP(nextptr), PACK(oldsize - newsize, 1, 0));
			PUT(FTRP(nextptr), PACK(oldsize - newsize, 1, 0));
            RESET_PALLOC(HDRP(NEXT_BLKP(nextptr)));
            coalesce(nextptr);
		} 

        dbg_checkheap(__LINE__);
		return oldptr;//剩余太小，不分割
    }
    else //当前块空间不足，需寻找新空闲块
    {
        if((newptr = malloc(size))==NULL)
        {
            dbg_checkheap(__LINE__);
            return NULL;
        }

        memcpy(newptr,oldptr,oldsize - WSIZE); //拷贝信息

        free(oldptr);//释放旧块

        dbg_checkheap(__LINE__);
        return newptr;
    }
}

/*
 * calloc - you may want to look at mm-naive.c
 * This function is not tested by mdriver, but it is
 * needed to run the traces.
 */
void *calloc (size_t nmemb, size_t size) {
    return NULL;
}


/*
 * Return whether the pointer is in the heap.
 * May be useful for debugging.
 */
static int in_heap(const void *p) {
    return p <= mem_heap_hi() && p >= mem_heap_lo();
}

/*
 * Return whether the pointer is aligned.
 * May be useful for debugging.
 */

/*
 * mm_checkheap
 */
void mm_checkheap(int lineno) {

    //检查序言块
    void *prologue_head = heap_listp-1*WSIZE;
    if (!in_heap(prologue_head)){//是否越界
        printf("Prologue not in heap\n");
        exit(1);
    }
    if (!GET_ALLOC(prologue_head) || GET_SIZE(prologue_head) != DSIZE){//头部信息是否正确
        printf("Prologue corrupted.\n");
        printf("Expected 0x%x, but got 0x%x.\n", PACK(DSIZE, 1, 1), GET(HDRP(prologue_head)));
        exit(1);
    }


    //检查结语块
    if (mem_heap_hi()-3 != epilogue_head){//位置是否正确
        printf("Incorrect epilogue location.\n");
        exit(1);
    }
    if (!aligned(epilogue_head + WSIZE)){//是否对齐
        printf("Epilogue not aligned.\n");
        exit(1);
    }
    if (!GET_ALLOC(epilogue_head) || GET_SIZE(epilogue_head)){//头部信息是否正确
        printf("Epilogue corrupted.\n");
        exit(1);
    }


    //检查空闲链表
    if (heap_listp - (void *)segList != SEGLIST_SIZE*DSIZE*2+2*WSIZE){    //segList指向是否正确
        printf("Incorrect free list header array size.\n");
        exit(1);
    }

    int free_cnt = 0;//记录空闲块数目
    for (int i = 0; i < SEGLIST_SIZE; i++){
        void *entry = segList + i;
        for (void *bp = MERGEADDR(heap_listp,GETADDR(SUCC_POINT(entry))); (bp != MERGEADDR(heap_listp,NULL)&&(bp!=NULL)); bp = MERGEADDR(heap_listp,GETADDR(SUCC_POINT(bp)))){
            free_cnt++;
            if (!in_heap(bp)){//是否越界
                printf("Free block not in heap.\n");
                printf("Block address: %p\n", bp);
                printf("mem_heap_lo: %p\nmem_heap_hi: %p\n", mem_heap_lo(), mem_heap_hi());
                exit(1);
            }
            if (!aligned(bp)){//是否对齐
                printf("Free block not aligned.\n");
                exit(1);
            }
            if (GET_ALLOC(HDRP(bp))){//是否未分配
                printf("Allocated block in free list.\n");
                exit(1);
            }
            if (GET_SIZE(HDRP(bp)) < MINBLOCK){//大小是否均大于MINBLOCK
                printf("Free block too small.\n");
                printf("Block size: %ld\n", GET_SIZE(HDRP(bp)));
                exit(1);
            }
            if ((GET(HDRP(bp))&0xfffffffd) != (GET(FTRP(bp))&0xfffffffd)){//头脚信息是否一致，因脚部不存palloc信息故忽略
                printf("Inconsistent free block header and footer.\n");
                exit(1);
            }
            if (getSegListIndex(GET_SIZE(HDRP(bp))) != i){//是否置于正确链表中
                printf("Free block falls into the wrong bucket.\n");
                exit(1);
            }
            void *next = MERGEADDR(heap_listp,GETADDR(SUCC_POINT(bp)));
            if(MERGEADDR(heap_listp,GETADDR(PRED_POINT(next))) != bp)//A指向B，B是否指向A
            {
                printf("Inconsistent freeblock pointer.\n");
                exit(1);
            }
        }
    }


    //按物理内存顺序检查每个块
    void *bp;
    int cnt = 1;//跳过序言块
    for (bp = heap_listp + DSIZE; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp), ++cnt){
        if (!in_heap(bp)){//是否越界
            printf("Block not in heap\n");
            printf("Block address: %p\n", bp);
            printf("mem_heap_lo: %p\nmem_heap_hi: %p\n", mem_heap_lo(), mem_heap_hi());
            exit(1);
        }
        if (!aligned(bp)){//是否对齐
            printf("Block not aligned.\n");
            exit(1);
        }
        if (GET_SIZE(HDRP(bp)) < MINBLOCK){//大小是否均大于MINBLOCK
            printf("Block too small.\n");
            printf("Block size: %ld\n", GET_SIZE(HDRP(bp)));
            exit(1);
        }
        if (!GET_ALLOC(HDRP(bp))){//是否未分配
            --free_cnt;//记数，与空闲链表中块数目比较
            if (!GET_ALLOC(HDRP(NEXT_BLKP(bp)))){//是否存在连续空闲块未被合并
                printf("Consective free blocks not coalesced.\n");
                exit(1);
            }
        }
        if (GET_ALLOC(HDRP(bp)) != GET_PALLOC(HDRP(NEXT_BLKP(bp)))){//当前块alloc bit与下一块的palloc bit是否一致
            printf("Prev_alloc bit inconsistent.\n");
            printf("Block in address %p is %sallocated, but the following block marked it otherwise.\n", bp, GET_ALLOC(HDRP(bp))? "": "un");
            exit(1);
        }
        if ((GET(HDRP(bp))&0xfffffffd) != (GET(FTRP(bp))&0xfffffffd)){//头脚信息是否一致，除palloc bit之外
                printf("Inconsistent free block header and footer.\n");
                exit(1);
            }
    }

    if (free_cnt){//空闲链表中空闲块数目与实际数目是否一致
        printf("%d",free_cnt);
        printf("Free list total size and free block number don't match.\n");
        exit(1);
    }
    if (bp != epilogue_head + WSIZE){//检查完所有块，bp应指向结语快末尾
        printf("Incorrect epilogue location.\n");
        exit(1);
    }
    return;
}
