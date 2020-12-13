/*
 * MIT License
 *
 * Copyright (c) [2018] [WangBoJing]

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 *
 *
****       *****                                  ************
  ***        *                                    **   **    *
  ***        *         *                          *    **    **
  * **       *         *                         *     **     *
  * **       *         *                         *     **     *
  *  **      *        **                               **
  *  **      *       ***                               **
  *   **     *    ***********    *****    *****        **             *****         *  ****
  *   **     *        **           **      **          **           ***    *     **** *   **
  *    **    *        **           **      *           **           **     **      ***     **
  *    **    *        **            *      *           **          **      **      **       *
  *     **   *        **            **     *           **         **       **      **       **
  *     **   *        **             *    *            **         **               **       **
  *      **  *        **             **   *            **         **               **       **
  *      **  *        **             **   *            **         **               **       **
  *       ** *        **              *  *             **         **               **       **
  *       ** *        **              ** *             **         **               **       **
  *        ***        **               * *             **         **         *     **       **
  *        ***        **     *         **              **          **        *     **      **
  *         **        **     *         **              **          **       *      ***     **
  *         **         **   *          *               **           **     *       ****   **
*****        *          ****           *             ******           *****        **  ****
                                       *                                           **
                                      *                                            **
                                  *****                                            **
                                  ****                                           ******

 *
 */




#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>

//sudo apt-get install libhugetlbfs-dev
#include <hugetlbfs.h>

#include "nty_mempool.h"

nty_mempool *nty_mempool_create(int chunk_size, size_t total_size, int is_hugepage) {

	if (chunk_size < (int)sizeof(nty_mem_chunk)) {
		return NULL;
	}
	if (chunk_size % 4 != 0) {
		printf("nty_mempool_create --> chunk_size: %d\n", chunk_size);
		return NULL;
	}

	nty_mempool *mp = calloc(1, sizeof(nty_mempool));
	if (mp == NULL) { 
		printf("nty_mempool_create --> calloc failed\n");
		return NULL;
	}

	mp->mp_type = is_hugepage;
	mp->mp_chunk_size = chunk_size;
	mp->mp_free_chunks = (total_size + (chunk_size-1)) / chunk_size;
	mp->mp_total_chunks = mp->mp_free_chunks;

	if (is_hugepage == MEM_HUGEPAGE) {
		mp->mp_startptr = get_huge_pages(total_size, GHP_DEFAULT);
		if (!mp->mp_startptr) {
			free(mp);
			assert(0);
		}
	} else {
		int res = posix_memalign((void **)&mp->mp_startptr, getpagesize(), total_size);
		if (res != 0) {
			free(mp);
			assert(0);
		}
	}

	if (geteuid() == 0) {
		if (mlock(mp->mp_startptr, total_size) < 0) {
			
		}
	}

	mp->mp_freeptr = (nty_mem_chunk *)mp->mp_startptr;
	mp->mp_freeptr->mc_free_chunks = mp->mp_free_chunks;
	mp->mp_freeptr->next = NULL;

	return mp;
}

void nty_mempool_destory(nty_mempool *mp) {
	if (mp->mp_type == MEM_HUGEPAGE) {
		free_huge_pages(mp->mp_startptr);
	} else {
		free(mp->mp_startptr);
	}

	free(mp);
}

void *nty_mempool_alloc(nty_mempool *mp) {

	nty_mem_chunk *p = mp->mp_freeptr;

	if (mp->mp_free_chunks == 0) return NULL;

	assert(p->mc_free_chunks > 0);

	p->mc_free_chunks --;
	mp->mp_free_chunks --;

	if (p->mc_free_chunks) {
		mp->mp_freeptr = (nty_mem_chunk*)((u_char*)p + mp->mp_chunk_size);
		mp->mp_freeptr->mc_free_chunks = p->mc_free_chunks;
		mp->mp_freeptr->next = p->next;
	} else {
		mp->mp_freeptr = p->next;
	}

	return p;
}


void nty_mempool_free(nty_mempool *mp, void *p) {
	nty_mem_chunk *mcp = (nty_mem_chunk*)p;

	assert(((u_char*)p - mp->mp_startptr) % mp->mp_chunk_size == 0);

	mcp->mc_free_chunks = 1;
	mcp->next = mp->mp_freeptr;
	mp->mp_freeptr = mcp;
	mp->mp_free_chunks ++;
}


int nty_mempool_getfree_chunks(nty_mempool *mp) {
	return mp->mp_free_chunks;
}

uint32_t nty_mempool_isdanger(nty_mempool *mp) {
#define DANGER_THREADSHOLD	0.95
#define SAFE_THREADSHOLD	0.90

	uint32_t danger_num = mp->mp_total_chunks * DANGER_THREADSHOLD;
	uint32_t safe_num = mp->mp_total_chunks * SAFE_THREADSHOLD;

	if ((int)danger_num < mp->mp_total_chunks - mp->mp_free_chunks) {
		return mp->mp_total_chunks - mp->mp_free_chunks - safe_num;
	}
	return 0;
}



