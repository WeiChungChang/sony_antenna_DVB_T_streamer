/* Copyright 2017-2018 Noovo Crop.
 * This software it the property of Noovo Crop.
 * You have to accept the terms in the license file before use
 *
 * Created by JohnChang      2017-06-12
 * Modified by 
 *
 * Copyright (c) 2017-2018 Noovo Crop.  All rights reserved
 */

#include "allocation.h"
#include "mmdbg.h"

static void reset_allocation(Allocation* allocation)
{
	allocation->rp = allocation->wp = 0;
}

static Allocation* new_allocation(size_t size)
{
	Allocation *allocation = malloc_dbg(sizeof(Allocation));
	if(!allocation)
		return NULL;

	allocation->data = malloc_dbg(size);
	if(!allocation->data)
		goto error;
	allocation->rp       = 0;
	allocation->wp       = 0;
	allocation->capacity = size;
	return allocation;
error:
	if(allocation)
		 free_dbg(allocation);
	if(allocation->data)
		 free_dbg(allocation->data); 
	return NULL;
}

void free_allocation(Allocation *allocation)
{
	if(!allocation)
		return;
	if (allocation->data) {
		unsigned char* ptr = (allocation->data);
		free_dbg(ptr);
	}
	free_dbg(allocation);
}

void init_allocator(Allocator* allocator, int capacity)
{
	mutex_init(&(allocator->mutex));
	allocator->availableAllocations    = malloc_dbg(capacity * sizeof(Allocation*));
	allocator->availableAllocationSize = capacity;
	allocator->allocatedCount          = 0;
	allocator->availableCount          = 0;

	allocator->_allocatedAllocationsCount  = 0;
	allocator->_allocatedAllocations       = malloc_dbg(MAX_ALLOCATOR_LIMIT * sizeof(Allocation*));
}

void terminate_allocator(Allocator* allocator)
{
	if(allocator->availableAllocations)
		free_dbg(allocator->availableAllocations);

	for (int i = 0; i < allocator->_allocatedAllocationsCount; i++) {
		Allocation *allocation = (allocator->_allocatedAllocations)[i];
		free_allocation(allocation);		
	}
	free_dbg(allocator->_allocatedAllocations);
}

Allocation* allocate(Allocator* allocator) 
{
	Allocation *allocation = NULL;

	mutex_lock (&(allocator->mutex));
	if (allocator->availableCount > 0) {
		allocation = (allocator->availableAllocations)[--allocator->availableCount];
		(allocator->availableAllocations)[allocator->availableCount] = NULL;
	} else {
		allocation = new_allocation(ALLOCATION_UNIT);
		if (!allocation) {
			printf("[%s:%s():%d] new_allocation failed!\n", __FILE__, __FUNCTION__, __LINE__);
			mutex_unlock (&(allocator->mutex));
			return NULL;
		} else {
			if (allocator->_allocatedAllocationsCount >= MAX_ALLOCATOR_LIMIT) {
				printf("[%s:%s():%d] exceeds MAX_ALLOCATOR_LIMIT!!\n", __FILE__, __FUNCTION__, __LINE__);
				mutex_unlock (&(allocator->mutex));
				return NULL;
			}
			(allocator->_allocatedAllocations)[allocator->_allocatedAllocationsCount] = allocation;
			(allocator->_allocatedAllocationsCount)++;
		}
	}
	allocator->allocatedCount++;
	reset_allocation(allocation);
	
	printf("[%s:%s():%d] alloc %d %d %d %p\n", __FILE__, __FUNCTION__, __LINE__,
		allocator->availableCount, allocator->availableAllocationSize, allocator->_allocatedAllocationsCount, pthread_self());
	mutex_unlock (&(allocator->mutex));

	return allocation;
}

int release(Allocator* allocator, Allocation* allocation) 
{
	if (!allocator || !allocation)
		return -1;

	mutex_lock (&(allocator->mutex));
	printf("[%s:%s():%d] release %d %d %d %p\n", __FILE__, __FUNCTION__, __LINE__,
		allocator->availableCount, allocator->availableAllocationSize, allocator->_allocatedAllocationsCount, pthread_self());

	if (allocator->availableCount >= allocator->availableAllocationSize) {
		allocator->availableAllocations = realloc(allocator->availableAllocations, sizeof(Allocation*) * allocator->availableAllocationSize * 2);
		if (!allocator->availableAllocations) {
			return -1;
		}
		allocator->availableAllocationSize *= 2;
    }
 
	(allocator->availableAllocations)[(allocator->availableCount)++] = allocation;
	allocator->allocatedCount -= 1;
	mutex_unlock (&(allocator->mutex));

	return 1;
}
