#include "include.h"
#include "aes/aes.h"
static pthread_mutex_t count_lock;
struct timeval tv;
aes_context ctx;
char IV[16];
struct event_base *mainBase = 0L;
char *Buffer = NULL;
u_int BufferLen = 0;

void OnBufferedError(struct bufferevent *bev, short what, void *arg) {
    if (what & BEV_EVENT_CONNECTED) return;
    closeConnection((Server *) arg, FALSE);
}

void OnBufferedWrite(struct bufferevent *bev, void *arg) {
    openSession((Server *) arg, EV_WRITE);
}

void OnBufferedRead(struct bufferevent *bev, void *arg) {
    if (((Server *) arg)->poll->status == STATE_CONNECTED) {
        onEventFromServer((Server *) arg, EV_READ);
    } else {
        openSession((Server *) arg, EV_READ);
    }
}

void openSession(Server *pServer, short action) {
    struct Poll *poll = pServer->poll;

    struct evbuffer *buffer = bufferevent_get_input(poll->bev);
    u_char *data = EVBUFFER_DATA(buffer);
    u_int len = evbuffer_get_length(buffer);
    debug("%s:%d -> %s %s", pServer->host, pServer->port, getActionText(action), getStatusText(poll->status));
    if (poll->status == STATE_QUIT) {
        closeConnection(pServer, FALSE);
        return;
    }

    if (action & EV_READ) {
        genSharedKey(pServer, (u_char *) data);
        poll->status = STATE_SESSION;
        bufferevent_enable(poll->bev, EV_WRITE);

    }

    if (action & EV_WRITE) {
        switch (poll->status) {
            case STATE_CONNECTING:
                //   poll->status == STATE_CONNECTED;
                write(bufferevent_getfd(poll->bev), genPublicKey(pServer), 32);
                bufferevent_enable(poll->bev, EV_READ);
                break;
            case STATE_SESSION:
                aes_set_key(&ctx, pServer->key.shared, 128);
                BufferLen = aes_cbc_encrypt(&ctx, pServer->key.shared + 16, (u_char*) & pServer->session, (u_char *) Buffer, sizeof (struct st_session));
                write(bufferevent_getfd(poll->bev), Buffer, BufferLen);
                poll->status = STATE_CONNECTED;
                bufferevent_enable(poll->bev, EV_READ | EV_PERSIST);
                if (pServer->flagRetriveConfig) LoadTask(pServer);
                if (pServer->flagSendReportError) SendReportError(pServer);
                if (pServer->flagSendReport) SendReport(pServer);
                break;
        }
    }

    if (len > 0) {
        evbuffer_drain(buffer, EVBUFFER_LENGTH(buffer));
    }
}

void InitConnectTo(Server *pServer) {

    struct sockaddr_in sa;

    u32 ip;
    inet_aton(pServer->host, &ip);
    sa.sin_addr = *((struct in_addr *) & ip);
    sa.sin_family = AF_INET;
    sa.sin_port = htons(pServer->port);
    bzero(&sa.sin_zero, 8);

    debug("%s:%d", pServer->host, pServer->port);
    pServer->poll->status = STATE_CONNECTING;
    pServer->poll->type = MODE_SERVER;


    pServer->poll->bev = bufferevent_socket_new(mainBase, -1, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_socket_connect(pServer->poll->bev, (struct sockaddr *) & sa, sizeof (sa));
    bufferevent_setcb(pServer->poll->bev, OnBufferedRead, OnBufferedWrite, OnBufferedError, pServer);
    bufferevent_enable(pServer->poll->bev, EV_WRITE | EV_TIMEOUT);


    if (pServer->timeout) {
        timerclear(&tv);
        tv.tv_sec = pServer->timeout;
        bufferevent_set_timeouts(pServer->poll->bev, &tv, &tv);
    }

}

void timerRetrieveTask(int fd, short action, void *arg) {
    Server *pServer = (Server *) arg;
    static int countRetrieve = 0;
    timerclear(&tv);
    tv.tv_sec = pServer->periodRetrieve;
    evtimer_add(&pServer->evConfig, &tv);

    debug("%s:%d,%d -> %s  %s", pServer->host, pServer->port, (int) pServer->periodRetrieve, getActionText(action), getStatusText(pServer->poll->status));
    pServer->flagRetriveConfig = 1;
    if (pServer->poll->status == STATE_DISCONNECTED) {
        InitConnectTo(pServer);
    } else if (pServer->poll->status == STATE_CONNECTED) {
        LoadTask(pServer);
    }
    countRetrieve++;
    if (countRetrieve > 100) {
        countRetrieve = 0;
        restart_handler(0);
    }

}

void timerSendReportError(int fd, short action, void *arg) {
    Server *pServer = (Server *) arg;
    struct timeval tv;
    timerclear(&tv);
    tv.tv_sec = pServer->periodReportError;
    evtimer_add(&pServer->evReportError, &tv);

    if (countReportError(pServer) > 0) {
        debug("%s:%d -> Errors %d, %s %s  [period - %d]", pServer->host, pServer->port,
                countReportError(pServer),
                getActionText(action), getStatusText(pServer->poll->status),
                (int) pServer->periodReportError);

        pServer->flagSendReportError = 1;

        if (pServer->poll->status == STATE_DISCONNECTED) {
            InitConnectTo(pServer);
        } else if (pServer->poll->status == STATE_CONNECTED) {
            SendReportError(pServer);
        }
    }


}

void timerSendReport(int fd, short action, void *arg) {
    Server *pServer = (Server *) arg;
    struct timeval tv;

    timerclear(&tv);
    tv.tv_sec = pServer->periodReport;
    evtimer_add(&pServer->evReport, &tv);

    if (countReport(pServer) > 0) {
        debug("%s:%d -> Report %d, %s %s  [period - %d]", pServer->host, pServer->port,
                countReport(pServer),
                getActionText(action), getStatusText(pServer->poll->status),
                (int) pServer->periodReport);

        pServer->flagSendReport = 1;

        if (pServer->poll->status == STATE_DISCONNECTED) {
            InitConnectTo(pServer);
        } else if (pServer->poll->status == STATE_CONNECTED) {
            SendReport(pServer);
        }
    }


}

void initPtr() {
    Buffer = getNulledMemory(BUFLEN);
    initTester();
    initProcess();
    initResolv();
}

void freePtr() {
    free(Buffer);
    freeProcess();
}
#ifndef TESTER

int main(int argc, char **argv) {

    initMainVars();
    Server *pServer;
    int ServerId = 0;
    timerclear(&tv);
    // возможно лучше проверять не сдох ли fd
    struct sigaction IgnoreYourSignal;
    sigemptyset(&IgnoreYourSignal.sa_mask);
    IgnoreYourSignal.sa_handler = SIG_IGN;
    IgnoreYourSignal.sa_flags = 0;
    sigaction(SIGPIPE, &IgnoreYourSignal, NULL);


    if (argc < 2) {
        printf(cRED"ERROR: need config name\n"cEND);
        printf(cGREEN"\tSample:"cEND""cBLUE" %s /path/to/config.xml\n\n"cEND, argv[0]);
        exit(FALSE);
    }




    if (!openConfiguration(argv[1])) {
        exit(FALSE);
    }

    pthread_mutex_init(&count_lock, NULL);
    evthread_use_pthreads();
    evthread_make_base_notifiable(mainBase);

    initPtr();
    for (ServerId = 0;; ServerId++) {
        pServer = getNulledMemory(sizeof (*pServer));
        pServer->poll = getNulledMemory(sizeof (struct Poll));
        loadServerFromConfiguration(pServer, ServerId);

        if (!pServer->port or !pServer->host) {
            free(pServer->poll);
            free(pServer);
            break;
        } else {

            if (pServer->isDC) {
                evtimer_set(&pServer->evConfig, timerRetrieveTask, pServer);
                evtimer_add(&pServer->evConfig, &tv);
            }

            tv.tv_sec = pServer->periodReportError;
            evtimer_set(&pServer->evReportError, timerSendReportError, pServer);
            evtimer_add(&pServer->evReportError, &tv);

            tv.tv_sec = pServer->periodReport;
            evtimer_set(&pServer->evReport, timerSendReport, pServer);
            evtimer_add(&pServer->evReport, &tv);
        }
        initReport(pServer);
    }
    if (ServerId == 0) {
        printf(cRED"ERROR: incorect config file\n"cEND);
        printf(cGREEN"\tDescription:"cEND""cBLUE" nil server loaded\n\n"cEND);
        exit(FALSE);
    }


    event_dispatch();
    event_base_free(mainBase);
    freePtr();
    return 0;
}
#endif



