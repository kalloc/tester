#include "include.h"
#include <zlib.h>
const char version[] = "Tester v0.01/lnx"PLATFORM", Compiled:"__DATE__" "__TIME__", Source:"__FILE__","__TIMESTAMP__"";
static struct evbuffer *evBuffer = NULL;

static struct timeval tv;

void initProcess() {
}

void freeProcess() {
}

void onErrorFromServer(Server *pServer, int code) {
    switch (code) {
        case SOCK_RC_AUTH_FAILURE:
            closeConnection(pServer, FALSE);
            break;
        case SOCK_RC_TIMEOUT:
            closeConnection(pServer, FALSE);
            break;
    }
#ifdef DEBUG
    printf(cRED"PROCESS FAIL"cEND" -> %s[%d] host [%s:%d]\n", getErrorText(code), code, pServer->host, pServer->port);
#endif
}

#define Size_Request_hdr sizeof(pReq->hdr)
#define Size_Request_sizes sizeof(pReq->sizes)
#define Size_Request (sizeof(struct Request)-sizeof(char *))

#define HEXPRINT 

void RequestSend(Server *pServer, u32 type, struct evbuffer *evSend) {
    struct Poll *poll = pServer->poll;
#define COMPRESS
    struct evbuffer *evReq = evbuffer_new();
    struct Request Req, *pReq;
    bzero((char *) & Req, Size_Request);

    evbuffer_add(evReq, (char *) & Req, Size_Request);

    if (evSend > 0) {
        evbuffer_add_buffer(evReq, evSend);
    }

#ifdef DEBUG

    printf(cGREEN"try to send Request len:%d to %s:%d 0x%08x\n"cEND, evbuffer_get_length(evReq), pServer->host, pServer->port, pServer->poll->bev);
#endif
    pReq = (struct Request *) (EVBUFFER_DATA(evReq));
    pReq->sizes.UncmprSize = evbuffer_get_length(evReq) - Size_Request_sizes;
    pReq->hdr.TesterId = config.testerid;
    pReq->hdr.ReqType = type;
#ifdef DEBUG
    printf(cBLUE"\treq->hdr.TesterId=%d\n"cEND, pReq->hdr.TesterId);
    printf(cBLUE"\treq->hdr.ReqType=%d\n"cEND, pReq->hdr.ReqType);
#endif

#ifdef COMPRESS
    if (pReq->sizes.UncmprSize > 200) {
        int lenAlloc = (pReq->sizes.UncmprSize)*1.01 + 12 + Size_Request_sizes;
        int lenCmpr = lenAlloc - Size_Request_sizes;
        char *ptrCmpr;
        ptrCmpr = getNulledMemory(lenAlloc);
        memcpy(ptrCmpr, pReq, Size_Request_sizes);
        compress2((Bytef *) ptrCmpr + Size_Request_sizes, (uLongf *) & lenCmpr, (Bytef *) & pReq->hdr, evbuffer_get_length(evReq) - Size_Request_sizes, Z_DEFAULT_COMPRESSION);
        evbuffer_free(evReq);

        evReq = evbuffer_new();
        evbuffer_add(evReq, ptrCmpr, lenCmpr + Size_Request_sizes);
        free(ptrCmpr);
        pReq = (struct Request *) EVBUFFER_DATA(evReq);
        pReq->sizes.CmprSize = lenCmpr;
    }

#endif
    pReq->sizes.crc = crc32(0xffffffff, (const Bytef *) pReq, evbuffer_get_length(evReq));
#ifdef DEBUG
    printf(cBLUE"\treq->sizes.CmprSize=%d\n"cEND, pReq->sizes.CmprSize);
    printf(cBLUE"\treq->sizes.UncmprSize=%d\n"cEND, pReq->sizes.UncmprSize);
    printf(cBLUE"\treq->sizes.crc=0x%08x\n"cEND, pReq->sizes.crc);
#ifdef HEXPRINT
    hexPrint((char *) EVBUFFER_DATA(evReq), evbuffer_get_length(evReq));
#endif
#endif

    bufferevent_write_buffer(poll->bev, evReq);
    evbuffer_free(evReq);
}

void LoadTask(Server *pServer) {
#ifdef DEBUG
    printf(cGREEN"CRON"cEND" -> getConfigFromServer\n");
#endif

    if (!evBuffer) {
        evBuffer = evbuffer_new();
    }
    evbuffer_add(evBuffer, version, sizeof (version));
    RequestSend(pServer, MODULE_READ_OBJECTS_CFG, evBuffer);
};

void onLoadTask(Server *pServer) {
    struct Poll *poll = pServer->poll;
    struct _Tester_Cfg_AddData *Cfg_AddData;
    struct _Tester_Cfg_Record *Cfg_Record;
    struct Request *pReq, *pReqCmpr;
    struct evbuffer *buffer = EVBUFFER_INPUT(poll->bev);
    u_char *data = EVBUFFER_DATA(buffer);
    u_int len = EVBUFFER_LENGTH(buffer);
    char *ptr = 0;
    u32 crc = 0;

    pReq = (struct Request *) data;
    if (len < Size_Request) {
        return;
    }
    if (pReq->sizes.UncmprSize < Size_Request_hdr or pReq->sizes.UncmprSize > MAX_UNCMPR or(pReq->sizes.CmprSize > 0 and pReq->sizes.CmprSize > MAX_CMPR)) {
        return;
    }


    crc = pReq->sizes.crc;
    pReq->sizes.crc = 0;

    if (crc32(0xffffffff, (const Bytef *) pReq, (pReq->sizes.CmprSize > 0 ? pReq->sizes.CmprSize : pReq->sizes.UncmprSize) + Size_Request_sizes) != crc) {
        return;
    }

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

    Cfg_AddData = (struct _Tester_Cfg_AddData *) & pReq->Data;
    gettimeofday(&tv, NULL);
    pServer->timeOfLastUpdate = tv.tv_sec;

    config.TimeStabilization = tv.tv_sec - Cfg_AddData->ServerTime;
#ifdef DEBUG
    printf("tv.tv_sec - Cfg_AddData->ServerTime (%d - %d) = %d\n", tv.tv_sec, Cfg_AddData->ServerTime, config.TimeStabilization);
#endif
    int RecordsLen = (pReq->sizes.UncmprSize - sizeof (* Cfg_AddData) - Size_Request);
    int RecordOffset;

    gettimeofday(&tv, NULL);
    for (
            RecordOffset = sizeof (* Cfg_AddData);
            RecordOffset < RecordsLen;
            RecordOffset += sizeof (* Cfg_Record)
            ) {
        //        Cfg_Record = (struct _Tester_Cfg_Record *) (&pReq->Data + RecordOffset);
        Cfg_Record = (void *) & pReq->Data + RecordOffset;
#ifdef DEBUG
        /*
                printf("\tid объекта тестирования = %x\n", Cfg_Record->LObjId);
                printf("\tтип модуля тестирования (пинг-порт-хттп-...) = %s\n", getModuleText(Cfg_Record->ModType));

                printf("\tпорт првоерки = %hd\n", (short) Cfg_Record->Port);
                printf("\tпериодичность проверки объекта в секундах = %d\n", Cfg_Record->CheckPeriod);
                printf("\tпериодичность проверки ИП-адреса в секундах, или 0, если проверку делать не нужно = %d\n", Cfg_Record->ResolvePeriod);
                printf("\tип объекта тестирования = %s\n", ipString(Cfg_Record->IP));
                printf("\tдата ближайшей проверки. unixtime utc = %x\n", Cfg_Record->NextCheckDt);
                printf("\tимя хоста = %s\n", Cfg_Record->HostName);
                printf("\tLObjId предыдущего хоста в folded-цепочке или 0 = %d\n", Cfg_Record->FoldedNext);
                printf("\tLObjId следующего хоста в folded-цепочке или 0 = %d\n", Cfg_Record->FoldedPrev);
                printf("\tТаймаут для проверки = %d\n", Cfg_Record->TimeOut);
                printf("\tРазмер дополнительных данных = %d\n", Cfg_Record->ConfigLen);
         */
#endif

        if (Cfg_Record->TimeOut == 0 or Cfg_Record->TimeOut >= config.timeout * 950)
            Cfg_Record->TimeOut = config.timeout * 950;

        switch (Cfg_Record->ModType) {
            case MODULE_PING:
                addICMPTask(pServer, Cfg_Record, Cfg_Record->NextCheckDt - config.TimeStabilization - tv.tv_sec);
                break;
            case MODULE_TCP_PORT:
                addTCPTask(pServer, Cfg_Record, Cfg_Record->NextCheckDt - config.TimeStabilization - tv.tv_sec);
                break;
            case MODULE_DNS:
                RecordOffset += Cfg_Record->ConfigLen;
                addDNSTask(pServer, Cfg_Record, Cfg_Record->NextCheckDt - config.TimeStabilization - tv.tv_sec, (char *) (Cfg_Record + 1));
            default:
                RecordOffset += Cfg_Record->ConfigLen;
                addLuaTask(pServer, Cfg_Record, Cfg_Record->NextCheckDt - config.TimeStabilization - tv.tv_sec, (char *) (Cfg_Record + 1));
                break;
        }
    }
    pServer->flagRetriveConfig = 0;

    if (pReq->sizes.CmprSize > 0) {
        free(ptr);
    }

}

void onEventFromServer(Server *pServer, short action) {
    struct Poll *poll = pServer->poll;
    struct evbuffer *buffer = EVBUFFER_INPUT(poll->bev);
    u_char *data = EVBUFFER_DATA(buffer);
    u_int len = EVBUFFER_LENGTH(buffer);
#ifdef DEBUG
    printf(cGREEN"PROCESS"cEND" -> %s[%d] / %s[%d]\n", getActionText(action), action, getStatusText(poll->status), poll->status);
#ifdef HEXPRINT
    hexPrint((char *) data, len);
#endif
#endif

    if (len <= 4) {
        if (data[0] == '-')
            onErrorFromServer(pServer, atoi((char *) data));
    } else {

        switch (poll->type) {
            case MODE_SERVER:
                if (pServer->flagRetriveConfig) onLoadTask(pServer);
                if (pServer->flagSendReport) SendReport(pServer);
                if (pServer->flagSendReportError) SendReportError(pServer);
                break;
        }
    }
    if (len > 0) {
        evbuffer_drain(buffer, EVBUFFER_LENGTH(buffer));
    }
}

