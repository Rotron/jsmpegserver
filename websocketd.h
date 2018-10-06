#ifndef __WEBSOCKETD_H__
#define __WEBSOCKETD_H__

#include <libwebsockets.h>
#include <pthread.h>

struct clientlist {
    struct lws *lws;
    int mode;
    struct clientlist *tail; 
};

void ws_publish (const char *blob, size_t len, const char *topic);
void websocketd_init();

#endif
