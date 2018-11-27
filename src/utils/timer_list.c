#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "timer.h"
#include "timer_list.h"

#define my_malloc malloc
#define my_free free

// 平台相关
#ifdef WIN32
#include <windows.h>
static unsigned int
get_cur_ms() {
	return GetTickCount();
}
#else
static unsigned int
get_cur_ms() {
	struct timeval tv;
	struct timezone tz;
	gettimeofday(&tv, &tz);

	return ((tv.tv_usec / 1000) + tv.tv_sec * 1000);
}
#endif

// end 
struct timer {
	void(*on_timer)(void* udata); // timer回掉函数的指针;
	void* udata; // 用户要在timer触发后传给回掉函数的数据指针;

	unsigned int duration; // 触发的时间间隔
	unsigned int end_time_stamp; // 结束的时间撮,毫秒;
	int repeat; // 重复的次数，-1,表示一直调用;
	unsigned int timeid; // 这个timer对应的timerid;

	struct timer* hash_next; // 指向同一个hash值得下一个节点;
};

// hash表来管理我们的timer, hash value timerid;
#define HASH_COUNT 1024

struct timer_list {
	unsigned int global_timeid; // 当前可以使用的timer的id号，一直往上增加;
	// 以这个数来作为 timer的ID号,从0开始
	struct timer* running_timer;
	struct timer* hash_timers[HASH_COUNT]; // HASH 数组,数组指向了timer的拥有同一个hash值得链表
};

struct timer_list*
create_timer_list()  {
	struct timer_list* list = my_malloc(sizeof(struct timer_list));
	memset(list, 0, sizeof(struct timer_list));

	return list;
}

unsigned int
add_timer(struct timer_list* list,
          void(*on_timer)(void* udata),
          void* udata, float after_sec) {
	int timeid = list->global_timeid ++; // 唯一的往上加
	struct timer* t = my_malloc(sizeof(struct timer));
	memset(t, 0, sizeof(struct timer));
	t->repeat = 1;
	t->duration = (unsigned int)(after_sec * 1000);
	t->end_time_stamp = get_cur_ms() + t->duration; // timer触发的时间撮,现在的时间 + 多少毫秒以后的时间
	t->timeid = timeid;
	t->udata = udata;
	t->on_timer = on_timer;

	// 做hash索引
	int index = timeid % HASH_COUNT;
	struct timer** walk = &list->hash_timers[index];
	while (*walk) {
		walk = &(*walk)->hash_next;
	}
	*walk = t;
	// end 

	return timeid;
}

unsigned int
schedule_timer(struct timer_list* list,
               void(*on_timer)(void* udata),
			   void* udata, float after_sec) {
	int timeid = list->global_timeid++; // 唯一的往上加
	struct timer* t = my_malloc(sizeof(struct timer));
	memset(t, 0, sizeof(struct timer));
	t->repeat = -1; // 永远执行下去;
	t->duration = (unsigned int)(after_sec * 1000);
	t->end_time_stamp = get_cur_ms() + t->duration; // timer触发的时间撮,现在的时间 + 多少毫秒以后的时间
	t->timeid = timeid;
	t->udata = udata;
	t->on_timer = on_timer;

	// 做hash索引
	int index = timeid % HASH_COUNT;
	struct timer** walk = &list->hash_timers[index];
	while (*walk) {
		walk = &(*walk)->hash_next;
	}
	*walk = t;
	// end 

	return timeid;
}

void
cancel_timer(struct timer_list* list, unsigned int timeid) {
	// 挡住自己删除自己的 timeid在回掉的时候
	if (list->running_timer != NULL && list->running_timer->timeid == timeid) {
		list->running_timer->repeat = 1;
		return;
	}
	// end

	int hash_index = timeid % HASH_COUNT;
	struct timer** walk = &list->hash_timers[hash_index];

	while (*walk) {
		if ((*walk)->timeid == timeid) { // 找到timer,将它移除到外面；
			struct timer* t = (*walk);
			*walk = (*walk)->hash_next;

			my_free(t); // 清除了一个timer;
			return;
		}
		walk = &(*walk)->hash_next;
	}
}

void
destroy_timer_list(struct timer_list* list) {
	for (int i = 0; i < HASH_COUNT; i++) {
		struct timer** walk = &list->hash_timers[i];
		while (*walk) {
			struct timer* t = (*walk);
			*walk = (*walk)->hash_next;
			my_free(t);
		}
	}

	my_free(list);
}

int
update_timer_list(struct timer_list* list) {
	unsigned int min_msec = UINT_MAX;
	struct timer* min_timer = NULL;

	unsigned int start_msec = get_cur_ms();
	list->running_timer = NULL;
	// 扫描所有的timer,执行到时间的timer;
	for (int i = 0; i < HASH_COUNT; i++) {
		struct timer** walk = &list->hash_timers[i];
		while (*walk) {
			unsigned int cur_time = get_cur_ms(); // 获取当前的时间
			if ((*walk)->end_time_stamp <= cur_time) { // 需要触发了
				list->running_timer = (*walk);
				if ((*walk)->on_timer) {
					(*walk)->on_timer((*walk)->udata);
				}
				list->running_timer = NULL;

				if ((*walk)->repeat > 0) {
					(*walk)->repeat --;
					if ((*walk)->repeat == 0) { // 将timer移除出去;
						struct timer* t = (*walk);
						(*walk) = (*walk)->hash_next;
						my_free(t);
					}
					else {
						(*walk)->end_time_stamp = get_cur_ms() + (*walk)->duration;
						if (((*walk)->end_time_stamp - start_msec) < min_msec) {
							min_timer = (*walk);
							min_msec = (*walk)->end_time_stamp - start_msec;
						}
						walk = &((*walk)->hash_next);
					}
				}
				else {
					(*walk)->end_time_stamp = get_cur_ms() + (*walk)->duration;
					if (((*walk)->end_time_stamp - start_msec) < min_msec) {
						min_timer = (*walk);
						min_msec = (*walk)->end_time_stamp - start_msec;
					}
					walk = &((*walk)->hash_next);
				}
			}
			else { // 不需要触发
				if (((*walk)->end_time_stamp - start_msec) < min_msec) {
					min_timer = (*walk);
					min_msec = (*walk)->end_time_stamp - start_msec;
				}
				walk = &((*walk)->hash_next);
			}
		}
	}

	if (min_timer != NULL) { // 存在下一个紧迫的timer,
		return (min_timer->end_time_stamp - get_cur_ms());
	}

	return -1; // timer list已经没有任何timer了。
}