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

    if (task->callback) {
        task->callback(task);
    }


    debug("%s(%s):%d -> id %d, Module %s [ms: %d]", task->Record.HostName, ipString(task->Record.IP), task->Record.Port, task->Record.LObjId, getModuleText(task->Record.ModType), poll->DelayMS);
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

    debug("%s(%s):%d -> id %d, Module %s", task->Record.HostName, ipString(task->Record.IP), task->Record.Port, task->Record.LObjId, getModuleText(task->Record.ModType));

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

    debug("%s(%s):%d -> id %d, Module %s", task->Record.HostName, ipString(task->Record.IP), task->Record.Port, task->Record.LObjId, getModuleText(task->Record.ModType));
    closeTCPConnection(task);
}

void OnReadTCPTask(struct bufferevent *bev, void *arg) {
    struct Task *task = (struct Task *) arg;
    debug("%s(%s):%d -> id %d, Module %s", task->Record.HostName, ipString(task->Record.IP), task->Record.Port, task->Record.LObjId, getModuleText(task->Record.ModType));

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
    debug("%s(%s):%d -> id %d, Module %s", task->Record.HostName, ipString(task->Record.IP), task->Record.Port, task->Record.LObjId, getModuleText(task->Record.ModType));

    task->code = STATE_CONNECTING;
};

void timerTCPTask(int fd, short action, void *arg) {

    struct Task *task = (struct Task *) arg;
    debug("%d id:%d %s", action, task->LObjId, getStatusText(task->code));
    if (!task->Record.IP || task->isEnd == TRUE) return;
    if (task->code) closeTCPConnection(task);


    timerclear(&tv);

    if (task->Record.CheckPeriod and task->pServer->timeOfLastUpdate == task->timeOfLastUpdate) {
        if (task->newTimer) {
            tv.tv_sec = task->newTimer;
            task->newTimer = 0;
        } else {
            tv.tv_sec = task->Record.CheckPeriod;
        }
    } else {
        tv.tv_sec = 60;
        task->isEnd = TRUE;
    }
    task->Record.NextCheckDt += tv.tv_sec;
    evtimer_add(&task->time_ev, &tv);
    openTCPConnection(task);


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


