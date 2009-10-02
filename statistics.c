#include "include.h"

void incStat(int module, int state) {
    if (state == STATE_TIMEOUT || state == STATE_CONNECTING) {
        config.stat[module].TIMEOUT += 1;
    } else if (state == STATE_DONE) {
        config.stat[module].OK += 1;
    } else if (state == STATE_ERROR) {
        config.stat[module].ERROR += 1;
    } else {
        printf("не понятно откуда  в модуле %s, взялась кривая ошибка — %s\n",getModuleText(module),getStatusText(state));
    }
    config.stat[module].TOTAL += 1;

/*
    if (config.stat[module].tv.tv_sec == 0) {
        gettimeofday(&config.stat[module].tv, NULL);
    }
*/

}
