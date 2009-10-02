#include "include.h"

Server *pServerDC = NULL;
struct ReportListHead *Head; /* List head. */
static pthread_mutex_t *send_mutex = NULL;

/*
 * добавляем репорт в очередь
 * ReportError  - обрабатывается раз в секунду
 * Report       - как происходит обновление данных с сервера
 */

void addICMPReport(struct Task * task) {
    static pthread_mutex_t *mutex=NULL;
    if (!mutex) {
        mutex=calloc(1,sizeof(*mutex));
        pthread_mutex_init(mutex, NULL);
    }
    pthread_mutex_lock(mutex);
    pthread_mutex_lock(send_mutex);

    struct _chg_ping *ping;
    struct stICMPInfo * poll = (struct stICMPInfo *) (task->poll);

    incStat(task->Record.ModType, task->code);

#ifdef DEBUG
    printf(cGREEN"REPORT"cEND" ADD -> id %d, Module %s for %s [%s:%d]\n", task->Record.LObjId, getModuleText(task->Record.ModType), task->Record.HostName, ipString(task->Record.IP), task->Record.Port);
#endif
    ReportEntryPtr = getNulledMemory(sizeof (struct ReportListEntry)); /* Insert at the head. */

    ReportEntryPtr->ModType = MODULE_PING;
    ReportEntryPtr->Len = sizeof (struct _chg_ping);
    ping = getNulledMemory(ReportEntryPtr->Len);
    ping->LObjId = task->LObjId;
    ping->IP = task->Record.IP;
    ping->CheckDt = poll->CheckDt.tv_sec - config.TimeStabilization;
    ping->CheckOk = poll->CheckOk;
    ping->DelayMS = task->code == STATE_DONE ? poll->DelayMS : 0xffff;
    ping->ProblemIP = poll->ProblemIP;
    ping->ProblemICMP_Code = poll->ProblemICMP_Code;
    ping->ProblemICMP_Type = poll->ProblemICMP_Type;
    ReportEntryPtr->Data = (char *) ping;

    if (pServerDC) {

        ReportEntryPtrForDC = getNulledMemory(sizeof (struct ReportListEntry));
        memcpy(ReportEntryPtrForDC, ReportEntryPtr, sizeof (struct ReportListEntry));
        ReportEntryPtrForDC->Data = getNulledMemory(ReportEntryPtr->Len);
        memcpy(ReportEntryPtrForDC->Data, ReportEntryPtr->Data, ReportEntryPtr->Len);

        if (task->code == STATE_DONE) {
            pServerDC->report.counterReport++;
        } else {
            pServerDC->report.counterReportError++;
        }
        if (ping->CheckOk) {
            LIST_INSERT_HEAD(&pServerDC->report.ReportHead, ReportEntryPtrForDC, entries);

        } else {
            LIST_INSERT_HEAD(&pServerDC->report.ReportErrorHead, ReportEntryPtrForDC, entries);
        }
    }

    if (task->code == STATE_DONE) {
        task->pServer->report.counterReport++;
    } else {
        task->pServer->report.counterReportError++;
    }
    if (ping->CheckOk) {
        LIST_INSERT_HEAD(&task->pServer->report.ReportHead, ReportEntryPtr, entries);

    } else {
        LIST_INSERT_HEAD(&task->pServer->report.ReportErrorHead, ReportEntryPtr, entries);
    }
    pthread_mutex_unlock(mutex);
    pthread_mutex_unlock(send_mutex);

}

void addTCPReport(struct Task * task) {
    static pthread_mutex_t *mutex;
    if (!mutex) {
        mutex=calloc(1,sizeof(*mutex));
        pthread_mutex_init(mutex, NULL);
    }
    pthread_mutex_lock(mutex);
    pthread_mutex_lock(send_mutex);

    struct _chg_tcp *tcp;
    struct stTCPUDPInfo * poll = (struct stTCPUDPInfo *) (task->poll);
    incStat(task->Record.ModType, task->code);

    ReportEntryPtr = calloc(1, sizeof (struct ReportListEntry)); /* Insert at the head. */

    ReportEntryPtr->ModType = task->Record.ModType;
    ReportEntryPtr->Len = sizeof (struct _chg_tcp);
    tcp = getNulledMemory(ReportEntryPtr->Len);
    tcp->LObjId = task->LObjId;
    tcp->IP = task->Record.IP;
    tcp->CheckDt = poll->CheckDt.tv_sec - config.TimeStabilization;
    tcp->CheckOk = task->code == STATE_DONE ? 1 : 0;
    tcp->DelayMS = task->code == STATE_DONE ? poll->DelayMS : 0xffff;
    tcp->ProblemIP = poll->ProblemIP;
    tcp->ProblemICMP_Code = poll->ProblemICMP_Code;
    tcp->ProblemICMP_Type = poll->ProblemICMP_Type;
    tcp->Port = task->Record.Port;

#ifdef DEBUG
    printf(cGREEN"REPORT"cEND" ADD -> id %d,State is %s, Module %s for %s [%s:%d] [ms: %d] [time test:%d]\n", task->Record.LObjId, getStatusText(task->code), getModuleText(task->Record.ModType), task->Record.HostName, ipString(task->Record.IP), task->Record.Port, tcp->DelayMS, tcp->CheckDt);
#endif

    ReportEntryPtr->Data = (char *) tcp;

    if (pServerDC) {
        ReportEntryPtrForDC = getNulledMemory(sizeof (struct ReportListEntry));
        memcpy(ReportEntryPtrForDC, ReportEntryPtr, sizeof (struct ReportListEntry));
        ReportEntryPtrForDC->Data = getNulledMemory(ReportEntryPtr->Len);
        memcpy(ReportEntryPtrForDC->Data, ReportEntryPtr->Data, ReportEntryPtr->Len);

        if (task->code == STATE_DONE) {
            pServerDC->report.counterReport++;
        } else {
            pServerDC->report.counterReportError++;
        }
        if (tcp->CheckOk) {
            LIST_INSERT_HEAD(&pServerDC->report.ReportHead, ReportEntryPtrForDC, entries);

        } else {
            LIST_INSERT_HEAD(&pServerDC->report.ReportErrorHead, ReportEntryPtrForDC, entries);
        }
    }


    if (task->code == STATE_DONE) {
        task->pServer->report.counterReport++;
    } else {
        task->pServer->report.counterReportError++;
    }
    if (tcp->CheckOk) {
        LIST_INSERT_HEAD(&task->pServer->report.ReportHead, ReportEntryPtr, entries);
    } else {
        LIST_INSERT_HEAD(&task->pServer->report.ReportErrorHead, ReportEntryPtr, entries);
    }

    pthread_mutex_unlock(mutex);
    pthread_mutex_unlock(send_mutex);

}

u32 countReport(Server *pServer) {
    return pServer->report.counterReport;
}

u32 countReportError(Server *pServer) {
    return pServer->report.counterReportError;
}

void _SendReport(Server *pServer, short error) {

    pthread_mutex_lock(send_mutex);
    struct evbuffer * ptr[MODULE_COUNT];
    short counter;

    bzero(ptr, MODULE_COUNT * sizeof (*ptr));

    if (error) {
        Head = &pServer->report.ReportErrorHead;
    } else {
        Head = &pServer->report.ReportHead;
    }

#ifdef ONEPACKET

    LIST_FOREACH(ReportEntryPtr, Head, entries) {
#ifdef DEBUG
        printf("\tPrepare to send LObjId %d, result %s \n", *(int*) ReportEntryPtr->Data, error ? "ERROR" : "OK");
#endif
        if (ptr[ReportEntryPtr->ModType] == 0) {
            ptr[ReportEntryPtr->ModType] = evbuffer_new();
        }

        evbuffer_add(ptr[ReportEntryPtr->ModType], ReportEntryPtr->Data, ReportEntryPtr->Len);
        LIST_REMOVE(ReportEntryPtr, entries);
        free(ReportEntryPtr->Data);
        free(ReportEntryPtr);
    }

    pthread_mutex_unlock(send_mutex);
    for (counter = 0; counter < MODULE_COUNT; counter++) {
        if (ptr[counter] > 0) {
            RequestSend(pServer, counter, ptr[counter]);
            evbuffer_free(ptr[counter]);
        }
    }
#else

    LIST_FOREACH(ReportEntryPtr, Head, entries) {

        RequestSend(poll, ReportEntryPtr->ModType, ReportEntryPtr->Data, ReportEntryPtr->Len);
        LIST_REMOVE(ReportEntryPtr, entries);
        free(ReportEntryPtr->Data);
        free(ReportEntryPtr);
    }
    pthread_mutex_unlock(send_mutex);
#endif

};

void SendReport(Server *pServer) {
#ifdef DEBUG
    struct timeval tv;
    gettimeofday(&tv, NULL);
    printf(cGREEN"REPORT"cEND" SEND %d, sec is %d[%d] %s:%d \n", pServer->report.counterReport, tv.tv_sec - config.TimeStabilization, config.TimeStabilization, pServer->host, pServer->port);
#endif
    if (pServer->report.counterReport == 0)return;
    _SendReport(pServer, FALSE);
    pServer->report.counterReport = 0;
    pServer->flagSendReport = 0;
#ifdef DEBUG
    printf("\n");
#endif

}

void SendReportError(Server *pServer) {
#ifdef DEBUG
    struct timeval tv;
    gettimeofday(&tv, NULL);
    printf(cGREEN"REPORT ERROR"cEND" SEND %d, sec is %d[%d] %s:%d \n", pServer->report.counterReportError, tv.tv_sec - config.TimeStabilization, config.TimeStabilization, pServer->host, pServer->port);
#endif
    if (pServer->report.counterReportError == 0) return;
    _SendReport(pServer, TRUE);
    pServer->report.counterReportError = 0;
    pServer->flagSendReportError = 0;
#ifdef DEBUG
    printf("\n");
#endif

}

void initReport(Server *pServer) {
    if (!send_mutex) {
        send_mutex=calloc(1,sizeof(*send_mutex));
        pthread_mutex_init(send_mutex, NULL);
    }
    if (pServer->isDC) {
        pServerDC = pServer;
    }
    pServer->report.counterReport = 0;
    pServer->report.counterReportError = 0;
    LIST_INIT(&pServer->report.ReportHead);
    LIST_INIT(&pServer->report.ReportErrorHead);
}

