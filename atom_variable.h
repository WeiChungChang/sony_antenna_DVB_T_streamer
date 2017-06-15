/* Copyright 2017-2018 Noovo Crop.
 * This software it the property of Noovo Crop.
 * You have to accept the terms in the license file before use
 *
 * Created by JohnChang      2017-06-12
 * Modified by 
 *
 * Copyright (c) 2017-2018 Noovo Crop.  All rights reserved
 */


#ifndef _ATOM_VARIABLE_H_
#define _ATOM_VARIABLE_H_ 1

#include <pthread.h>

typedef struct atom_v
{
	pthread_mutex_t mutex;
	int val;
} atom_v;

#define _LOCK(atom_v) \
	({	\
		pthread_mutex_lock(&((atom_v)->mutex));	\
	})

#define _UNLOCK(atom_v) \
	({	\
		pthread_mutex_unlock(&((atom_v)->mutex));	\
	})

#define GET(atom_v) \
	({	\
		int r;	\
		_LOCK(atom_v);	\
		r = ((atom_v)->val); \
		_UNLOCK(atom_v);	\
		r;	\
	})

#define SET(atom_v, new_value) \
	({	\
		_LOCK(atom_v);	\
		((atom_v)->val) = (new_value); \
		_UNLOCK(atom_v);	\	
	})


#define INIT_ATOM_V(atom_v, init_value) \
	({	\
		((atom_v)->val) = (init_value);	\
		pthread_mutex_init(&((atom_v)->mutex), NULL);	\
	})

#endif
