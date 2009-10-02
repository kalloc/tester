#include <event2/event.h>

#include "include.h"

static struct event_base *base = 0L;
struct timeval tv;

void closeTCPConnection(struct Task *task) {
    struct stTCPUDPInfo *poll = (struct stTCPUDPInfo *) task->poll;
    gettimeofday(&tv, NULL);


    if ((char *) poll->bev != 0) {
        bufferevent_free(poll->bev);
        poll->bev = 0;
    }

    poll->DelayMS = timeDiffMS(tv, poll->CheckDt);

#ifndef TESTER
    if (task->callback) {
        task->callback(task);
    }
#else
    incStat(task->Record.ModType, task->code);
#endif


#ifdef DEBUG
    printf(cGREEN"TASK"cEND" CLOSE -> id %d, Module %s for %s [%s:%d] [ms: %d]\n", task->Record.LObjId, getModuleText(task->Record.ModType), task->Record.HostName, ipString(task->Record.IP), task->Record.Port, poll->DelayMS);
#endif

    task->code = STATE_DISCONNECTED;
    shutdown(poll->fd, 1);
    close(poll->fd);

    //таск больше не нужен
    if (task->isEnd == TRUE) {
        evtimer_del(&task->time_ev);
        deleteTask(task);
    }

};

void OnErrorTCPTask(struct bufferevent *bev, short what, void *arg) {
    struct Task *task = (struct Task *) arg;
    struct stTCPUDPInfo *poll = (struct stTCPUDPInfo *) task->poll;
    int valopt = 0;
    socklen_t lon = 0;

#ifdef DEBUG
    printf(cGREEN"TASK"cEND" ERROR -> id %d, Module %s for %s [%s:%d]\n", task->Record.LObjId, getModuleText(task->Record.ModType), task->Record.HostName, ipString(task->Record.IP), task->Record.Port);
#endif

    getsockopt(poll->fd, SOL_SOCKET, SO_ERROR, (void *) & valopt, &lon);
    poll->CheckOk = 0;
    task->code = STATE_TIMEOUT;
    //добавляем репорт о ошибке
    closeTCPConnection(task);

}

void OnWriteTCPTask(struct bufferevent *bev, void *arg) {
    struct Task *task = (struct Task *) arg;
    struct stTCPUDPInfo *poll = (struct stTCPUDPInfo *) task->poll;

    task->code = STATE_DONE;
    poll->CheckOk = 1;

#ifdef DEBUG
    printf(cGREEN"TASK"cEND" WRITE -> id %d, Module %s for %s [%s:%d]\n", task->Record.LObjId, getModuleText(task->Record.ModType), task->Record.HostName, ipString(task->Record.IP), task->Record.Port);
#endif

    closeTCPConnection(task);
}

void OnReadTCPTask(struct bufferevent *bev, void *arg) {
    struct Task *task = (struct Task *) arg;
#ifdef DEBUG
    printf(cGREEN"TASK"cEND" READ -> id %d, Module %s for %s [%s:%d] \n", task->Record.LObjId, getModuleText(task->Record.ModType), task->Record.HostName, ipString(task->Record.IP), task->Record.Port);
#endif

}

void openTCPConnection(struct Task *task) {
    struct stTCPUDPInfo *poll = (struct stTCPUDPInfo *) task->poll;
    struct sockaddr_in sa;


    gettimeofday(&((struct stTCPUDPInfo *) task->poll)->CheckDt, NULL);


    sa.sin_addr = *((struct in_addr *) & task->Record.IP);
    sa.sin_family = AF_INET;
    sa.sin_port = htons(task->Record.Port);
    bzero(&sa.sin_zero, 8);


    poll->bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_socket_connect(poll->bev, (struct sockaddr *) & sa, sizeof (sa));
    bufferevent_setcb(poll->bev, OnReadTCPTask, OnWriteTCPTask, OnErrorTCPTask, task);
    bufferevent_enable(poll->bev, EV_WRITE | EV_READ | EV_PERSIST | EV_TIMEOUT);


    timerclear(&tv);
    if (task->Record.TimeOut > 1000) {
        tv.tv_sec = task->Record.TimeOut / 1000;
        tv.tv_usec = (task->Record.TimeOut % 1000)*1000;
    } else {
        tv.tv_usec = task->Record.TimeOut * 1000;
    }

    bufferevent_set_timeouts(poll->bev, &tv, &tv);

#ifdef DEBUG
    printf(cGREEN"TASK"cEND" CONNECT -> id %d, Module %s for %s [%s:%d]\n", task->Record.LObjId, getModuleText(task->Record.ModType), task->Record.HostName, ipString(task->Record.IP), task->Record.Port);
#endif

    task->code = STATE_CONNECTING;
};

void timerTCPTask(int fd, short action, void *arg) {
    struct Task *task = (struct Task *) arg;

    //если состояние подключения не изменилось, то значит что какого либо ответа не было получено
#ifdef DEBUG
    printf(cGREEN"TASK"cEND" SCHEDULE -> id %d, Module %s for %s [%s:%d]\n", task->Record.LObjId, getModuleText(task->Record.ModType), task->Record.HostName, ipString(task->Record.IP), task->Record.Port);
#endif

    if (task->isEnd == TRUE) {
        closeTCPConnection(task);
        return;
    }
    openTCPConnection(task);


    timerclear(&tv);

    if (task->Record.CheckPeriod and task->pServer->timeOfLastUpdate == task->timeOfLastUpdate) {
        tv.tv_sec = task->Record.CheckPeriod;
    } else {
        task->isEnd = TRUE;
        tv.tv_sec = 60;
    }
    evtimer_add(&task->time_ev, &tv);
}

static void timerMain(int fd, short action, void *arg) {
    struct event *ev = (struct event *) arg;
    tv.tv_sec = 60;
    tv.tv_usec = 0;
    evtimer_add(ev, &tv);

}

static void initTCPThread() {
    base = event_base_new();

    struct event ev;

    tv.tv_sec = 60;
    tv.tv_usec = 0;
    event_assign(&ev, base, -1, 0, timerMain, &ev);
    evtimer_add(&ev, &tv);
    event_base_dispatch(base);
}

void initTCPTester() {
    pthread_t threads;
    pthread_create(&threads, NULL, initTCPThread, NULL);
}


