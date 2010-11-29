#include "include.h"
#include <zlib.h>
#ifndef __TIMESTAMP__
#define __TIMESTAMP__ __DATE__" "__TIME__
#endif

const char version[] = "Tester v0.09/l"PLATFORM" ["LUA_RELEASE",c-ares "ARES_VERSION_STR","_EVENT_PACKAGE" "_EVENT_VERSION"], Compiled:"__DATE__" "__TIME__", Source:"__FILE__","__TIMESTAMP__"";
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
    debug("%s[%d] host [%s:%d]", getErrorText(code), code, pServer->host, pServer->port);
}

void RequestSend(Server *pServer, u32 type, struct evbuffer *evSend) {
    struct Poll *poll = pServer->poll;
    //#define COMPRESS
    struct evbuffer *evReq = evbuffer_new();
    struct Request Req, *pReq;
    bzero((char *) & Req, Size_Request);

    evbuffer_add(evReq, (char *) & Req, Size_Request);

    if (evSend > 0) {
        evbuffer_add_buffer(evReq, evSend);
    }

    debug("len:%d to %s:%d 0x%08x", evbuffer_get_length(evReq), pServer->host, pServer->port, pServer->poll->bev);

    pReq = (struct Request *) (EVBUFFER_DATA(evReq));
    pReq->sizes.UncmprSize = evbuffer_get_length(evReq) - Size_Request_sizes;
    pReq->hdr.TesterId = config.testerid;
    pReq->hdr.ReqType = type;


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
    /*
        #ifdef DEBUG
            printf(cBLUE"\treq->sizes.CmprSize=%d"cEND, pReq->sizes.CmprSize);
            printf(cBLUE"\treq->sizes.UncmprSize=%d"cEND, pReq->sizes.UncmprSize);
            printf(cBLUE"\treq->sizes.crc=0x%08x\n"cEND, pReq->sizes.crc);
        #ifdef HEXPRINT
            hexPrint((char *) EVBUFFER_DATA(evReq), evbuffer_get_length(evReq));
        #endif
        #endif
     */

    bufferevent_write_buffer(poll->bev, evReq);
    evbuffer_free(evReq);
}

void loadTask(Server *pServer) {
    debug("from %s:%d", pServer->host, pServer->port);

    if (!evBuffer) {
        evBuffer = evbuffer_new();
    }
    evbuffer_add(evBuffer, version, sizeof (version));
    RequestSend(pServer, MODULE_READ_OBJECTS_CFG, evBuffer);
};

int onLoadTask(Server *pServer) {
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
        return TRUE;
    }
    if (pReq->sizes.UncmprSize < Size_Request_hdr or pReq->sizes.UncmprSize > MAX_UNCMPR or(pReq->sizes.CmprSize > 0 and pReq->sizes.CmprSize > MAX_CMPR)) {
        warn("mismatch length of packet");
        return TRUE;
    }


    crc = pReq->sizes.crc;
    pReq->sizes.crc = 0;

    if (crc32(0xffffffff, (const Bytef *) pReq, (pReq->sizes.CmprSize > 0 ? pReq->sizes.CmprSize : pReq->sizes.UncmprSize) + Size_Request_sizes) != crc) {
        pReq->sizes.crc = crc;
        return FALSE;
    }

    if (pReq->sizes.CmprSize) {
        ptr = malloc(pReq->sizes.UncmprSize + Size_Request_sizes);
        bzero(ptr, pReq->sizes.UncmprSize + Size_Request_sizes);

        memcpy(ptr, data, Size_Request_sizes);
        pReqCmpr = (struct Request *) ptr;

        uncompress((Bytef *) (ptr + Size_Request_sizes), (uLongf *) & pReqCmpr->sizes.UncmprSize, (const Bytef *) & pReq->hdr, (uLongf) pReq->sizes.CmprSize);

        if (pReqCmpr->sizes.UncmprSize != pReq->sizes.UncmprSize) {
            return TRUE;
        }
        pReq = pReqCmpr;

    }

    Cfg_AddData = (struct _Tester_Cfg_AddData *) & pReq->Data;
    if (Cfg_AddData->CfgRereadPeriod != pServer->periodRetrieve) {
        debug("set new period reread config from %d to %d", pServer->periodRetrieve, Cfg_AddData->CfgRereadPeriod);
        pServer->periodRetrieve = Cfg_AddData->CfgRereadPeriod;
        tv.tv_usec = 0;
        tv.tv_sec = pServer->periodRetrieve;
        evtimer_del(&pServer->evConfig);
        evtimer_set(&pServer->evConfig, timerRetrieveTask, pServer);
        evtimer_add(&pServer->evConfig, &tv);
    }


    gettimeofday(&tv, NULL);
    config.TimeStabilization = tv.tv_sec - Cfg_AddData->ServerTime;
    pServer->timeOfLastUpdate = tv.tv_sec;

    debug("remote time %d, new offset %d", Cfg_AddData->ServerTime, config.TimeStabilization);
    int RecordsLen = (pReq->sizes.UncmprSize - sizeof (* Cfg_AddData) - Size_Request);
    int RecordOffset;
    int newShift;
    for (
            RecordOffset = sizeof (* Cfg_AddData);
            RecordOffset < RecordsLen;
            RecordOffset += sizeof (* Cfg_Record)
            ) {
        //        Cfg_Record = (struct _Tester_Cfg_Record *) (&pReq->Data + RecordOffset);
        Cfg_Record = (void *) & pReq->Data + RecordOffset;
        if (Cfg_Record->TimeOut == 0 or Cfg_Record->TimeOut >= config.timeout * 950)
            Cfg_Record->TimeOut = config.timeout * 950;




#ifdef DEBUG
        debug("\tid объекта тестирования = %d", Cfg_Record->LObjId);
        debug("\tтип модуля тестирования (пинг-порт-хттп-...) = %s", getModuleText(Cfg_Record->ModType));

        debug("\tпорт првоерки = %hd", (short) Cfg_Record->Port);
        debug("\tпериодичность проверки объекта в секундах = %d", Cfg_Record->CheckPeriod);
        debug("\tпериодичность проверки ИП-адреса в секундах, или 0, если проверку делать не нужно = %d", Cfg_Record->ResolvePeriod);
        debug("\tип объекта тестирования = %s", ipString(Cfg_Record->IP));
        debug("\tдата ближайшей проверки. unixtime utc = %d", Cfg_Record->NextCheckDt);
        debug("\tимя хоста = %s", Cfg_Record->HostName);
        debug("\tLObjId предыдущего хоста в folded-цепочке или 0 = %d", Cfg_Record->FoldedNext);
        debug("\tLObjId следующего хоста в folded-цепочке или 0 = %d", Cfg_Record->FoldedPrev);
        debug("\tТаймаут для проверки = %d", Cfg_Record->TimeOut);
        debug("\tРазмер дополнительных данных = %d", Cfg_Record->ConfigLen);
#endif
        if (Cfg_Record->ConfigLen > RecordsLen) return TRUE;

        newShift = Cfg_Record->NextCheckDt % Cfg_Record->CheckPeriod;
        Cfg_Record->NextCheckDt = ((tv.tv_sec / Cfg_Record->CheckPeriod) * Cfg_Record->CheckPeriod) + RemoteToLocalTime(Cfg_Record->NextCheckDt) % Cfg_Record->CheckPeriod;
        if (Cfg_Record->NextCheckDt < tv.tv_sec) Cfg_Record->NextCheckDt += Cfg_Record->CheckPeriod;


        switch (Cfg_Record->ModType) {
            case MODULE_PING:
                addICMPTask(pServer, Cfg_Record, newShift);
                break;
            case MODULE_TCP_PORT:
                addTCPTask(pServer, Cfg_Record, newShift);
                break;
            case MODULE_DNS:
                RecordOffset += Cfg_Record->ConfigLen;
                addDNSTask(pServer, Cfg_Record, newShift, (char *) (Cfg_Record + 1));
                break;
            default:
                RecordOffset += Cfg_Record->ConfigLen;
                addLuaTask(pServer, Cfg_Record, newShift, (char *) (Cfg_Record + 1));
                break;
        }
    }
    pServer->flagRetriveConfig = 0;

    if (pReq->sizes.CmprSize > 0) {
        free(ptr);
    }
    return TRUE;

}

void onEventFromServer(Server *pServer, short action) {
    struct Poll *poll = pServer->poll;
    struct evbuffer *buffer = EVBUFFER_INPUT(poll->bev);
    u_char *data = EVBUFFER_DATA(buffer);
    u_int len = EVBUFFER_LENGTH(buffer);

    if (len <= 4) {
        if (data[0] == '-')
            onErrorFromServer(pServer, atoi((char *) data));
    } else {
        switch (poll->type) {
            case MODE_SERVER:
                if (pServer->flagRetriveConfig) {
                    if (!onLoadTask(pServer)) return;
                }
                if (pServer->flagSendReport) SendReport(pServer);
                if (pServer->flagSendReportError) SendReportError(pServer);
                break;
        }
    }
    if (len > 0) {
        evbuffer_drain(buffer, EVBUFFER_LENGTH(buffer));
    }
}

