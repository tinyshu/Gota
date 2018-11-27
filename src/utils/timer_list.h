#ifndef __TIMER_LIST_H__
#define __TIMER_LIST_H__

// 创建timer list,
struct timer_list*
create_timer_list();

// 扫描执行我们的timer list里面 到时间的函数
// 返回的是: timer list里面下一个最紧急的timer到达的时间;

int
update_timer_list(struct timer_list* list);

// end 
// 销毁掉timer list
void
destroy_timer_list(struct timer_list* list);

#endif

