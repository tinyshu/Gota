#ifndef __TIMER_H__
#define __TIMER_H__

// 功能: 添加一个可以被触发一次的timer,
// 参数1:每一个线程/进程都应该有自己的timer list,这样才能保证,自己的线程
// 添加timer在响音的时候是在同一个线程里面;
// 参数2: 当timer触发以后的响应函数;
// 参数3: 当timer触发以后，需要传给这个触发函数的用户的数据；
// 参数4: 我们多少秒以后触发我们的timer响应;
// 返回值: 每一个timer都会回来一个不重复的id号,这个id号就是timer找到的
// 唯一的标识号;
unsigned int
add_timer(struct timer_list* list, 
          void (*on_timer)(void* udata), 
		  void* udata, float after_sec);

// 功能: 取消掉一个timer的触发
// 参数1: timer list;
// 参数2: add_timer返回的timeid, 我们可以通过这个timeid在timerlist里面找到这个
// timer;
void
cancel_timer(struct timer_list* list, unsigned int timeid);

// 功能每隔一段时间都触发这个timer，触发多次，知道你cancel掉这个timer
unsigned int
schedule_timer(struct timer_list* list,
               void(*on_timer)(void* udata),
			   void* udata, float after_sec);


#endif

