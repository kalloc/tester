#include "include.h"

u32 counterReport = 0;
u32 counterReportError = 0;

LIST_HEAD(ReportListHead, ReportListEntry) ReportHead;
struct ReportListHead *ReportHeadPtr; /* List head. */

LIST_HEAD(ReportErrorListHead, ReportListEntry) ReportErrorHead;
struct ReportErrorListHead *ReportErrorHeadPtr; /* List head. */

struct ReportListEntry {
    u16 ModType; // тип модуля тестирования (пинг-порт-хттп-...)
    u32 Len;
    void *Data;
    LIST_ENTRY(ReportListEntry) entries; /* List. */
} *ReportEntryPtr;

/*
 * добавляем репорт в очередь
 * ReportError  - обрабатывается раз в секунду
 * Report       - как происходит обновление данных с сервера
 */

void addICMPReport(struct Task * task) {
    struct _chg_ping *ping;
    struct st_icmp_poll * poll = (struct st_icmp_poll *) (task->poll);
#ifdef DEBUG
    printf(cGREEN"REPORT"cEND" ADD -> id %d, Module %s for %s [%s:%d]\n", task->Record.LObjId, getModuleText(task->Record.ModType), task->Record.HostName, ipString(task->Record.IP), task->Record.Port);
#endif
    ReportEntryPtr = malloc(sizeof (struct ReportListEntry)); /* Insert at the head. */

    ReportEntryPtr->ModType = MODULE_PING;
    ReportEntryPtr->Len = sizeof (struct _chg_ping);
    ping = malloc(ReportEntryPtr->Len);
    ping->LObjId = task->LObjId;
    ping->IP = task->Record.IP;
    ping->CheckDt = poll->CheckDt.tv_sec;
    ping->CheckOk = poll->CheckOk;
    ping->DelayMS = poll->DelayMS;
    ping->ProblemIP = poll->ProblemIP;
    ping->ProblemICMP_Code = poll->ProblemICMP_Code;
    ping->ProblemICMP_Type = poll->ProblemICMP_Type;
    ReportEntryPtr->Data = (char *) ping;

    if (task->code == STATE_DONE) {
        counterReport++;
    } else {
        counterReportError++;
    }
    if (ping->CheckOk) {
        LIST_INSERT_HEAD(&ReportHead, ReportEntryPtr, entries);

    } else {
        LIST_INSERT_HEAD(&ReportErrorHead, ReportEntryPtr, entries);
    }
}

void addTCPReport(struct Task * task) {
    struct _chg_tcp *tcp;
    struct st_tcp_poll * poll = (struct st_tcp_poll *) (task->poll);
#ifdef DEBUG
    printf(cGREEN"REPORT"cEND" ADD -> id %d, Module %s for %s [%s:%d]\n", task->Record.LObjId, getModuleText(task->Record.ModType), task->Record.HostName, ipString(task->Record.IP), task->Record.Port);
#endif
    ReportEntryPtr = malloc(sizeof (struct ReportListEntry)); /* Insert at the head. */

    ReportEntryPtr->ModType = MODULE_TCP_PORT;
    ReportEntryPtr->Len = sizeof (struct _chg_tcp);
//    ReportEntryPtr->ModType = MODULE_PING;
//    ReportEntryPtr->Len = sizeof (struct _chg_ping);
    tcp = malloc(ReportEntryPtr->Len);
    tcp->LObjId = task->LObjId;
    tcp->IP = task->Record.IP;
    tcp->CheckDt = poll->CheckDt.tv_sec;
    tcp->CheckOk = task->code == STATE_DONE ? 1 : 0;
    tcp->DelayMS = poll->DelayMS;
    tcp->ProblemIP = poll->ProblemIP;
    tcp->ProblemICMP_Code = poll->ProblemICMP_Code;
    tcp->ProblemICMP_Type = poll->ProblemICMP_Type;
    tcp->Port = poll->Port;


    ReportEntryPtr->Data = (char *) tcp;

    if (task->code == STATE_DONE) {
        counterReport++;
    } else {
        counterReportError++;
    }
    if (tcp->CheckOk) {
        LIST_INSERT_HEAD(&ReportHead, ReportEntryPtr, entries);

    } else {
        LIST_INSERT_HEAD(&ReportErrorHead, ReportEntryPtr, entries);
    }
}

u32 countReport() {
    return counterReport;
}

u32 countReportError() {
    return counterReportError;
}

void _SendReport(struct Poll *poll, short error) {
#define MODULE_COUNT 10
    char *ptr[MODULE_COUNT], *ptroff;
    u32 len[MODULE_COUNT];
    short counter;
    bzero(len, MODULE_COUNT * sizeof (int));

    LIST_HEAD(ReportListHead, ReportListEntry) *Head;
    if (error) {
        Head = &ReportErrorHead;
    } else {
        Head = &ReportHead;
    }

    LIST_FOREACH(ReportEntryPtr, Head, entries) {
        if (len[ReportEntryPtr->ModType] == 0) {
            ptroff = ptr[ReportEntryPtr->ModType] = malloc(ReportEntryPtr->Len);
        } else {
            ptroff = realloc(ptr[ReportEntryPtr->ModType], len[ReportEntryPtr->ModType] + ReportEntryPtr->Len);

        }
        memcpy(ptroff, ReportEntryPtr->Data, len[ReportEntryPtr->ModType] + ReportEntryPtr->Len);
        len[ReportEntryPtr->ModType] += ReportEntryPtr->Len;
        LIST_REMOVE(ReportEntryPtr, entries);
        free(ReportEntryPtr->Data);
        free(ReportEntryPtr);
    }

    for (counter = 0; counter < MODULE_COUNT; counter++) {
        if (len[counter] > 0) {
            printf("%s %d\n", getModuleText(counter), len[counter]);
            RequestSend(poll, counter, ptr[counter], len[counter]);
            //RequestSend(poll, MODULE_PING, ptr[counter], len[counter]);
            free(ptr[counter]);
        }
    }

    if (error) {
        counterReportError = 0;
        config.flagSendReportError = 0;
    } else {
        counterReport = 0;
        config.flagSendReport = 0;
    }

};

void SendReport(struct Poll *poll) {
    if (counterReport == 0)return;
#ifdef DEBUG
    printf(cGREEN"REPORT"cEND" SEND -> id %d\n", counterReport);
#endif
    _SendReport(poll, FALSE);
}

void SendReportError(struct Poll *poll) {
    if (counterReportError == 0)return;
#ifdef DEBUG
    printf(cGREEN"REPORT ERROR"cEND" SEND -> id %d\n", counterReportError);
#endif
    _SendReport(poll, TRUE);
}

void initReport() {
    counterReport = 0;
    counterReportError = 0;
    LIST_INIT(&ReportHead);
    LIST_INIT(&ReportErrorHead);
}