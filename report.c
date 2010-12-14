#include "include.h"

Server *pServerSC = NULL;
struct ReportListHead *Head; /* List head. */
static pthread_mutex_t *send_mutex = NULL;
struct timeval tv;

void addICMPReport(struct Task * task) {
    static pthread_mutex_t *mutex = NULL;
    if (!mutex) {
        mutex = calloc(1, sizeof (*mutex));
        pthread_mutex_init(mutex, NULL);
    }
    debug("mutex_lock")

    pthread_mutex_lock(mutex);
    pthread_mutex_lock(send_mutex);

    struct _chg_ping *chg;
    struct stICMPInfo * poll = (struct stICMPInfo *) (task->poll);

    incStat(task->Record.ModType, task->code);

    ReportEntryPtr = getNulledMemory(sizeof (struct ReportListEntry)); /* Insert at the head. */

    ReportEntryPtr->ModType = MODULE_PING;
    ReportEntryPtr->Len = sizeof (struct _chg_ping);
    chg = getNulledMemory(ReportEntryPtr->Len);
    chg->LObjId = task->LObjId;
    chg->IP = task->Record.IP;
    chg->CheckDt = LocalToRemoteTime(poll->CheckDt.tv_sec);
    chg->CheckRes = task->code == STATE_DONE ? 0 : -1;
    #ifdef CFG_3
    chg->CheckStatus=(chg->CheckRes)?0:1;
    #endif
    gettimeofday(&tv, NULL);
    chg->DelayMS = task->code == STATE_DONE ? timeDiffMS(tv, poll->CheckDt) : 0xffff;
    chg->ProblemIP = poll->ProblemIP;
    chg->ProblemICMP_Code = poll->ProblemICMP_Code;
    chg->ProblemICMP_Type = poll->ProblemICMP_Type;
    if (task->isVerifyTask) {
        chg->Flags |= TESTER_FLAG_CHECK_TASK;
    }
    chg->CFGVer = task->Record.CFGVer;
    if (task->isCfgChange) {
        task->isCfgChange = 0;
        chg->Flags |= TESTER_FLAG_CHANGE_CFGVER;
        task->Record.CFGVer = task->Record.CFGVer == 249 ? 1 : task->Record.CFGVer + 1;
    }

    debug("%d -> Module %s for %s [%s:%d] [ms: %d] [time test:%d] Result %s",
            task->Record.LObjId,
            getModuleText(task->Record.ModType),
            task->Record.HostName, ipString(task->Record.IP), task->Record.Port,
            poll->DelayMS,
            poll->CheckDt.tv_sec,
            getStatusText(task->code));

    ReportEntryPtr->Data = (char *) chg;

    if (pServerSC) {

        ReportEntryPtrForDC = getNulledMemory(sizeof (struct ReportListEntry));
        memcpy(ReportEntryPtrForDC, ReportEntryPtr, sizeof (struct ReportListEntry));
        ReportEntryPtrForDC->Data = getNulledMemory(ReportEntryPtr->Len);
        memcpy(ReportEntryPtrForDC->Data, ReportEntryPtr->Data, ReportEntryPtr->Len);

        if (task->code == STATE_DONE) {
            pServerSC->report.counterReport++;
        } else {
            pServerSC->report.counterReportError++;
        }
        if (chg->CheckRes > -1) {
            LIST_INSERT_HEAD(&pServerSC->report.ReportHead, ReportEntryPtrForDC, entries);

        } else {
            LIST_INSERT_HEAD(&pServerSC->report.ReportErrorHead, ReportEntryPtrForDC, entries);
        }
    }

    if (task->code == STATE_DONE) {
        task->pServer->report.counterReport++;
    } else {
        task->pServer->report.counterReportError++;
    }
    if (chg->CheckRes > -1) {
        LIST_INSERT_HEAD(&task->pServer->report.ReportHead, ReportEntryPtr, entries);

    } else {
        LIST_INSERT_HEAD(&task->pServer->report.ReportErrorHead, ReportEntryPtr, entries);
    }
    debug("mutex_unlock")

    pthread_mutex_unlock(send_mutex);
    pthread_mutex_unlock(mutex);

}

void addTCPReport(struct Task * task) {
    static pthread_mutex_t *mutex;
    if (!mutex) {
        mutex = calloc(1, sizeof (*mutex));
        pthread_mutex_init(mutex, NULL);
    }
    pthread_mutex_lock(mutex);
    pthread_mutex_lock(send_mutex);
    debug("mutex_lock");

    struct _chg_tcp *chg;
    struct stTCPUDPInfo * poll = (struct stTCPUDPInfo *) (task->poll);
    incStat(task->Record.ModType, task->code);

    ReportEntryPtr = calloc(1, sizeof (struct ReportListEntry)); /* Insert at the head. */

    ReportEntryPtr->ModType = task->Record.ModType;
    ReportEntryPtr->Len = sizeof (struct _chg_tcp);
    chg = getNulledMemory(ReportEntryPtr->Len);
    chg->LObjId = task->LObjId;
    chg->IP = task->Record.IP;
    chg->CheckDt = LocalToRemoteTime(poll->CheckDt.tv_sec);
    chg->CheckRes = task->code == STATE_DONE ? 0 : -1;
    #ifdef CFG_3
	chg->CheckStatus=(chg->CheckRes)?0:1;
    #endif

    if (task->isVerifyTask) {
        chg->Flags |= TESTER_FLAG_CHECK_TASK;
    }
    chg->CFGVer = task->Record.CFGVer;
    if (task->isCfgChange) {
        task->isCfgChange = 0;
        chg->Flags |= TESTER_FLAG_CHANGE_CFGVER;
        task->Record.CFGVer = task->Record.CFGVer == 249 ? 1 : task->Record.CFGVer + 1;
    }
    gettimeofday(&tv, NULL);
    chg->DelayMS = task->code == STATE_DONE ? timeDiffMS(tv, poll->CheckDt) : 0xffff;
    chg->ProblemIP = poll->ProblemIP;
    chg->ProblemICMP_Code = poll->ProblemICMP_Code;
    chg->ProblemICMP_Type = poll->ProblemICMP_Type;
    chg->Port = task->Record.Port;

    debug("%d -> Module %s for %s [%s:%d] [ms: %d] [time test:%d] Flags is %d, Result %s, to %s",
            task->Record.LObjId,
            getModuleText(task->Record.ModType),
            task->Record.HostName, ipString(task->Record.IP), task->Record.Port,
            chg->DelayMS,
            poll->CheckDt.tv_sec,
            chg->Flags,
            getStatusText(task->code),
            task->pServer->host
            );

    ReportEntryPtr->Data = (char *) chg;

    if (pServerSC) {
        ReportEntryPtrForDC = getNulledMemory(sizeof (struct ReportListEntry));
        memcpy(ReportEntryPtrForDC, ReportEntryPtr, sizeof (struct ReportListEntry));
        ReportEntryPtrForDC->Data = getNulledMemory(ReportEntryPtr->Len);
        memcpy(ReportEntryPtrForDC->Data, ReportEntryPtr->Data, ReportEntryPtr->Len);

        if (task->code == STATE_DONE) {
            pServerSC->report.counterReport++;
        } else {
            pServerSC->report.counterReportError++;
        }
        if (chg->CheckRes > -1) {
            LIST_INSERT_HEAD(&pServerSC->report.ReportHead, ReportEntryPtrForDC, entries);

        } else {
            LIST_INSERT_HEAD(&pServerSC->report.ReportErrorHead, ReportEntryPtrForDC, entries);
        }
    }


    if (task->code == STATE_DONE) {
        task->pServer->report.counterReport++;
    } else {
        task->pServer->report.counterReportError++;
    }
    if (chg->CheckRes > -1) {
        LIST_INSERT_HEAD(&task->pServer->report.ReportHead, ReportEntryPtr, entries);
    } else {
        LIST_INSERT_HEAD(&task->pServer->report.ReportErrorHead, ReportEntryPtr, entries);
    }

    pthread_mutex_unlock(send_mutex);
    pthread_mutex_unlock(mutex);
    debug("mutex_unlock")

}

void addDNSReport(struct Task * task) {
    static pthread_mutex_t *mutex;
    if (!mutex) {
        mutex = calloc(1, sizeof (*mutex));
        pthread_mutex_init(mutex, NULL);
    }
    debug("mutex_lock")
    pthread_mutex_lock(mutex);
    pthread_mutex_lock(send_mutex);

    struct _chg_tcp *chg;
    struct DNSTask * dnstask = (struct DNSTask *) (task->poll);
    incStat(task->Record.ModType, task->code);

    ReportEntryPtr = calloc(1, sizeof (struct ReportListEntry)); /* Insert at the head. */

    ReportEntryPtr->ModType = task->Record.ModType;
    ReportEntryPtr->Len = sizeof (struct _chg_tcp);
    chg = getNulledMemory(ReportEntryPtr->Len);
    chg->LObjId = task->LObjId;
    chg->IP = task->Record.IP;
    if (task->isVerifyTask) {
        chg->Flags |= TESTER_FLAG_CHECK_TASK;
    }

    chg->CheckDt = LocalToRemoteTime(dnstask->CheckDt.tv_sec);
    chg->CheckRes = task->code == STATE_DONE ? 0 : -1;
    #ifdef CFG_3
	chg->CheckStatus=(chg->CheckRes)?0:1;
    #endif

    gettimeofday(&tv, NULL);
    chg->DelayMS = task->code == STATE_DONE ? timeDiffMS(tv, dnstask->CheckDt) : 0xffff;
    chg->Port = task->Record.Port;

    debug("%d -> Module %s for %s [%s:%d] [ms: %d] [time test:%d] Flags is %d Result %s, to %s",
            task->Record.LObjId,
            getModuleText(task->Record.ModType),
            task->Record.HostName, ipString(task->Record.IP), task->Record.Port,
            chg->DelayMS,
            dnstask->CheckDt.tv_sec,
            chg->Flags,
            getStatusText(task->code),
            task->pServer->host);

    ReportEntryPtr->Data = (char *) chg;

    if (pServerSC) {
        ReportEntryPtrForDC = getNulledMemory(sizeof (struct ReportListEntry));
        memcpy(ReportEntryPtrForDC, ReportEntryPtr, sizeof (struct ReportListEntry));
        ReportEntryPtrForDC->Data = getNulledMemory(ReportEntryPtr->Len);
        memcpy(ReportEntryPtrForDC->Data, ReportEntryPtr->Data, ReportEntryPtr->Len);

        if (task->code == STATE_DONE) {
            pServerSC->report.counterReport++;
        } else {
            pServerSC->report.counterReportError++;
        }
        if (chg->CheckRes > -1) {
            LIST_INSERT_HEAD(&pServerSC->report.ReportHead, ReportEntryPtrForDC, entries);

        } else {
            LIST_INSERT_HEAD(&pServerSC->report.ReportErrorHead, ReportEntryPtrForDC, entries);
        }
    }


    if (task->code == STATE_DONE) {
        task->pServer->report.counterReport++;
    } else {
        task->pServer->report.counterReportError++;
    }
    if (chg->CheckRes > -1) {
        LIST_INSERT_HEAD(&task->pServer->report.ReportHead, ReportEntryPtr, entries);
    } else {
        LIST_INSERT_HEAD(&task->pServer->report.ReportErrorHead, ReportEntryPtr, entries);
    }

    pthread_mutex_unlock(send_mutex);
    pthread_mutex_unlock(mutex);
    debug("mutex_unlock")

}

u32 countReport(Server *pServer) {
    return pServer->report.counterReport;
}

u32 countReportError(Server *pServer) {
    return pServer->report.counterReportError;
}

void inline debugPrintReport(struct ReportListEntry * ReportEntry) {
    debug("\tтип модуля тестирования (пинг-порт-хттп-...) = %s", getModuleText(ReportEntry->ModType));
    if (ReportEntry->ModType == MODULE_PING) {
        struct _chg_ping *report = (struct _chg_ping *) ReportEntry->Data;
        debug("\tid объекта тестирования = %d", report->LObjId);
        debug("\tдата проверки = %x", report->CheckDt);
        debug("\tип объекта тестирования = %s", ipString(report->IP));
        #ifdef CFG_3
    	    debug("\tстатус = %d", report->CheckStatus);
	#endif

        debug("\tрезультат = %d", report->CheckRes);
        debug("\tвремя ответа в мс  = %u", report->DelayMS);
        debug("\tИП сообщивший о недоступности/проблемах или 0 = %x", report->ProblemIP);
        debug("\ticmp.type проблемы от хоста ProblemIP = %x", report->ProblemICMP_Type);
        debug("\ticmp.code проблемы от хоста ProblemIP = %x", report->ProblemICMP_Code);
        debug("\tфлаги = %d", report->Flags);
        debug("\tверсия конфига  = %d", report->CFGVer);
    } else if (ReportEntry->ModType == MODULE_READ_OBJECTS_CFG) {

    } else {
        struct _chg_tcp *report = (struct _chg_tcp *) ReportEntry->Data;
        debug("\tid объекта тестирования = %d", report->LObjId);
        debug("\tдата проверки = %x", report->CheckDt);
        debug("\tип объекта тестирования = %s", ipString(report->IP));
        #ifdef CFG_3
    	    debug("\tстатус = %d", report->CheckStatus);
	#endif

        debug("\tрезультат = %d", report->CheckRes);
        debug("\tвремя ответа в мс  = %d", report->DelayMS);
        debug("\tИП сообщивший о недоступности/проблемах или 0 = %x", report->ProblemIP);
        debug("\ticmp.type проблемы от хоста ProblemIP = %x", report->ProblemICMP_Type);
        debug("\ticmp.code проблемы от хоста ProblemIP = %x", report->ProblemICMP_Code);
        debug("\tфлаги = %d", report->Flags);
        debug("\tверсия конфига  = %d", report->CFGVer);
        debug("\tпорт првоерки = %hd", (short) report->Port);
    }
    /*
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
     */
}

void _SendReport(Server *pServer, short error) {

    debug("mutex_lock")
    pthread_mutex_lock(send_mutex);
    struct evbuffer * ptr[MODULE_LAST];
    short counter;

    bzero(ptr, MODULE_LAST * sizeof (*ptr));

    if (error) {
        Head = &pServer->report.ReportErrorHead;
    } else {
        Head = &pServer->report.ReportHead;
    }

#ifdef ONEPACKET

    LIST_FOREACH(ReportEntryPtr, Head, entries) {

        if (ptr[ReportEntryPtr->ModType] == 0) {
            ptr[ReportEntryPtr->ModType] = evbuffer_new();
        }
/*
#ifdef DEBUG
        debugPrintReport(ReportEntryPtr);
#endif
*/

        evbuffer_add(ptr[ReportEntryPtr->ModType], ReportEntryPtr->Data, ReportEntryPtr->Len);
        LIST_REMOVE(ReportEntryPtr, entries);
        free(ReportEntryPtr->Data);
        free(ReportEntryPtr);
    }
    for (counter = 0; counter < MODULE_LAST; counter++) {
        if (ptr[counter] > 0) {
            RequestSend(pServer, counter, ptr[counter]);
            evbuffer_free(ptr[counter]);
        }
    }
    debug("mutex_unlock")
    pthread_mutex_unlock(send_mutex);
#else

    LIST_FOREACH(ReportEntryPtr, Head, entries) {

        RequestSend(poll, ReportEntryPtr->ModType, ReportEntryPtr->Data, ReportEntryPtr->Len);
        LIST_REMOVE(ReportEntryPtr, entries);
        free(ReportEntryPtr->Data);
        free(ReportEntryPtr);
    }
    pthread_mutex_unlock(send_mutex);
#endif

}

void SendReport(Server *pServer) {
    debug("%s:%d -> Send %d report", pServer->host, pServer->port, pServer->report.counterReport);
    if (pServer->report.counterReport == 0)return;
    _SendReport(pServer, FALSE);
    pServer->report.counterReport = 0;
    pServer->flagSendReport = 0;

}

void SendReportError(Server *pServer) {
    debug("%s:%d -> Send %d report", pServer->host, pServer->port, pServer->report.counterReportError);
    if (pServer->report.counterReportError == 0) return;
    _SendReport(pServer, TRUE);
    pServer->report.counterReportError = 0;
    pServer->flagSendReportError = 0;

}

void initReport(Server *pServer) {
    if (!send_mutex) {
        send_mutex = calloc(1, sizeof (*send_mutex));
        pthread_mutex_init(send_mutex, NULL);
    }
    if (pServer->isSC) {
        pServerSC = pServer;
    }
    pServer->report.counterReport = 0;
    pServer->report.counterReportError = 0;
    LIST_INIT(&pServer->report.ReportHead);
    LIST_INIT(&pServer->report.ReportErrorHead);
}

