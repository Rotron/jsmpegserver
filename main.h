#ifndef __MAIN_H__
#define __MAIN_H__

#include "websocketd.h"

struct topiclist {
    char *topic;
    struct clientlist *clientlist;
    int breakpoint;
    struct topiclist *tail;
};

void addnewclient(struct lws *w, char *topic);
void removeclient (struct lws *lws);
struct topiclist *findtopicobj (const char *topic);
struct topiclist *gettopiclisthead ();
void showtopology (char *filename, int line);

#endif