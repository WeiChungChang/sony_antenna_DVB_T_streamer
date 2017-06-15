#include <stdio.h>
#include <sys/time.h>
#include "allocation.h"
#include "threadqueue.h"

/*5 seconds*/
#define MAX_TIME_OUT_SEC 5

int file_store_loop (const char* fname, Allocator *allocator, threadqueue *dataQueue)
{
	Allocation   *current_buf = NULL;
	FILE         *fhandler    = NULL;
	struct timespec timeout   = {0};
	int ret;

	timeout.tv_sec = MAX_TIME_OUT_SEC;

	fhandler = fopen(fname, "wb");
	if(fhandler == NULL) {
		fprintf(stderr, "[%s:%s():%d]: open file failed! fname = %s!\n", __FILE__, __FUNCTION__, __LINE__, fname);
		return -1;
	}

	while(1) {
		if (!(current_buf) || EMPTY(current_buf)) {
			if (current_buf) {
				ret = release(allocator, current_buf);
				if (ret < 0) {
					printf("[%s:%s():%d]: realease failed\n", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
			}
			threadmsg msg = {0};
			ret = thread_queue_get(dataQueue, &timeout, &msg); /*blocking wait*/
			if (ret == ETIMEDOUT || ret < 0) {
				if (ret == ETIMEDOUT) {
					printf("[%s:%s():%d]: timeout; maybe EOS, ret = %d\n", __FILE__, __FUNCTION__, __LINE__, ret);
				} else if (ret == -2) {
					printf("[%s:%s():%d]: stop\n", __FILE__, __FUNCTION__, __LINE__);
				} else {
					printf("[%s:%s():%d]: thread_queue_get failed\n", __FILE__, __FUNCTION__, __LINE__);
				}
				break;
			}			
			current_buf = (Allocation*)(msg.data);
		} else {
			/*store to file*/
			size_t w_sz = fwrite(READ_POS(current_buf), 1, AVAILABLE_DATA_SZ(current_buf), fhandler);
			if (w_sz != AVAILABLE_DATA_SZ(current_buf)) {
				printf("[%s:%s():%d]: R/W sz mismatch!\n", __FILE__, __FUNCTION__, __LINE__);
			}
			CONFIRM_READ(current_buf, w_sz);
			fflush(fhandler);
		}
	}
	
	fclose(fhandler);
    return 1;
}
