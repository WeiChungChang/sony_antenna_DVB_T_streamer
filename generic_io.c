#include "generic_io.h"

void* generic_input_thread_loop(void *param) 
{
	Allocation   *allocation = NULL;
	input_param   *iparam    = (input_param*)param;
	atom_v        *stop      = &(iparam->stop);
	
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
