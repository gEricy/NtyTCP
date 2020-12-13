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





#ifndef __NTY_BUFFER_H__
#define __NTY_BUFFER_H__

#include <stdint.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <errno.h>

#include "nty_queue.h"
#include "nty_tree.h"

#include "nty_mempool.h"


enum rb_caller
{
	AT_APP, 
	AT_MTCP
};


#define MAX(a, b) ((a)>(b)?(a):(b))
#define MIN(a, b) ((a)<(b)?(a):(b))

/*----------------------------------------------------------------------------*/
typedef struct _nty_sb_manager
{
	size_t chunk_size;
	uint32_t cur_num;
	uint32_t cnum;
	struct _nty_mempool *mp;
	struct _nty_sb_queue *freeq;

} nty_sb_manager;

typedef struct _nty_send_buffer {
	unsigned char *data;
	unsigned char *head;

	uint32_t head_off;
	uint32_t tail_off;
	uint32_t len;
	uint64_t cum_len;
	uint32_t size;

	uint32_t head_seq;
	uint32_t init_seq;
} nty_send_buffer;

#ifndef _INDEX_TYPE_
#define _INDEX_TYPE_
typedef uint32_t index_type;
typedef int32_t signed_index_type;
#endif


typedef struct _nty_sb_queue {
	index_type _capacity;
	volatile index_type _head;
	volatile index_type _tail;

	nty_send_buffer * volatile * _q;
} nty_sb_queue;

#define NextIndex(sq, i)	(i != sq->_capacity ? i + 1: 0)
#define PrevIndex(sq, i)	(i != 0 ? i - 1: sq->_capacity)
#define MemoryBarrier(buf, idx)	__asm__ volatile("" : : "m" (buf), "m" (idx))



/** rb frag queue **/

typedef struct _nty_rb_frag_queue {
	index_type _capacity;
	volatile index_type _head;
	volatile index_type _tail;

	struct _nty_fragment_ctx * volatile * _q;
} nty_rb_frag_queue;


/** ring buffer **/

typedef struct _nty_fragment_ctx {
	uint32_t seq;
	uint32_t len:31,
			 is_calloc:1;
	struct _nty_fragment_ctx *next;
} nty_fragment_ctx;

typedef struct _nty_ring_buffer {
	u_char *data;
	u_char *head;

	uint32_t head_offset;
	uint32_t tail_offset;

	int merged_len;
	uint64_t cum_len;
	int last_len;
	int size;

	uint32_t head_seq;
	uint32_t init_seq;

	nty_fragment_ctx *fctx;
} nty_ring_buffer;

typedef struct _nty_rb_manager {
	size_t chunk_size;
	uint32_t cur_num;
	uint32_t cnum;

	nty_mempool *mp;
	nty_mempool *frag_mp;

	nty_rb_frag_queue *free_fragq;
	nty_rb_frag_queue *free_fragq_int;
	
} nty_rb_manager;



typedef struct _nty_stream_queue
{
	index_type _capacity;
	volatile index_type _head;
	volatile index_type _tail;

	struct _nty_tcp_stream * volatile * _q;
} nty_stream_queue;

typedef struct _nty_stream_queue_int
{
	struct _nty_tcp_stream **array;
	int size;

	int first;
	int last;
	int count;

} nty_stream_queue_int;


nty_sb_manager *nty_sbmanager_create(size_t chunk_size, uint32_t cnum);
nty_rb_manager *RBManagerCreate(size_t chunk_size, uint32_t cnum);


nty_stream_queue *CreateStreamQueue(int capacity);


nty_stream_queue_int *CreateInternalStreamQueue(int size);
void DestroyInternalStreamQueue(nty_stream_queue_int *sq);


nty_send_buffer *SBInit(nty_sb_manager *sbm, uint32_t init_seq);
void SBFree(nty_sb_manager *sbm, nty_send_buffer *buf);
size_t SBPut(nty_sb_manager *sbm, nty_send_buffer *buf, const void *data, size_t len);
int SBEnqueue(nty_sb_queue *sq, nty_send_buffer *buf);
size_t SBRemove(nty_sb_manager *sbm, nty_send_buffer *buf, size_t len);


size_t RBRemove(nty_rb_manager *rbm, nty_ring_buffer* buff, size_t len, int option);
int RBPut(nty_rb_manager *rbm, nty_ring_buffer* buff, 
	   void* data, uint32_t len, uint32_t cur_seq);
void RBFree(nty_rb_manager *rbm, nty_ring_buffer* buff);

int StreamInternalEnqueue(nty_stream_queue_int *sq, struct _nty_tcp_stream *stream);
struct _nty_tcp_stream *StreamInternalDequeue(nty_stream_queue_int *sq);


/*** ******************************** sb queue ******************************** ***/


nty_sb_queue *CreateSBQueue(int capacity);
int StreamQueueIsEmpty(nty_stream_queue *sq);


nty_send_buffer *SBDequeue(nty_sb_queue *sq);

nty_ring_buffer *RBInit(nty_rb_manager *rbm, uint32_t init_seq);


struct _nty_tcp_stream *StreamDequeue(nty_stream_queue *sq);
int StreamEnqueue(nty_stream_queue *sq, struct _nty_tcp_stream *stream);

void DestroyStreamQueue(nty_stream_queue *sq);



#endif



