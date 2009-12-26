#include "include.h"
#include "aes/aes.h"
#include <zlib.h>

struct timeval tv;
aes_context ctx;
static struct event_base *base = 0L;
struct evbuffer *In, *Out;

static void eventcb(struct bufferevent *bev, short what, void *ptr) {

    if (what & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
        if (what & BEV_EVENT_ERROR) {
            unsigned long err;
        }
        closeConnection((Server *) ptr, TRUE);
        free(ptr);
    }
}

static void onLoadTask(Server *pServer) {
    struct Poll *poll = pServer->poll;
    struct _Verifer_Cfg_Record *Cfg_Record;
    struct _Tester_Cfg_Record FakeCfg_Record = {0};
    struct Request *pReq, *pReqCmpr;
    u_char *data = EVBUFFER_DATA(In);
    u_int len = EVBUFFER_LENGTH(In);
    char *ptr = 0;
    u32 crc = 0;
    int Readed = 0;


    pReq = (struct Request *) data;
/*
    printf("2 EVBUFFER_LENGTH(In) = %d\n", EVBUFFER_LENGTH(In));

    printf("0 %08x. %d vs %d\n", pReq, len, Size_Request);
*/
    if (len < Size_Request) {
        return;
    }
/*
    printf("1 %08x. %d vs %d\n", pReq, len, Size_Request);
*/

    if (pReq->sizes.UncmprSize < Size_Request_hdr or pReq->sizes.UncmprSize > MAX_UNCMPR or(pReq->sizes.CmprSize > 0 and pReq->sizes.CmprSize > MAX_CMPR)) {
        return;
    }

/*
    printf("2 %08x. %d vs %d\n", pReq, len, Size_Request);
*/

    crc = pReq->sizes.crc;

/*
    printf("3 %08x\n", crc);
*/
    pReq->sizes.crc = 0;
/*
    printf("Compress %d, Un %d\n", pReq->sizes.CmprSize, pReq->sizes.UncmprSize);
*/

    if (crc32(0xffffffff, (const Bytef *) pReq, (pReq->sizes.CmprSize > 0 ? pReq->sizes.CmprSize : pReq->sizes.UncmprSize) + Size_Request_sizes) != crc) {
        return;
    }
/*
    printf("CRC OK\n");
*/

    if (pReq->sizes.CmprSize) {
        ptr = malloc(pReq->sizes.UncmprSize + Size_Request_sizes);
        bzero(ptr, pReq->sizes.UncmprSize + Size_Request_sizes);

        memcpy(ptr, data, Size_Request_sizes);
        pReqCmpr = (struct Request *) ptr;


        uncompress((Bytef *) (ptr + Size_Request_sizes), (uLongf *) & pReqCmpr->sizes.UncmprSize, (const Bytef *) & pReq->hdr, (uLongf) pReq->sizes.CmprSize);


        if (pReqCmpr->sizes.UncmprSize != pReq->sizes.UncmprSize) {
            return;
        }
        pReq = pReqCmpr;

    }
    gettimeofday(&tv, NULL);
    int RecordsLen = pReq->sizes.UncmprSize - Size_Request;
    int RecordOffset = 0;
    int newShift = 0;
    for (
            ;
            RecordOffset < RecordsLen;
            RecordOffset += sizeof (* Cfg_Record), Readed += 1
            ) {
        Cfg_Record = (void *) & pReq->Data + RecordOffset;
        if (Cfg_Record->TimeOut == 0 or Cfg_Record->TimeOut >= config.timeout * 950)
            Cfg_Record->TimeOut = config.timeout * 950;


        FakeCfg_Record.LObjId = Cfg_Record->LObjId;
        FakeCfg_Record.ModType = Cfg_Record->ModType;
        FakeCfg_Record.Port = Cfg_Record->Port;
        FakeCfg_Record.IP = Cfg_Record->IP;
        FakeCfg_Record.ConfigLen = Cfg_Record->ConfigLen;
        snprintf(FakeCfg_Record.HostName, sizeof (FakeCfg_Record.HostName), "%s", Cfg_Record->HostName);
        FakeCfg_Record.TimeOut = Cfg_Record->TimeOut;

        FakeCfg_Record.NextCheckDt = RemoteToLocalTime(Cfg_Record->NextCheckDt);
        if (FakeCfg_Record.NextCheckDt < tv.tv_sec) FakeCfg_Record.NextCheckDt = 0;
        else FakeCfg_Record.NextCheckDt = FakeCfg_Record.NextCheckDt - tv.tv_sec;





#ifdef DEBUG
        debug("\tid объекта тестирования = %d", Cfg_Record->LObjId);
        debug("\tтип модуля тестирования (пинг-порт-хттп-...) = %s", getModuleText(Cfg_Record->ModType));

        debug("\tпорт првоерки = %hd", (short) Cfg_Record->Port);
        debug("\tип объекта тестирования = %s", ipString(Cfg_Record->IP));
        debug("\tдата ближайшей проверки. unixtime utc = %d", FakeCfg_Record.NextCheckDt);
        debug("\tимя хоста = %s", Cfg_Record->HostName);
        debug("\tТаймаут для проверки = %d", Cfg_Record->TimeOut);
        debug("\tРазмер дополнительных данных = %d", Cfg_Record->ConfigLen);
#endif

        switch (Cfg_Record->ModType) {
            case MODULE_PING:
                addICMPTask(config.pVeriferDC, &FakeCfg_Record, newShift);
                break;
            case MODULE_TCP_PORT:
                addTCPTask(config.pVeriferDC, &FakeCfg_Record, newShift);
                break;
            case MODULE_DNS:
                addDNSTask(config.pVeriferDC, &FakeCfg_Record, newShift, (char *) (Cfg_Record + 1));
                RecordOffset += Cfg_Record->ConfigLen;
                break;
            default:
                addLuaTask(config.pVeriferDC, &FakeCfg_Record, newShift, (char *) (Cfg_Record + 1));
                RecordOffset += Cfg_Record->ConfigLen;
                break;
        }
    }

    if (pReq->sizes.CmprSize > 0) {
        free(ptr);
    }
    evbuffer_add(Out, &Readed, sizeof (int));
/*
    hexPrint(&Readed, sizeof (int));
*/
}

static void callbackRead(struct bufferevent *bev, void *ptr) {
    Server *pServer = (Server *) ptr;
    struct Poll *poll = pServer->poll;
    struct st_session session;
    size_t len;
    char *Buffer[BUFLEN] = {0};
    u_int BufferLen = 0;
    debug("%s -> %s", ConnectedIpString(pServer), getStatusText(poll->status));


    In = bufferevent_get_input(bev);
    Out = bufferevent_get_output(bev);
    len = evbuffer_get_length(In);

/*
    hexPrint((u_char *) EVBUFFER_DATA(In), EVBUFFER_LENGTH(In));
    printf("EVBUFFER_LENGTH(In) = %d\n", EVBUFFER_LENGTH(In));
*/

    switch (poll->status) {
        case STATE_CONNECTED:
            genSharedKey(pServer, (u_char *) EVBUFFER_DATA(In));
            len = 32;
            poll->status = STATE_SESSION;
            break;
        case STATE_SESSION:
            aes_set_key(&ctx, pServer->key.shared, 128);
            bzero(&session, sizeof (struct st_session));
            BufferLen = aes_cbc_decrypt(&ctx, pServer->key.shared + 16, EVBUFFER_DATA(In), (unsigned char*) & session, sizeof (struct st_session));

            if (memcmp(session.password, pServer->passwordRecv, sizeof (pServer->passwordRecv))) {
                BufferLen = snprintf(Buffer, BUFLEN, "%d", SOCK_RC_AUTH_FAILURE);
                write(bufferevent_getfd(bev), Buffer, BufferLen);
                bufferevent_settimeout(bev, 0, 0);
                return;
            } else {
                aes_set_key(&ctx, pServer->key.shared, 128);
                BufferLen = aes_cbc_encrypt(&ctx, pServer->key.shared + 16, (u_char*) & pServer->session, (u_char *) Buffer, sizeof (struct st_session));
                evbuffer_add(Out, Buffer, BufferLen);
            }
            poll->status = STATE_DONE;
            break;
        default:

/*
            printf("1 EVBUFFER_LENGTH(In) = %d\n", EVBUFFER_LENGTH(In));
*/
            onLoadTask(pServer);
            break;
    }

    evbuffer_drain(In, len);
    if (evbuffer_get_length(In) > 0) {
        callbackRead(bev, ptr);
    }
}

static void callbackWrite(struct bufferevent *bev, void *ptr) {
    Server *pServer = (Server *) ptr;
    debug("%s", ConnectedIpString(pServer));
    struct Poll *poll = pServer->poll;
    struct evbuffer *Out = bufferevent_get_output(bev);
    evbuffer_add(Out, genPublicKey(pServer), 32);
    poll->status = STATE_CONNECTED;
    bufferevent_setcb(pServer->poll->bev, callbackRead, NULL, eventcb, (void *) pServer);
}

void OnAcceptVeriferTask(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *a, int slen, void *p) {
    struct bufferevent *bev;
    struct sockaddr_in *sa;

    sa = (struct sockaddr_in *) a;
    Server *pServer = getNulledMemory(sizeof (*pServer));

    pServer->isVerifer = 1;
    pServer->poll = getNulledMemory(sizeof (struct Poll));
    pServer->poll->client.sin_addr = ((struct sockaddr_in *) a)->sin_addr;
    pServer->poll->client.sin_port = ((struct sockaddr_in *) a)->sin_port;

    debug("%s", ConnectedIpString(pServer));

    memcpy(pServer->session.password, config.pVerifer->session.password, sizeof (pServer->session.password));
    memcpy(pServer->passwordRecv, config.pVerifer->passwordRecv, sizeof (config.pVerifer->passwordRecv));
    pServer->poll->bev = bufferevent_socket_new(evconnlistener_get_base(listener), fd, LEV_OPT_CLOSE_ON_FREE | LEV_OPT_CLOSE_ON_EXEC | BEV_OPT_DEFER_CALLBACKS);


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
