#ifndef TIMER_UV_LIST_H__
#define TIMER_UV_LIST_H__

extern uv_loop_t* get_uv_loop();
struct uv_timer;

typedef void(*on_uv_time)(void* udata);

uv_timer* create_timer(on_uv_time fn,void* udata,int repeat, int first_time_out,int interval);

void cancle_timer(uv_timer* timer);

uv_timer* create_timer_once(on_uv_time fn, void* udata, int first_time_out,int interval);

void* get_timer_udata(uv_timer* timer);
#endif


