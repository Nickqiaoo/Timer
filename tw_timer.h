#ifndef TIME_WHEEL_TIMER
#define TIME_WHEEL_TIMER

#include <netinet/in.h>
#include <stdio.h>
#include <time.h>

#define BUFFER_SIZE 64
class tw_timer; //前向声明

//绑定socket和定时器
struct client_data {
    sockaddr_in address;
    int sockfd;
    char buf[BUFFER_SIZE];
    tw_timer* timer;
};

//定时器类
class tw_timer {
   public:
    tw_timer(int rot, int ts)
        : next(NULL), prev(NULL), rotation(rot), time_slot(ts) {}

   public:
    int rotation; //记录定时器在时间轮转多少圈后生效
    int time_slot; //记录定时器属于时间轮上哪个槽(对应的链表)
    void (*cb_func)(client_data*); //定时器回调函数
    client_data* user_data; //客户数据
    tw_timer* next; //下一个定时器
    tw_timer* prev; //前一个
};


class time_wheel {
   public:
    time_wheel() : cur_slot(0) {
        for (int i = 0; i < N; ++i) {
            slots[i] = NULL; //初始化每个槽的头结点
        }
    }
    //销毁每个槽的定时器
    ~time_wheel() {
        for (int i = 0; i < N; ++i) {
            tw_timer* tmp = slots[i];
            while (tmp) {
                slots[i] = tmp->next;
                delete tmp;
                tmp = slots[i];
            }
        }
    }
    
    //根据定时值创建一个定时器,并把它插入合适的槽中
    tw_timer* add_timer(int timeout) {
        if (timeout < 0) {
            return NULL;
        }
        //计算在时间轮转动多少个滴答后触发
        int ticks = 0;
        if (timeout < TI) {
            ticks = 1;
        } else {
            ticks = timeout / TI;
        }
        //计算转动多少圈后触发
        int rotation = ticks / N;
        //计算应该插入哪个槽中
        int ts = (cur_slot + (ticks % N)) % N;
        //创建定时器
        tw_timer* timer = new tw_timer(rotation, ts);
        //如果槽中尚无定时器则设为头结点
        if (!slots[ts]) {
            printf("add timer, rotation is %d, ts is %d, cur_slot is %d\n",
                   rotation, ts, cur_slot);
            slots[ts] = timer;
        } else {
            //否则将定时器插入槽中
            timer->next = slots[ts];
            slots[ts]->prev = timer;
            slots[ts] = timer;
        }
        return timer;
    }
    //删除定时器
    void del_timer(tw_timer* timer) {
        if (!timer) {
            return;
        }
        int ts = timer->time_slot;
        //如果要删除的是头结点,则重置头结点
        if (timer == slots[ts]) {
            slots[ts] = slots[ts]->next;
            if (slots[ts]) {
                slots[ts]->prev = NULL;
            }
            delete timer;
        } else {
            timer->prev->next = timer->next;
            if (timer->next) {
                timer->next->prev = timer->prev;
            }
            delete timer;
        }
    }
    //SI时间到后,调用该函数,时间轮向前滚动一个槽
    void tick() {
        tw_timer* tmp = slots[cur_slot];
        printf("current slot is %d\n", cur_slot);
        while (tmp) {
            printf("tick the timer once\n");
            //如果rotation大于0,则在这一轮不起作用
            if (tmp->rotation > 0) {
                tmp->rotation--;
                tmp = tmp->next;
            } else {
                //否则,执行定时任务,删除定时器
                tmp->cb_func(tmp->user_data);
                if (tmp == slots[cur_slot]) {
                    printf("delete header in cur_slot\n");
                    slots[cur_slot] = tmp->next;
                    delete tmp;
                    if (slots[cur_slot]) {
                        slots[cur_slot]->prev = NULL;
                    }
                    tmp = slots[cur_slot];
                } else {
                    tmp->prev->next = tmp->next;
                    if (tmp->next) {
                        tmp->next->prev = tmp->prev;
                    }
                    tw_timer* tmp2 = tmp->next;
                    delete tmp;
                    tmp = tmp2;
                }
            }
        }
        cur_slot = ++cur_slot % N;//更新当前槽
    }

   private:
    //时间轮上槽的数目
    static const int N = 60;
    //槽间隔1s
    static const int TI = 1;
    //时间轮的槽,每一个元素指向一个定时器链表
    tw_timer* slots[N];
    //时间轮当前槽
    int cur_slot;
};

#endif