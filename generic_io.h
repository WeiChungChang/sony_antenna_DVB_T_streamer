/* Copyright 2017-2018 Noovo Crop.
 * This software it the property of Noovo Crop.
 * You have to accept the terms in the license file before use
 *
 * Created by JohnChang      2017-06-12
 * Modified by 
 *
 * Copyright (c) 2017-2018 Noovo Crop.  All rights reserved
 */

#ifndef _GENERIC_IO_H_
#define _GENERIC_IO_H_ 1

#include "allocation.h"
#include "threadqueue.h"

#include "atom_variable.h"

typedef int (*read_data_funct)(void *op, int8_t *dist, int max);

typedef struct _input_param {
	read_data_funct read_data;
	void            *opData;
	Allocator       *allocator;
	threadqueue     *dataQueue;

	atom_v			stop;
	struct timespec *timeout;
} input_param;

typedef struct _output_param {
	threadqueue 	*dataQueue;
	Allocator       *allocator;
	Allocation      *current_buf;
} output_param;

void* generic_input_thread_loop(void *param);

#endif
