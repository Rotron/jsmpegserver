#ifndef __MAIN_H__
#define __MAIN_H__

#include "websocketd.h"

struct DATAPIPE {
    char *blob;
    size_t len;
    int mode;
    struct DATAPIPE *tail;
};

struct topiclist {
    char *topic;
    struct clientlist *clientlist;
    struct DATAPIPE *datapipehead;
    struct DATAPIPE *datapipenow;
    struct topiclist *tail;
};

void addnewclient(struct lws *w, char *topic);
void removeclient (struct lws *lws);
struct topiclist *findtopicobj (const char *topic);
struct topiclist *gettopiclisthead ();
void showtopology (char *filename, int line);

#endif