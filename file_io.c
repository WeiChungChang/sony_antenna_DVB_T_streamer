int ff_read_packet_impl (void *opaque)
{
	output_param *param  = (output_param*)(opaque);
	int          read_sz = buf_size;
	int ret;

	while(1) {
		if (!(param->current_buf) || EMPTY(param->current_buf)) {
			if (param->current_buf) {
				ret = release(param->allocator, param->current_buf);
				if (ret < 0) {
					printf("[%s:%s():%d]: realease failed\n", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
			}
			threadmsg msg = {0};
			threadqueue *dataQueue = param->dataQueue;
			ret = thread_queue_get(dataQueue, NULL, &msg); /*blocking wait*/
			if (ret < 0) {
				printf("[%s:%s():%d]: thread_queue_get failed\n", __FILE__, __FUNCTION__, __LINE__);
				break;
			}			
			param->current_buf = (Allocation*)(msg.data);
		} else {
			if (AVAILABLE_DATA_SZ(param->current_buf) >= buf_size) {
				COPY_TO(param->current_buf, buf, buf_size);
				return read_sz;
			} else {
				int cp_sz = AVAILABLE_DATA_SZ(param->current_buf);
				COPY_TO(param->current_buf, buf, cp_sz);
				buf += cp_sz;
				buf_size -= cp_sz;
			}
		}
	}
    return 0;
}
