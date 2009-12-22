#include "include.h"
#include "aes/aes.h"

struct timeval tv;
aes_context ctx;
static struct event_base *base = 0L;

static void eventcb(struct bufferevent *bev, short what, void *ptr) {

    if (what & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
        if (what & BEV_EVENT_ERROR) {
            unsigned long err;
        }
        closeConnection((Server *) ptr, TRUE);
        free(ptr);
    }
}

static void callbackRead(struct bufferevent *bev, void *ptr) {
    Server *pServer = (Server *) ptr;
    struct Poll *poll = pServer->poll;
    struct evbuffer *In, *Out;
    struct st_session session;
    size_t len;
    char *Buffer[BUFLEN] = {0};
    u_int BufferLen = 0;
    printf("verify.c: %s -> %s\n", ConnectedIpString(pServer), getStatusText(poll->status));
    debug("%s -> %s", ConnectedIpString(pServer), getStatusText(poll->status));


    In = bufferevent_get_input(bev);
    Out = bufferevent_get_output(bev);
    len = evbuffer_get_length(In);
    printf("Verify\n---------------------------\n");
    hexPrint(EVBUFFER_DATA(In), len);
    printf("--------------------------------\n\n\n");

    switch (poll->status) {
        case STATE_CONNECTED:
            genSharedKey(pServer, (u_char *) EVBUFFER_DATA(In));
            len = 32;
            poll->status = STATE_SESSION;
            printf("Verify PublicShared\n---------------------------\n");
            hexPrint(pServer->key.public, 16);
            hexPrint(pServer->key.shared, 16);
            printf("--------------------------------\n\n\n");
            break;
        case STATE_SESSION:
            aes_set_key(&ctx, pServer->key.shared, 128);
            bzero(&session, sizeof (struct st_session));
            BufferLen = aes_cbc_decrypt(&ctx, pServer->key.shared + 16, EVBUFFER_DATA(In), (unsigned char*) & session, sizeof (struct st_session));

            if (memcmp(session.password, pServer->session.password, sizeof (pServer->session.password))) {
                BufferLen = snprintf(Buffer, BUFLEN, "%d", SOCK_RC_AUTH_FAILURE);
                write(bufferevent_getfd(bev), Buffer, BufferLen);
                //                evbuffer_add(Out, Buffer, BufferLen);
                printf("session.password=%s, config.session.password=%s\n", session.password, pServer->session.password);
                printf(cRED"\tPassword:\tFAIL\n"cEND);
                bufferevent_settimeout(bev,0,0);
                return;
            } else {
                printf(cGREEN"\tPassword:"cEND"\t"cBLUE"OK\n"cEND);
                aes_set_key(&ctx, pServer->key.shared, 128);
                BufferLen = aes_cbc_encrypt(&ctx, pServer->key.shared + 16, (u_char*) & pServer->session, (u_char *) Buffer, sizeof (struct st_session));
                evbuffer_add(Out, Buffer, BufferLen);
            }
            poll->status = STATE_DONE;
            break;
        default:
            LoadTask(pServer);
            break;
    }



    evbuffer_drain(In, len);

    if (evbuffer_get_length(In) > 0) {
        callbackRead(bev, ptr);
    }

    //    buffer = bufferevent_get_output(bev);
    //    evbuffer_add_printf(buffer, "fuckyou");
}

static void callbackWrite(struct bufferevent *bev, void *ptr) {
    Server *pServer = (Server *) ptr;
    struct Poll *poll = pServer->poll;
    struct evbuffer *Out;
    printf("verify.c Write: %s -> %s\n", ConnectedIpString(pServer), getStatusText(poll->status));
    Out = bufferevent_get_output(bev);
    evbuffer_add(Out, genPublicKey(pServer), 32);
    poll->status = STATE_CONNECTED;
    bufferevent_setcb(pServer->poll->bev, callbackRead, NULL, eventcb, (void *) pServer);
}

void OnAcceptVeriferTask(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *a, int slen, void *p) {
    struct bufferevent *bev;
    struct sockaddr_in *sa;
    sa = (struct sockaddr_in *) a;
    Server *pServer = getNulledMemory(sizeof (*pServer));
    pServer->poll = getNulledMemory(sizeof (struct Poll));
    pServer->poll->client.sin_addr = ((struct sockaddr_in *) a)->sin_addr;
    pServer->poll->client.sin_port = ((struct sockaddr_in *) a)->sin_port;
    memcpy(pServer->session.password, config.pVerifer->session.password, sizeof (pServer->session.password));
    pServer->poll->bev = bufferevent_socket_new(evconnlistener_get_base(listener), fd, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);


    bufferevent_setcb(pServer->poll->bev, NULL, callbackWrite, eventcb, (void *) pServer);
    bufferevent_enable(pServer->poll->bev, EV_READ | EV_WRITE | EV_TIMEOUT);
    if (pServer->timeout) {
        timerclear(&tv);
        tv.tv_sec = pServer->timeout;
        bufferevent_set_timeouts(pServer->poll->bev, &tv, &tv);
    }


}

void runVeriferTask() {

    struct sockaddr_in sa;

    u32 ip;
    if (config.pVerifer->host) {
        inet_aton(config.pVerifer->host, &ip);
        sa.sin_addr = *((struct in_addr *) & ip);
    } else {
        sa.sin_addr.s_addr = 0;
    }
    sa.sin_family = AF_INET;
    sa.sin_port = htons(config.pVerifer->port);
    bzero(&sa.sin_zero, 8);
    debug("%s:%d", config.pVerifer->host, config.pVerifer->port);
    config.pVerifer->poll->type = MODE_VERIFER;
    evconnlistener_new_bind(base, OnAcceptVeriferTask, NULL, LEV_OPT_CLOSE_ON_FREE | LEV_OPT_CLOSE_ON_EXEC | LEV_OPT_REUSEABLE, -1, (struct sockaddr*) & sa, sizeof (sa));


}

static void initThread() {
    base = event_base_new();
    runVeriferTask();
    event_base_dispatch(base);
}

void initVerifer() {
    pthread_t threads;
    pthread_create(&threads, NULL, initThread, NULL);
}

struct event_base *getVeriferBase() {
    return base;
}
