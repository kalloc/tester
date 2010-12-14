#define _GNU_SOURCE     /* Expose declaration of tdestroy() */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <search.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>
#include <err.h>
#include <time.h>
#include <ctype.h>

#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/queue.h>

#include <arpa/inet.h>
#include <arpa/nameser.h>

#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>


#include <ares.h>
#include <ares_dns.h>
#include <ares_version.h>


#include <event2/event.h>
#include <event2/listener.h>
#include <event2/event_struct.h>
#include <event2/event_compat.h>
#include <event2/bufferevent.h>
#include <event2/bufferevent_compat.h>
#include <event2/buffer.h>
#include <event2/buffer_compat.h>
#include <event2/util.h>
#include <event2/thread.h>

#include <pthread.h>

/*
 *
 *		misc define
 *
 */


#define SOCK_RC_TIMEOUT               -1
#define SOCK_RC_AUTH_FAILURE          -2  // Ошибка авторизации
#define SOCK_RC_WRONG_SIZES           -3  // В пакете данных указаны недопустимые размеры пакета
#define SOCK_RC_NOT_ENOUGH_MEMORY     -4
#define SOCK_RC_CRC_ERROR             -5
#define SOCK_RC_DECOMPRESS_FAILED     -6
#define SOCK_RC_INTEGRITY_FAILED      -7  // Длина пакета недопустима при указанной команде
#define SOCK_RC_SQL_ERROR             -8  // При обработке команды возникла ошибка работы с БД
#define SOCK_RC_UNKNOWN_REQUEST       -9  // Запрошена неизвестная команда
#define SOCK_RC_TOO_MUCH_CONNECTIONS  -10
#define SOCK_RC_SERVER_SHUTTING_DOWN  -11
#define TESTER_SQL_HOST_NAME_LEN      128



//активация изменений
#define CFG_2
#define CFG_3


//отправлять одни пакетом
#define ONEPACKET

#ifndef DEBUG
#define DEBUG
#define HEXPRINT
#endif

#define MODULE_COUNT        1024
#define OR                  ||
#define AND                 &&
#define or                  OR
#define and                 AND
#define cBLUE               "\e[1;34m"
#define cYELLOW             "\e[1;33m"
#define cGREEN              "\e[1;32m"
#define cRED                "\e[1;31m"
#define cEND                "\e[0m"
#define BUFLEN              1024
#define TTL                 3600
#define TRUE                1
#define FALSE               0
#define KEYLEN              128
#define MAX_UNCMPR          1024*1024*5
#define MAX_CMPR            1024*512
//hack (:
#define u8                  u_int8_t
#define u16                 u_int16_t
#define u32                 u_int32_t
#define u64                 u_int64_t
#define s8                  int8_t
#define s16                 int16_t
#define s32                 int32_t
#define s64                 int64_t

#ifdef __x86_64__
#define PLATFORM            "64"
#else
#define PLATFORM            "32"
#endif

//log level
#define LOG_NONE            0
#define LOG_NOTICE          (1<<0)
#define LOG_INFO            (1<<1)
#define LOG_WARN            (1<<2)
#define LOG_DEBUG           (1<<3)


#define STATE_DISCONNECTED  0
#define STATE_CONNECTING    (1<<0)
#define STATE_CONNECTED     (1<<1)
#define STATE_DONE          (1<<2)
#define STATE_ERROR         (1<<3)
#define STATE_SESSION       (1<<4)
#define STATE_QUIT          (1<<5)
#define STATE_TIMEOUT       (1<<6)
#define STATE_WRITE         (1<<7)
#define STATE_READ          (1<<8)

#define emptyRequestSend(poll, type)  RequestSend(poll, type, NULL)
#define freeConnection(poll) closeConnection(poll,TRUE)

#define initMainVars()         struct timeval tv;\
    struct rlimit rlimit;\
    mainBase=event_init();\
    rlimit.rlim_cur=100000;\
    rlimit.rlim_max=100000;\
    bzero(&config, sizeof (struct st_config));\
    timerclear(&tv);\
    setrlimit(RLIMIT_NOFILE,&rlimit);

#define setEnd(t) t->isEnd=TRUE;

/*
 *
 *		misc structure
 *
 */
enum {
    MODE_SERVER, MODE_CONFIG, MODE_VERIFER
};

enum MODULE_TYPE {
    MODULE_PING = 1, // Тестер сообщает данные ping-проверки
    MODULE_TCP_PORT = 2, // Тестер сообщает данные tcp-port-проверки
    MODULE_HTTP = 3, // Тестер сообщает данные http-проверки
    MODULE_SMTP = 4, // Тестер сообщает данные smtp-проверки
    MODULE_FTP = 5, // Тестер сообщает данные ftp-проверки
    MODULE_DNS = 6, // Тестер сообщает данные dns-проверки
    MODULE_POP = 7, // Тестер сообщает данные pop3-проверки
    MODULE_TELNET = 8, // Тестер сообщает данные pop3-проверки
    MODULE_LAST,
    MODULE_READ_OBJECTS_CFG = 1000, // Тестер запрашивает свои рабочие данные
    MODULE_CHECK_TASKS
};

enum {
    DNS_SUBTASK,
    DNS_RESOLV,
    DNS_TASK,
    DNS_GETNS
};

struct nv {
    const char *name;
    int value;
};


#define Size_Request_hdr sizeof(pReq->hdr)
#define Size_Request_sizes sizeof(pReq->sizes)
#define Size_Request (sizeof(struct Request)-sizeof(char *))

#define TESTER_FLAG_HAS_TRACE         (1<<0)
#define TESTER_FLAG_CHECK_TASK        (1<<1)
#define TESTER_FLAG_HAS_RAW_DATA      (1<<2)
#define TESTER_FLAG_CHANGE_CFGVER     (1<<3)

#ifdef DEBUG
#define debug(...) loger(__FILE__,__FUNCTION__,LOG_DEBUG, __VA_ARGS__);
#else
#define debug(...)
#endif
#define warn(...) loger(__FILE__,__FUNCTION__,LOG_WARN, __VA_ARGS__);
#define notice(...) loger(__FILE__,__FUNCTION__,LOG_NOTICE, __VA_ARGS__);
#define info(...) loger(NULL,NULL,LOG_INFO, __VA_ARGS__);

#define RemoteToLocalTime(x)  (x+config.TimeStabilization)
#define LocalToRemoteTime(x)  (x-config.TimeStabilization)


#define MSToTimeval(ms,tv)         timerclear(&tv);\
        if (ms > 1000) {\
            tv.tv_sec = ms / 1000 / 2;\
            tv.tv_usec = (ms % 1000)*1000 / 2;\
        } else {\
            tv.tv_sec = 0;\
            tv.tv_usec = ms * 1000 / 2;\
        }

//typedef struct {
//    char *ptr;
//    u_int len;
//} Data;

struct st_session {
    char garbage[16]; // заполняется случайными данными
    char password[48]; // текстовый нуль-терминированный пароль сессии
};


#pragma pack(push)  /* push current alignment to stack */
#pragma pack(1)     /* set alignment to 1 byte boundary */

struct Request {
    //1. Описатель длины/целостности пакета

    struct _st_sizes {
        u32 CmprSize; // Размер блока данных в сжатом виде, или 0, если данные несжаты
        u32 UncmprSize; // Размер блока данных
        u32 crc; // crc32()^0xFFFFFFFF блока данных
    } sizes;

    //2. Заголовок данных

    struct _st_t_hdr {
        u32 TesterId; // ID тестера
        u32 ReqType; // Код команды/moduleid
    } hdr;
    char *Data;
};

struct _Tester_Cfg_Record {
    u32 LObjId; // id объекта тестирования

    u16 ModType; // тип модуля тестирования (пинг-порт-хттп-...)

    u16 Port; // порт првоерки
    u32 CheckPeriod; // периодичность проверки объекта в секундах
    u32 ResolvePeriod; // периодичность проверки ИП-адреса в секундах, или 0, если проверку делать не нужно
    u32 IP; // ип объекта тестирования
    u32 NextCheckDt; // дата ближайшей проверки. unixtime utc
    char HostName[TESTER_SQL_HOST_NAME_LEN]; // имя хоста
    u32 FoldedNext; // LObjId предыдущего хоста в folded-цепочке или 0
    u32 FoldedPrev; // LObjId следующего хоста в folded-цепочке или 0
    u32 TimeOut;
#ifdef CFG_3
    s64              LimWarn;
    s64              LimAlrt;
    u8               LimDir;
#endif
#ifdef CFG_2
    u8 CFGVer;
#endif
    u32 ConfigLen;
};

struct _Tester_Cfg_AddData {
    u32 ServerTime; // Сервер сообщает свое текущее время. unixtime utc
#ifdef CFG_2
    u32 CfgRereadPeriod; // Tester reread config period
    u32 BODMaxAge; // Operdata DB cache MaxAge
    u32 CHGMaxAge; // CHG DB cache MaxAge
#endif

};

struct _Verifer_Cfg_Record {
    u32 LObjId; // id объекта тестирования

    u32 ModType; // тип модуля тестирования (пинг-порт-хттп-...)

    u16 Port; // порт првоерки
    u32 IP; // ип объекта тестирования
    u32 NextCheckDt; // дата ближайшей проверки. unixtime utc
    char HostName[TESTER_SQL_HOST_NAME_LEN]; // имя хоста
    u32 TimeOut;
#ifdef CFG_3
    s64              LimWarn;
    s64              LimAlrt;
    u8               LimDir;
#endif

#ifdef CFG_2
    u8 CFGVer;
#endif
    u32 ConfigLen;
};

struct _chg_tcp {
    u32 LObjId; // id объекта мониторинга
    u32 CheckDt; // дата проверки, unixtime utc
    u32 IP; // ип тестировавшегося объекта
    s32 CheckRes; // нормированный результат проверки. 1=ок, или 0.
    u16 DelayMS; // время получения результата в мс, или 0xFFFF если ответ не был получен
    u32 ProblemIP; // ИП сообщивший о недоступности/проблемах или 0
    u16 ProblemICMP_Type; // icmp.type проблемы от хоста ProblemIP
    u16 ProblemICMP_Code; // icmp.code проблемы от хоста ProblemIP
#ifdef CFG_3
    u8               CheckStatus;
#endif
    u8 Flags;
#ifdef CFG_2
    u8 CFGVer;
#endif
    u16 Port; // порт, по которому проводилась проверка
};

struct _chg_ping {
    u32 LObjId; // id объекта мониторинга
    u32 CheckDt; // дата проверки, unixtime utc
    u32 IP; // ип тестировавшегося объекта
    u32 CheckRes; // нормированный результат проверки. 1=ок, или 0.
    u16 DelayMS; // время получения результата в мс, или 0xFFFF если ответ не был получен
    u32 ProblemIP; // ИП сообщивший о недоступности/проблемах или 0
    u16 ProblemICMP_Type; // icmp.type проблемы от хоста ProblemIP
    u16 ProblemICMP_Code; // icmp.code проблемы от хоста ProblemIP
#ifdef CFG_3
    u8               CheckStatus;
#endif
    u8 Flags;
#ifdef CFG_2
    u8 CFGVer;
#endif
};

#pragma pack(pop)

struct cond_wait {
    pthread_mutex_t lock;
    pthread_cond_t cond;
};

struct Poll {
    struct event ev;
    struct bufferevent *bev;
    struct sockaddr_in client;
    unsigned status;
    char type;
    int fd;
};

struct stICMPInfo {
    struct timeval CheckDt; // дата проверки, unixtime utc
    u16 DelayMS; // время получения результата в мс, или 0xFFFF если ответ не был получен
    u32 ProblemIP; // ИП сообщивший о недоступности/проблемах или 0
    u16 ProblemICMP_Type; // icmp.type проблемы от хоста ProblemIP
    u16 ProblemICMP_Code; // icmp.code проблемы от хоста ProblemIP
    u16 id;
};

struct stTCPUDPInfo {
    struct bufferevent *bev;
    int fd;
    struct timeval CheckDt; // дата проверки, unixtime utc
    u16 DelayMS; // время получения результата в мс, или 0xFFFF если ответ не был получен
    u32 ProblemIP; // ИП сообщивший о недоступности/проблемах или 0
    u16 ProblemICMP_Type; // icmp.type проблемы от хоста ProblemIP
    u16 ProblemICMP_Code; // icmp.code проблемы от хоста ProblemIP
    u16 id;
    u16 Port; // порт, по которому проводилась проверка
    u8 Protocol;
};

struct struct_LuaTask {
    LIST_HEAD(LuaNetListHead, LuaNetListEntry) LuaNetListHead;
    struct Task *ptask;
    char *ptr;
    int ref;
    unsigned issleep : 1;
    struct event sleep;
    struct struct_LuaModule *luaModule;
    lua_State *state;
};

struct struct_LuaModule {
    int ModType;
    char name[56];
    lua_State *state;
    struct event_base *base;
    struct event timer;
    pthread_t threads;
    struct timespec mtime;
};

typedef struct st_server {
    struct Poll *poll;
    const char *host;
    int port;
    time_t timeout;
    u32 timeOfLastUpdate;

    struct st_session session;
    char passwordRecv[48]; // текстовый нуль-терминированный пароль сессии для проверячного тестера

    struct {
        u32 counterReport;
        u32 counterReportError;
        LIST_HEAD(ReportListHead, ReportListEntry) ReportHead, ReportErrorHead;
    } report;

    unsigned isSC : 1;
    unsigned isDC : 1;
    unsigned isVerifer : 1;

    time_t periodRetrieve;
    time_t periodReport;
    time_t periodReportError;

    struct event evConfig;
    struct event evReportError;
    struct event evReport;

    unsigned flagRetriveConfig : 1;
    unsigned flagSendReportError : 1;
    unsigned flagSendReport : 1;

    struct st_key {
        u_char public[32];
        u_char shared[32];
        u_char secret[32];
    } key;

} Server;

typedef struct st_config {

    struct st_stat {
        u32 OK;
        u32 TOTAL;
        u32 TIMEOUT;
        u32 ERROR;
        struct timeval tv;
    } stat[100];

    struct st_lua {
        int period;
        char *path;
    } lua;
    char *log;
    Server *pServerSC;
    Server *pVerifer;
    Server *pVeriferDC;
    size_t maxInput;
    time_t timeout;
    time_t TimeStabilization;
    time_t minRecheckPeriod;
    time_t minPeriod;
    u_int testerid;
    int fd;
    int loglevel;

} Config;
Config config;

struct DNSTask {
    ares_channel channel;
    struct Task *task;
    struct event timer;
    struct event ev;
    short role;
    int type;

    u32 taskTTL;
    short taskType;
    char *taskPattern;
    u32 taskPatternLen;

    unsigned isNeed : 1;
    int fd;
    u32 NSIP;

    char CheckOk; // нормированный результат проверки. 1=ок, или 0.
    u16 DelayMS; // время получения результата в мс, или 0xFFFF если ответ не был получен
    struct timeval CheckDt; // дата проверки, unixtime utc


};

struct Task {
    u32 LObjId;
    struct event time_ev;
    unsigned code;
    unsigned isVerifyTask : 1;
    unsigned isHostAsIp : 1;
    unsigned isEnd : 1;
    unsigned isSub : 1;
    unsigned isRead : 1;
    unsigned isShiftActive : 1;
#ifdef CFG_2
    unsigned isCfgChange : 1;
#endif
    unsigned readedSize;
    struct event read;
    void *poll;
    void *ptr;
    Server *pServer;
    struct DNSTask *resolv;
    void (*callback)(struct Task *);
    struct _Tester_Cfg_Record Record;
    u16 timeShift;
    u32 timeOfLastUpdate;
    u32 newTimer;
};





/*
 * прототипы
 */

//main
void openSession(Server *, short);
void newConnectionTask(Server *);
void readFromServer(int, short, void *);

//Tools
void restart_handler(int );
void setNextTimer(struct Task *);
unsigned int openConfiguration(char *);
void base64_encode(char *, int, char *, int *);
int base64_decode(const unsigned char *, unsigned char *, int *);
void stackDump(lua_State *, int);
int timeDiffMS(struct timeval, struct timeval);
int timeDiffUS(struct timeval, struct timeval);
void *getNulledMemory(int size);
int hex2bin(char *, char *);
char * getErrorText(int);
char * getActionText(int);
char * getStatusText(int);
char * getModuleText(int);
char * ConnectedIpString(Server *);
char * ipString(u32);
void closeConnection(Server *, short);
void initToolsPtr();
void freeToolsPtr();
void hexPrint(char *, int);
inline unsigned char *skip_question(const unsigned char *, const unsigned char *, int);
char * bin2hex(unsigned char *, int );
unsigned char * genSharedKey(Server *, unsigned char *);
unsigned char * genPublicKey(Server *);
int in_cksum(u_short *, int);
void loger(char *, const char *, int, const char *, ...);
#define stack(L) stackDump(L,__LINE__)
void loadServerFromConfiguration(Server *, u32);
const char *type_name(int );
char * getRoleText(int) ;
inline int getDNSTypeAfterParseAnswer(unsigned char *, unsigned char *, int );

//Processer
void Process(Server *);
void initProcess();
void freeProcess();
void loadTask(Server *);
void processServer(Server *, short);
void timerRetrieveTask(int, short, void *);
void onEventFromServer(Server *, short);
void RequestSend(Server *, u32, struct evbuffer *);

//Tester
void initTester();

//Report

struct ReportListEntry {
    u16 ModType; // тип модуля тестирования (пинг-порт-хттп-...)
    u32 Len;
    void *Data;
    LIST_ENTRY(ReportListEntry) entries; /* List. */
} *ReportEntryPtr, *ReportEntryPtrForDC;
void SendReportError(Server *);
void SendReport(Server *);
u32 countReportError(Server *);
u32 countReport(Server *);
void addICMPReport(struct Task *);
void addTCPReport(struct Task *);
void addDNSReport(struct Task *);
void addReport(struct Task *);
void initReport();
void addLuaReport(struct Task *);

//Statistics
void incStat(int, int);

//Task
#define  addTCPTask(Server,Cfg,newShift) _addTCPTask(Server, Cfg, newShift, 0)
#define  addICMPTask(Server,Cfg,newShift) _addICMPTask(Server, Cfg, newShift, 0)
#define  addLuaTask(Server,Cfg,newShift,data) _addLuaTask(Server, Cfg, newShift, 0,data)
#define  addDNSTask(Server,Cfg,newShift,data) _addDNSTask(Server, Cfg, newShift, 0,data)

#define  addTCPVerifyTask(Server,Cfg) _addTCPTask(Server, Cfg, 0, 1)
#define  addICMPVerifyTask(Server,Cfg) _addICMPTask(Server, Cfg, 0, 1)
#define  addLuaVerifyTask(Server,Cfg,data) _addLuaTask(Server, Cfg, 0, 1,data)
#define  addDNSVerifyTask(Server,Cfg,data) _addDNSTask(Server, Cfg, 0, 1,data)

void _addTCPTask(Server *, struct _Tester_Cfg_Record *, int, int);
void _addICMPTask(Server *, struct _Tester_Cfg_Record *, int, int);
void _addLuaTask(Server *, struct _Tester_Cfg_Record *, int, int, char *);
void _addDNSTask(Server *, struct _Tester_Cfg_Record *, int, int, char *);
void deleteTask(struct Task *);
#define getTask(id) searchTask(id,TRUE)
#define createTask(id) searchTask(id,FALSE)

//Tester_ICMP
void OnDisposeICMPTask(struct Task *);
void timerICMPTask(int, short, void *);
void OnReadICMPTask(int, short, void *);
void OnWriteICMPTask(int, short, void *);
void initICMPTester();
struct event_base *getICMPBase();

//Tester_TCP
void timerTCPTask(int, short, void *);
void initTCPTester();
struct event_base *getTCPBase();

//Tester_LUA

struct LuaNetListEntry {
    struct Task *task;
    LIST_ENTRY(LuaNetListEntry) entries; /* List. */
} *LuaNetListEntryPtr;
void initLUATester();
#define getLua(name) searchLua(name,TRUE)
#define createLua(name) searchLua(name,FALSE)
void timerLuaTask(int, short, void *);
struct event_base *getLuaBase(int);

//Tester_DNS
void timerDNSTask(int, short, void *);
struct event_base *getDNSBase();
void initDNSTester();
//Resolver
void timerResolv(int, short, void *);
struct event_base *getResolvBase();
void initResolv();



//client
void timerSendReportError(int, short, void *);
void timerSendReport(int, short, void *);