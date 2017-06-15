/* Copyright 2017-2018 Noovo Crop.
 * This software it the property of Noovo Crop.
 * You have to accept the terms in the license file before use
 *
 * Created by JohnChang      2017-06-12
 * Modified by 
 *
 * Copyright (c) 2017-2018 Noovo Crop.  All rights reserved
 */

#include "generic_io.h"
#include <sys/time.h>

void* generic_input_thread_loop(void *param) 
{
	Allocation   *allocation          = NULL;
	input_param   *iparam             = (input_param*)param;
	atom_v        *stop               = &(iparam->stop);
#if 0
	struct        timespec abstimeout = {0};
	
    if (iparam->timeout) {
		struct timespec *timeout = iparam->timeout;
        struct timeval now;

        gettimeofday(&now, NULL);
        abstimeout.tv_sec = now.tv_sec + timeout->tv_sec;
        abstimeout.tv_nsec = (now.tv_usec * 1000) + timeout->tv_nsec;
        if (abstimeout.tv_nsec >= 1000000000) {
            abstimeout.tv_sec++;
            abstimeout.tv_nsec -= 1000000000;
        }
    }
#endif

	allocation = allocate(iparam->allocator);
	if (!allocation) {
		/*cannot get new allocation!*/
		printf("[%s:%s():%d] cannot get new allocation!\n", __FILE__, __FUNCTION__, __LINE__);
		goto error;
	}

	while (1) {
		if(GET(stop)) {
			printf("[%s:%s():%d] main thread inform to stop!\n", __FILE__, __FUNCTION__, __LINE__);
			break;
		}
		
		/*blocking operation*/
		int received = (iparam->read_data)(iparam->opData, WRITE_POS(allocation), FREE_SPACE_SZ(allocation));
		if (received == 0) /*EOS*/ {
			printf("[%s:%s():%d] EOS\n", __FILE__, __FUNCTION__, __LINE__);
			goto exit;
		} else if (received < 0) {
			printf("[%s:%s():%d] read_data failed\n", __FILE__, __FUNCTION__, __LINE__);
			goto error;
		} else {
			CONFIRM_WRITE(allocation, received);
#if 0
			printf("[%s:%s():%d] received %d, free = %d full? %d\n", __FILE__, __FUNCTION__, __LINE__, received, FREE_SPACE_SZ(allocation),
				FULL(allocation));
#endif
		}

		if ((FULL(allocation))) {
			thread_queue_add(iparam->dataQueue, (void*)(allocation), 0/*useless*/);

			//printf("[%s:%s():%d] %d\n", __FILE__, __FUNCTION__, __LINE__, thread_queue_length(iparam->dataQueue));
			allocation = allocate(iparam->allocator);
			if (!allocation) {
				/*cannot get new allocation!*/
				printf("[%s:%s():%d] cannot get new allocation!\n", __FILE__, __FUNCTION__, __LINE__);
				break;
			}

		}		
	}
error:
exit:
	return NULL;
}
