//周燕居 2200017754
//对三种情况分类执行不同优化方式
/*
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */
#include <stdio.h>
#include "cachelab.h"
#include "contracts.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/*
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. The REQUIRES and ENSURES from 15-122 are included
 *     for your convenience. They can be removed if you like.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    REQUIRES(M > 0);
    REQUIRES(N > 0);

		int i, j, n, m;
        int v1,v2,v3,v4,v5,v6,v7,v8;
if (M == 32)//32*32时
{
	for (i = 0; i < N; i += 8)
		for (j = 0; j < M; j += 8)//分成8x8小块，因为cache刚好能存8行，不会冲突
			for (n = i; n < i + 8; n++)//用临时变量储存一行，避免对角线时冲突
			{
				v1 = A[n][j];
				v2 = A[n][j+1];
				v3 = A[n][j+2];
				v4 = A[n][j+3];
				v5 = A[n][j+4];
				v6 = A[n][j+5];
				v7 = A[n][j+6];			
				v8 = A[n][j+7];
				B[j][n] = v1;
				B[j+1][n] = v2;
				B[j+2][n] = v3;
				B[j+3][n] = v4;
				B[j+4][n] = v5;
				B[j+5][n] = v6;
				B[j+6][n] = v7;
				B[j+7][n] = v8;
			}
}
if (M == 64)//64*64时，cache只能存4行，若8x8分块每次写B都会冲突，在每个8*8内按4*4操作
{
	for (i = 0; i < N; i += 8)
		for (j = 0; j < M; j += 8)
		{
			for (n = i; n < i + 4; n++)//左上四分之一转置，右上四分之一转置后仍存在B的右上四分之一
			{
				v1 = A[n][j]; v2 = A[n][j+1]; v3 = A[n][j+2]; v4 = A[n][j+3];
				v5 = A[n][j+4]; v6 = A[n][j+5]; v7 = A[n][j+6]; v8 = A[n][j+7];
							
				B[j][n] = v1; B[j][n+4] = v5;
				B[j+1][n] = v2; B[j+1][n+4] = v6;
				B[j+2][n] = v3; B[j+2][n+4] = v7;
				B[j+3][n] = v4; B[j+3][n+4] = v8;
			}
			for(m = j; m < j+4; m++)//B右上四分之一平移到B左下四分之一，A左下四分之一转置存到B右上四分之一
			{
				v1 = A[i + 4][m]; 
				v2 = A[i + 5][m]; 
				v3 = A[i + 6][m]; 
				v4 = A[i + 7][m];
				v5 = B[m][i + 4]; v6 = B[m][i + 5]; v7 = B[m][i + 6]; v8 = B[m][i + 7]; 

				B[m][i+4] = v1; B[m][i+5] = v2; B[m][i+6] = v3; B[m][i+7] = v4;
				B[m+4][i] = v5; B[m+4][i+1] = v6; B[m+4][i+2] = v7; B[m+4][i+3] = v8;     
			}
			for(n=i;n<i+4;n++)//A右下四分之一转置存到B右下四分之一
			{
				v1 = A[n+4][j+4]; v2 = A[n+4][j+5]; v3 = A[n+4][j+6]; v4 = A[n+4][j+7];
				B[j+4][n+4] = v1; B[j+5][n+4] = v2; B[j+6][n+4] = v3; B[j+7][n+4] = v4;  
			}
		}
}

if(M==60)//先把左侧68*56以每8列位单位转置，再转剩下的
{
	m = M / 8 * 8;
	for (j = 0; j < m; j += 8)//8个8个转置68*56
	{
		for (i = 0; i < N; i++)
		{
			v1 = A[i][j];
			v2 = A[i][j+1];
			v3 = A[i][j+2];
			v4 = A[i][j+3];
			v5 = A[i][j+4];
			v6 = A[i][j+5];
			v7 = A[i][j+6];
			v8 = A[i][j+7];
			
			B[j][i] = v1;
			B[j+1][i] = v2;
			B[j+2][i] = v3;
			B[j+3][i] = v4;
			B[j+4][i] = v5;
			B[j+5][i] = v6;
			B[j+6][i] = v7;
			B[j+7][i] = v8;
		}			
	}	
	for (i = 0; i < N; i++)//转置右边最后4列68行
		for (j = m; j < M; j++)
		{
			v1 = A[i][j];
			B[j][i] = v1;
		}
}

    ENSURES(is_transpose(M, N, A, B));
}

/*
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started.
 */

 /*
  * trans - A simple baseline transpose function, not optimized for the cache.
  */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;
    REQUIRES(M > 0);
    REQUIRES(N > 0);

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }

    ENSURES(is_transpose(M, N, A, B));
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc);

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc);

}

/*
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

