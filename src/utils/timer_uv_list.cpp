#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern "C" {
#include "uv.h"
}
#include "../../moduel/netbus/netbus.h"
#include "timer_uv_list.h"

#define my_malloc malloc
#define my_free free

typedef struct uv_timer {
	uv_timer_t time_handle;
	on_uv_time on_time;
	void* udata;
	int repeat_count; //-1表示无限
}uv_timer;

static uv_timer* alloc_time(on_uv_time fn, void* udata, int repeat) {
	uv_timer* t = (uv_timer*)my_malloc(sizeof(uv_timer));
	if (t == NULL) {
		return NULL;
	}

	t->on_time = fn;
	t->udata = udata;
	t->repeat_count = repeat;
	uv_timer_init(get_uv_loop(),&t->time_handle);
	return t;
}

static void timer_cb(uv_timer_t* handle) {
	uv_timer* timer = (uv_timer*)handle->data;
	if (timer==NULL){
		return;
	}
	if (timer->on_time!=NULL) {
		timer->on_time(timer->udata);
		if (timer->repeat_count > 0) {
			timer->repeat_count--;
			if (timer->repeat_count==0) {
				cancle_timer(timer);
			}
		}
	}
}

uv_timer* create_timer(on_uv_time fn, void* udata, int repeat,int first_time_out, int interval) {
	uv_timer* timer = alloc_time(fn, udata, repeat);
	timer->time_handle.data = timer;
	uv_timer_start(&timer->time_handle, timer_cb, first_time_out, interval);
	return timer;
}

void cancle_timer(uv_timer* timer) {
	if (timer!=NULL) {
		uv_timer_stop(&timer->time_handle);
		my_free(timer);
		timer = NULL;
	}
}

uv_timer* create_timer_once(on_uv_time fn, void* udata,int first_time_out,int interval) {
	uv_timer* timer = create_timer(fn, udata,1, first_time_out,interval);
	return timer;
}

void* get_timer_udata(uv_timer* timer) {
	return timer->udata;
}