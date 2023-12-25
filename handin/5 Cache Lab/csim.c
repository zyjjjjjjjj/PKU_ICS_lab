// 
#include "cachelab.h"
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

extern char *optarg;
extern int optind, opterr, optopt;

typedef struct cache_Block{
    int validBit; //valid bit
    int tag;       //tag bit
    int lru;       //LRU counter
} cacheBlock;
cacheBlock** cache;
int visitOrder;
int s, E, b, printTraceInfo;
int hitC = 0, missC = 0, evictionC = 0;

void initCache()
{
    int S = pow(2, s);
    visitOrder = 1;
    cache = (cacheBlock**)malloc(S*sizeof(cacheBlock*));
	for (int i = 0; i < S; i++) {
		cache[i] = (cacheBlock*)malloc(E*sizeof(cacheBlock));
	}
    for(int i = 0;i < S;i++){
      for(int j = 0;j < E;j++){
         cache[i][j].validBit=0;    
         cache[i][j].tag=0;
      }
   }
}

void releaseCache() {   //free cache memory
    int S = (int)pow(2, s);
    for (int i = 0; i < S; i++) {
        free(cache[i]);
    }
    free(cache);
}

void cacheOP(unsigned long addr)
{
    int addrSet = (addr >> b) & ((-1U) >> (64 - s));
    int addrTag = (addr >> (b + s));
    int hitFlag = 0;
    int replaceFlag = -1;
    int lruFlag = 0x7fffffff;
    for (int i = 0; i < E; i++)
    {
        cacheBlock tmp = cache[addrSet][i];
        if (tmp.validBit != 1)
        {
            lruFlag = 0;
            replaceFlag = i;
            continue;
        }
        if (tmp.tag == addrTag)
        {
            cache[addrSet][i].lru = visitOrder++;
            hitFlag = 1;
            break;
        }
        else
        {
            if (tmp.lru < lruFlag)
            {
                lruFlag = tmp.lru;
                replaceFlag = i;
            }
        }
    }
    if (hitFlag)
    {
        hitC++;
        if (printTraceInfo)
            printf(" hit");
    }
    else
    {
        missC++;
        if (printTraceInfo)
            printf(" miss");
        if (cache[addrSet][replaceFlag].validBit == 1)
        {
            evictionC++;
            if (printTraceInfo)
                printf(" eviction");
        }
        cache[addrSet][replaceFlag].validBit = 1;
        cache[addrSet][replaceFlag].tag = addrTag;
        cache[addrSet][replaceFlag].lru = visitOrder++;
    }
}

void readFile(FILE* pFile)
{
    char opt;
    long unsigned addr;
    int size;
    while (fscanf(pFile, " %c %lx,%d\n", &opt, &addr, &size) != EOF)
    {
        if (printTraceInfo)
            printf("%c %lx,%d", opt, addr, size);
        switch (opt)
        {
        case 'M':cacheOP(addr);
        case 'L':
        case 'S':cacheOP(addr);
        }
        if (printTraceInfo)
           printf("\n");
    }
    fclose(pFile);
}

void usage() {
    printf("Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\n");
    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -v         Optional verbose flag.\n");
    printf("  -s <num>   Number of set index bits.\n");
    printf("  -E <num>   Number of lines per set.\n");
    printf("  -b <num>   Number of block offset bits.\n");
    printf("  -t <file>  Trace file.\n\n");
    printf("Examples:\n");
    printf("  linux>  ./csim-ref -s 4 -E 1 -b 4 -t traces/yi.trace\n");
    printf("  linux>  ./csim-ref -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
}

int main(int argc, char *argv[])
{
    int opt;
    char file_name[1000];
    FILE* pFile = NULL;
	while(-1 != (opt = (getopt(argc, argv, "hvs:E:b:t:"))))
	{
		switch(opt)
		{
			case 'h':
				usage();
				break;
			case 'v':
				printTraceInfo = 1;
				break;
			case 's':
				s = atoi(optarg);
				break;
			case 'E':
				E = atoi(optarg);
				break;
			case 'b':
				b = atoi(optarg);
				break;
			case 't':
				strcpy(file_name, optarg);
				break;
			default:
				usage();
				break;
		}
	}
	if(s<=0 || E<=0 || b<=0 || file_name==NULL) // 如果选项参数不合格就退出
	        return -1;
    pFile = fopen(file_name, "r");
	if(pFile == NULL)
	{
		printf("open error");
		exit(-1);
	}
    initCache();
    readFile(pFile);
    releaseCache();
    printSummary(hitC, missC, evictionC);
    return 0;
}