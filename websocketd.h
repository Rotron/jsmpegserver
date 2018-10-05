#ifndef __WEBSOCKETD_H__
#define __WEBSOCKETD_H__

struct clientlist {
    struct lws *lws;
    struct clientlist *tail; 
};

void ws_publish(char *blob, size_t len, char *topic);
void websocketd_init();

#endif
