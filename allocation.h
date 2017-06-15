#ifndef _allocation_h_
#define _allocation_h_
#include <pthread.h>
#include <stdio.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include "allocation.h"
#include <sys/queue.h>

#define mutex_t pthread_mutex_t
#define cond_t  pthread_cond_t

#define mutex_init(m)  pthread_mutex_init((m), NULL)

#define mutex_lock     pthread_mutex_lock
#define mutex_unlock   pthread_mutex_unlock
#define mutex_destroy  pthread_mutex_destroy

#define cond_init(c)   pthread_cond_init((c), NULL)
#define cond_signal    pthread_cond_signal
#define cond_broadcast pthread_cond_broadcast
#define cond_wait      pthread_cond_wait
#define cond_destroy   pthread_cond_destroy

#define INIT_ALLOCATOR_NUM   64
#define MAX_ALLOCATOR_LIMIT  64
#define ALLOCATION_UNIT 32 * 1024 * 188 /*4096 packets*/

#define CAPACITY(pAllocation)               ((pAllocation)->capacity)
#define FREE_SPACE_SZ(pAllocation)          (((pAllocation)->capacity) - ((pAllocation)->wp))
#define AVAILABLE_DATA_SZ(pAllocation)      (((pAllocation)->wp) - ((pAllocation)->rp))
#define BUFFER_HEAD(pAllocation)            ((void*)((pAllocation)->data))
#define WRITE_POS(pAllocation)              ((void*)(((pAllocation)->data) + ((pAllocation)->wp)))
#define READ_POS(pAllocation)               ((void*)(((pAllocation)->data) + ((pAllocation)->rp)))
#define FULL(pAllocation)                   (((pAllocation)->wp) == CAPACITY(pAllocation))
#define EMPTY(pAllocation)                   (((pAllocation)->rp) == ((pAllocation)->wp))
#define CONFIRM_WRITE(pAllocation, sz)      (((pAllocation)->wp) += (sz))
#define COPY_FROM(pAllocation/*source*/, source/*source*/, size)  \
	({	\
		memcpy(WRITE_POS((pAllocation)), (source), (size));	\
		(((pAllocation)->wp) += (size));	\
	})

#define COPY_TO(pAllocation/*source*/, dist/*dist*/, size)  \
		({	\
			memcpy((dist), READ_POS((pAllocation)), (size)); \
			(((pAllocation)->rp) += (size));	\
		})


typedef struct Allocation {
	void*         data;
	int           capacity;
	int           wp;
	int           rp;
} Allocation;

typedef struct Allocator {
	pthread_mutex_t             mutex;
	Allocation                  **availableAllocations;
	int                         allocatedCount;
	int                         availableCount;
	int                         availableAllocationSize;

	Allocation**                _allocatedAllocations;
	int                         _allocatedAllocationsCount;
} Allocator;

void init_allocator(Allocator* allocator, int capacity);

void terminate_allocator(Allocator* allocator);

Allocation* allocate(Allocator* allocator);

int release(Allocator* allocator, Allocation* allocation);

#endif

