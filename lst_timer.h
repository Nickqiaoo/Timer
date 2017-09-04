#ifndef LST_TIMER
#define LST_TIMER

#include <time.h>
#include <stdio.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 64
class util_timer; //前向声明

//用户数据结构,客户端socket地址,socket文件描述符,读缓存和定时器
struct client_data {
    sockaddr_in address;
    int sockfd;
    char buf[BUFFER_SIZE];
    util_timer* timer;
};

//定时器类
class util_timer {
   public:
    util_timer() : prev(NULL), next(NULL) {}

   public:
    time_t expire; //任务的超时时间,这里使用绝对时间
    void (*cb_func)(client_data*); //任务回调函数
    client_data* user_data;  //回调函数处理的客户数据,由定时器执行者传递给回调函数
    util_timer* prev; //上一个定时器
    util_timer* next; //下一个
};

//定时器链表,它是一个升序,双向链表,且带有头尾节点
class sort_timer_lst {
   public:      
    sort_timer_lst() : head(NULL), tail(NULL) {}
    //链表销毁时,删除所有定时器
    ~sort_timer_lst() {
        util_timer* tmp = head;
        while (tmp) {
            head = tmp->next;
            delete tmp;
            tmp = head;
        }
    }
    //添加定时器
    void add_timer(util_timer* timer) {
        if (!timer) {
            return;
        }
        if (!head) {
            head = tail = timer;
            return;
        }
        /*如果该定时器超时时间小于头结点,则插入头部,否则调用重载函数插入合适的位置
        */
        if (timer->expire < head->expire) {
            timer->next = head;
            head->prev = timer;
            head = timer;
            return;
        }
        add_timer(timer, head);
    }
    //当定时器超时时间延长时,向后调整定时器
    void adjust_timer(util_timer* timer) {
        if (!timer) {
            return;
        }
        util_timer* tmp = timer->next;
        //如果是尾节点或者超时时间仍小于下一个定时器,则不用调整
        if (!tmp || (timer->expire < tmp->expire)) {
            return;
        }
        //如果是头结点,则取出重新插入
        if (timer == head) {
            head = head->next;
            head->prev = NULL;
            timer->next = NULL;
            add_timer(timer, head);
        } else {
            //不是头结点则取出插入之后的链表中
            timer->prev->next = timer->next;
            timer->next->prev = timer->prev;
            add_timer(timer, timer->next);
        }
    }
    //删除定时器
    void del_timer(util_timer* timer) {
        if (!timer) {
            return;
        }
        //当只有目标定时器时
        if ((timer == head) && (timer == tail)) {
            delete timer;
            head = NULL;
            tail = NULL;
            return;
        }
        //如果至少由两个定时器,且目标定时器是头结点
        if (timer == head) {
            head = head->next;
            head->prev = NULL;
            delete timer;
            return;
        }
        //如果至少由两个定时器,且目标定时器是尾结点
        if (timer == tail) {
            tail = tail->prev;
            tail->next = NULL;
            delete timer;
            return;
        }
        //位于中间
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        delete timer;
    }
    /*SIGALRM信号每次被触发就在其信号处理函数中执行,以处理链表上到期的任务
    */
    void tick() {
        if (!head) {
            return;
        }
        printf("timer tick\n");
        time_t cur = time(NULL);
        util_timer* tmp = head;
        //依次处理每个到期的定时器
        while (tmp) {
            if (cur < tmp->expire) {
                break;
            }
            //调用回调函数,执行定时任务
            tmp->cb_func(tmp->user_data);
            //删除该定时器
            head = tmp->next;
            if (head) {
                head->prev = NULL;
            }
            delete tmp;
            tmp = head;
        }
    }

   private:
    void add_timer(util_timer* timer, util_timer* lst_head) {
        util_timer* prev = lst_head;
        util_timer* tmp = prev->next;
        while (tmp) {
            //找到超时时间大于目标定时器的节点
            if (timer->expire < tmp->expire) {
                prev->next = timer;
                timer->next = tmp;
                tmp->prev = timer;
                timer->prev = prev;
                break;
            }
            prev = tmp;
            tmp = tmp->next;
        }
        //没有找到则设为尾节点
        if (!tmp) {
            prev->next = timer;
            timer->prev = prev;
            timer->next = NULL;
            tail = timer;
        }
    }

   private:
    util_timer* head;
    util_timer* tail;
};

#endif