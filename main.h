#ifndef __MAIN_H__
#define __MAIN_H__

#include "websocketd.h"

struct topiclist {
    char *topic;
    struct clientlist *clientlist;
    struct topiclist *tail;
};

void addnewclient(struct lws *w, char *topic);
void removeclient (struct lws *lws);
struct clientlist *findclientlist (const char *topic);
void showtopology (char *filename, int line);

#endif